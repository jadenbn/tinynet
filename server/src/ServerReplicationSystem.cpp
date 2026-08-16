#include "../server/include/ServerReplicationSystem.h"
#include "../server/include/Server.h"
#include "Packets.h"
#include "Protocol.h"

ServerReplicationSystem::ServerReplicationSystem(ServerGame &c_game)
    : game(c_game) {};

bool ServerReplicationSystem::HandlePacket(const ClientID id, Buffer &buff) {
  if (buff.index >= buff.length)
    return false;
  PacketType packetType = static_cast<PacketType>(packets::ReadChar(buff));

  switch (packetType) {
  case PacketType::PlayerInput:
    ApplyPlayerInputPacket(PlayerInputPacket::deserialize(buff));
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