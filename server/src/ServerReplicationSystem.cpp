#include "../server/include/ServerReplicationSystem.h"
#include "Packets.h"
#include <iostream>

ServerReplicationSystem::ServerReplicationSystem(ServerGame &c_game)
    : game(c_game) {};

bool ServerReplicationSystem::HandlePacket(Buffer &buff) {
  if (buff.index >= buff.size)
    return false;
  PacketType packetType = static_cast<PacketType>(ReadChar(buff));

  switch (packetType) {
  case PacketType::PlayerInput:
    ApplyPlayerInputPacket(PlayerInputPacket::deserialize(buff));
    break;
  case PacketType::ConnectionRequest:
  case PacketType::Heartbeat:
    break;
  default:
    return false;
  }

  return true;
}

bool ServerReplicationSystem::ApplyPlayerInputPacket(
    const PlayerInputPacket &p) {
  game.playerPosition = {p.x, p.y};
  return true;
}