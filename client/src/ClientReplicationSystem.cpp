#include "ClientReplicationSystem.h"
#include "game/ClientWorld.h"
#include <stdexcept>

ClientReplicationSystem::ClientReplicationSystem(ClientWorld &world,
                                                 ClientSimulation &simulation)
    : world(world), simulation(simulation) {}

bool ClientReplicationSystem::HandlePacket(Buffer &buff) {
  if (buff.index >= buff.length)
    return false;
  try {
    switch (static_cast<PacketType>(packets::ReadChar(buff))) {
    case PacketType::WorldSnapshot:
      return ApplyWorldSnapshot(WorldSnapshot::deserialize(buff));
    case PacketType::ConnectionAccepted:
    case PacketType::Heartbeat:
      return true;
    default:
      return false;
    }
  } catch (const std::invalid_argument &) {
    return false;
  }
}

bool ClientReplicationSystem::ApplyWorldSnapshot(const WorldSnapshot &snapshot) {
  if (hasSnapshot && !IsNewer(snapshot.tick, latestTick))
    return false;
  latestTick = snapshot.tick;
  hasSnapshot = true;
  for (auto it = world.players.begin(); it != world.players.end();) {
    if (!snapshot.players.contains(it->first))
      it = world.players.erase(it);
    else
      ++it;
  }
  for (const auto &[id, state] : snapshot.players) {
    world.AddPlayer(id);
    world.players.at(id).state = state;
    if (id == localID)
      simulation.Reconcile(state);
  }
  return true;
}
