#pragma once

#include "Packets.h"
#include <cstdint>
constexpr uint32_t PROTOCOL_HASH = 0x12345678;

enum class PacketType : uint8_t {
  ConnectionRequest = 0,
  ConnectionAccepted,
  ConnectionRejected,
  Heartbeat,
  PlayerInput,
  WorldSnapshot,
};

namespace packets {

inline void WriteChar(Buffer &buff, PacketType type) {
  packets::WriteChar(buff, static_cast<uint8_t>(type));
};

} // namespace packets

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

struct ConnectionRequest {

  void Serialize(Buffer &buff) const;
  static ConnectionRequest deserialize(Buffer &buff);
};
struct ConnectionAccepted {
  uint32_t clientID;

  void Serialize(Buffer &buff) const;
  static ConnectionAccepted deserialize(Buffer &buff);
};
struct ConnectionRejected {
  uint8_t reason; // kind of temporary icl

  void Serialize(Buffer &buff) const;
  static ConnectionRejected deserialize(Buffer &buff);
};