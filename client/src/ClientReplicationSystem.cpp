#include "ClientReplicationSystem.h"
#include "../shared/include/Protocol.h"
#include "Packets.h"
#include <iostream>

ClientReplicationSystem::ClientReplicationSystem(ClientGame &c_game)
    : game(c_game) {};

bool ClientReplicationSystem::HandlePacket(Buffer &buff) {
  if (buff.index >= buff.size)
    return false;
  PacketType packetType = static_cast<PacketType>(ReadChar(buff));

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
  game.playerPosition = {p.player1X, p.player1Y};
  return true;
}