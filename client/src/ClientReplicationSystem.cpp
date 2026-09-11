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
  if (world.players.size() <= 0) {
    world.players.push_back(Player(0)); // tmp
  }
  world.players[0].pos = {p.player1X, p.player1Y};
  return true;
}