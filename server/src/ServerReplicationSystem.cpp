#include "../server/include/ServerReplicationSystem.h"
#include "../server/include/Server.h"
#include "GameTypes.h"
#include "Packets.h"
#include "Protocol.h"
#include "ServerWorld.h"
#include <iostream>

ServerReplicationSystem::ServerReplicationSystem(ServerWorld &c_game)
    : game(c_game) {};

bool ServerReplicationSystem::HandlePacket(const ClientID id, Buffer &buff) {
  if (buff.index >= buff.length)
    return false;
  PacketType packetType = static_cast<PacketType>(packets::ReadChar(buff));

  switch (packetType) {
  case PacketType::PlayerInput:
    ApplyPlayerInputPacket(PlayerInputPacket::deserialize(buff));
    break;

  case PacketType::ConnectionRequest: {
    ServerTypes::ServerPlayer newPlayer(id, NetworkSafeVector2{0.0f, 0.0f}, id);
    game.AddPlayer(newPlayer);
    break;
  }

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
