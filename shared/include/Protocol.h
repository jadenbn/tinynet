#pragma once

#include "Packets.h"
#include <cstdint>
constexpr uint32_t PROTOCOL_HASH = 0x12345678;

enum class PacketType : uint8_t {
  ConnectionRequest = 1,
  Heartbeat,
  PlayerInput,
  WorldSnapshot,
};

inline void WriteChar(Buffer &buff, PacketType type) {
  WriteChar(buff, static_cast<uint8_t>(type));
};

struct WorldSnapshot {
  float player1X;
  float player1Y;

  void Serialize(Buffer &buff) const;
  static WorldSnapshot deserialize(Buffer &buff);
};

struct PlayerInputPacket {
  float x;
  float y;

  void Serialize(Buffer &buff) const;
  static PlayerInputPacket deserialize(Buffer &buff);
};

struct ConnectionRequest {};
struct ConnectionAccepted {
  uint32_t clientID;
};
struct ConnectionRejected {
  uint8_t reason; // kind of temporary icl
};