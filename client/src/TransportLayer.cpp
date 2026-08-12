#include "TransportLayer.h"
#include "Packets.h"
#include "Protocol.h"
#include <iostream>

TransportLayer::TransportLayer(Game &c_game) : game(c_game) {};

bool TransportLayer::HandlePacket(Buffer &buff) {
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

  return false;
}

bool TransportLayer::ApplyPlayerInputPacket(const PlayerInputPacket &p) {
  std::cout << "applying player input packet" << '\n';
  game.playerPosition = {p.x, p.y};
  return true;
}