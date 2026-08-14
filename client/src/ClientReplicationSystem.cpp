#include "ClientReplicationSystem.h"
#include "Packets.h"
#include "Protocol.h"
#include "game/ClientWorld.h"
#include <iostream>

ClientReplicationSystem::ClientReplicationSystem(ClientWorld &c_game,
                                                 Client &client_c)
    : game(c_game), client(client_c) {};

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
  // game.playerPosition = {p.player1X, p.player1Y};

  std::cout << "received from serv" << '\n';
  return true;
}