#include "ClientReplicationSystem.h"
#include "Address.h"
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
  case PacketType::ConnectionRequest:
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
  
  // world.players = p.players();
  for (const auto &[id, ref] : p.players) {
    world.players[id].pos.x = ref.position.x;
    world.players[id].pos.y = ref.position.y;
  }
  return true;
}
