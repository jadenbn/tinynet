#include "../shared/include/Protocol.h"
#include "Packets.h"
#include "ServerWorld.h"
#include <cstdint>
#include <utility>

void WorldSnapshot::Serialize(Buffer &buff) const {
  // writes as:
  // playerID
  // position x
  // position y
  packets::WriteChar(buff, PacketType::WorldSnapshot);
  packets::WriteInteger(buff, players.size());

  for (const auto &[id, reference] : players) {
    packets::WriteInteger(buff, reference.playerID);
    packets::WriteFloat(buff, reference.position.x);
    packets::WriteFloat(buff, reference.position.y);
  }
}

WorldSnapshot WorldSnapshot::deserialize(Buffer &buff) {
  uint32_t playersListSize = packets::ReadInteger(buff);
  std::unordered_map<PlayerID, ServerTypes::ServerPlayer> reconstruct;

  for (int i = 0; i < playersListSize; i++) {
    uint32_t id = packets::ReadInteger(buff);
    float x = packets::ReadFloat(buff);
    float y = packets::ReadFloat(buff);

    reconstruct.emplace(
        std::pair<uint32_t, ServerTypes::ServerPlayer>(id, {id, {x, y}, id}));
  }

  return {playersListSize, reconstruct};
}

void PlayerInputPacket::Serialize(Buffer &buff) const {
  packets::WriteChar(buff, PacketType::PlayerInput);
  packets::WriteFloat(buff, this->direction.x);
  packets::WriteFloat(buff, this->direction.y);
}

PlayerInputPacket PlayerInputPacket::deserialize(Buffer &buff) {
  return {packets::ReadFloat(buff), packets::ReadFloat(buff)};
}

void ConnectionRequest::Serialize(Buffer &buff) const {
  packets::WriteChar(buff, PacketType::ConnectionRequest);
}

ConnectionRequest ConnectionRequest::deserialize(Buffer &buff) { return {}; }

void ConnectionAccepted::Serialize(Buffer &buff) const {
  packets::WriteChar(buff, PacketType::ConnectionAccepted);
  packets::WriteInteger(buff, clientID);
}

ConnectionAccepted ConnectionAccepted::deserialize(Buffer &buff) {
  return {packets::ReadInteger(buff)};
};

void ConnectionRejected::Serialize(Buffer &buff) const {
  packets::WriteChar(buff, PacketType::ConnectionRejected);
  packets::WriteChar(buff, reason);
}

ConnectionRejected ConnectionRejected::deserialize(Buffer &buff) {
  return {packets::ReadChar(buff)};
};
