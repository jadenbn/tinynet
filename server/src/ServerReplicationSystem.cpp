#include "ServerReplicationSystem.h"
#include "Packets.h"
#include "Protocol.h"
#include "ServerWorld.h"
#include <stdexcept>

ServerReplicationSystem::ServerReplicationSystem(ServerWorld &world) : game(world) {}

bool ServerReplicationSystem::HandlePacket(ClientID id, Buffer &buff) {
  if (buff.index >= buff.length)
    return false;
  try {
    switch (static_cast<PacketType>(packets::ReadChar(buff))) {
    case PacketType::PlayerInput:
      return ApplyPlayerInputPacket(PlayerInputPacket::deserialize(buff), id);
    case PacketType::ConnectionRequest:
      return game.AddPlayer(id);
    case PacketType::Heartbeat:
      return true;
    default:
      return false;
    }
  } catch (const std::invalid_argument &) {
    return false;
  }
}

bool ServerReplicationSystem::ApplyPlayerInputPacket(const PlayerInputPacket &packet,
                                                     ClientID id) {
  return game.QueueInputs(id, packet);
}
