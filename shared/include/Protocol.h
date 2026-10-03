#pragma once

#include "GameTypes.h"
#include "Packets.h"
#include <cstdint>
#include <unordered_map>
#include <vector>

constexpr uint32_t PROTOCOL_HASH = 0x12345679;

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
  uint32_t tick = 0;
  std::unordered_map<PlayerID, PlayerState> players;

  void Serialize(Buffer &buff) const;
  static WorldSnapshot deserialize(Buffer &buff);
};

struct PlayerInputPacket {
  std::vector<InputCommand> inputs;

  void Serialize(Buffer &buff) const;
  static PlayerInputPacket deserialize(Buffer &buff);
};

struct ConnectionRequest {

  void Serialize(Buffer &buff) const;
  static ConnectionRequest deserialize(Buffer &buff);
};

// sent when there's nothing else to say, so the other side keeps getting acks
// and doesn't think we've timed out
struct Heartbeat {

  void Serialize(Buffer &buff) const;
  static Heartbeat deserialize(Buffer &buff);
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
