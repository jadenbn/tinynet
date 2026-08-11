#pragma once

#include "Packets.h"
#include <cstdint>
constexpr uint32_t PROTOCOL_HASH = 0x12345678;

enum class PacketType : uint8_t {
  ConnectionRequest = 1,
  Heartbeat,
  PlayerInput,
};


inline void WriteChar(Buffer &buff, PacketType type) {
  WriteChar(buff, static_cast<uint8_t>(type));
};

struct PlayerInputPacket {
  float x;
  float y;

  void Serialize(Buffer &buff);
  const PlayerInputPacket deserialize(Buffer buff);
};