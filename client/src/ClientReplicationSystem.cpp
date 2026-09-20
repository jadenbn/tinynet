#include "ClientReplicationSystem.h"
#include "Packets.h"
#include "Protocol.h"
#include "game/ClientWorld.h"
#include <iostream>

ClientReplicationSystem::ClientReplicationSystem(ClientWorld &c_game,
                                                 Client &client_c)
    : world(c_game), client(client_c) {};

bool ClientReplicationSystem::HandlePacket(Buffer &buff) {
  if (buff.index >= buff.length)
    return false;
  PacketType packetType = static_cast<PacketType>(packets::ReadChar(buff));

  switch (packetType) {
  case PacketType::WorldSnapshot:
    ApplyWorldSnapshot(WorldSnapshot::deserialize(buff));
    break;
  case PacketType::ConnectionAccepted:
    break;
  case PacketType::ConnectionRequest:
    break;
  case PacketType::Heartbeat:
    break;
  default:
    std::cout << "ClientReplicationSystem received an unknown packet!" << '\n';
    return false;
  }

  return true;
}

bool ClientReplicationSystem::ApplyWorldSnapshot(const WorldSnapshot &p) {
  // TODO: optimize. we are aligning every player based on the network every
  // tick. we should either do this by keeping track of dirty changes, or change
  // the architecture so that we just update based on the authoritative server
  // when we need it.
  for (const auto &[id, ref] : p.players) {
    if (!world.players.contains(id)) {
        world.AddPlayer(id);
    }

    world.players.at(id).pos.x = ref.pos.x;
    world.players.at(id).pos.y = ref.pos.y;
  }

  return true;
}
