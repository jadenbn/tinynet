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

    std::cout << "client received packet from server" << '\n';
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
  std::cout << "asdflk" << '\n';
  game.playerPosition = {p.x, p.y};
  return true;
}