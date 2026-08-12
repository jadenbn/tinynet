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

bool ClientReplicationSystem::ApplyPlayerInputPacket(
    const PlayerInputPacket &p) {
  game.playerPosition = {p.x, p.y};
  return true;
}