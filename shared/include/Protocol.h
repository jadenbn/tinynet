#pragma once

#include "GameTypes.h"
#include "Packets.h"
#include <cstdint>
#include <unordered_map>
#include <vector>

constexpr uint32_t PROTOCOL_HASH = 0x12345683;

enum class PacketType : uint8_t {
  ConnectionRequest = 0,
  ConnectionAccepted,
  ConnectionRejected,
  Heartbeat,
  PlayerInput,
  WorldSnapshot,
  Disconnect,
};

namespace packets {

inline void WriteChar(Buffer &buff, PacketType type) {
  packets::WriteChar(buff, static_cast<uint8_t>(type));
};

} // namespace packets

struct WorldSnapshot {
  uint32_t tick = 0;
  std::unordered_map<PlayerID, PlayerState> players;
  MatchState match;
  BallState ball;

  void Serialize(Buffer &buff) const;
  void SerializeRaw(Buffer &buff) const; // same state, floats: benchmark baseline
  static WorldSnapshot deserialize(Buffer &buff);
};

struct PlayerInputPacket {
  std::vector<InputCommand> inputs;

  void Serialize(Buffer &buff) const;
  static PlayerInputPacket deserialize(Buffer &buff);
};

// one game event per reliable message. there's no packet type byte, since the
// reliable channel is its own stream inside every packet
constexpr int QUICK_CHAT_PHRASES = 4;
struct EventMessage {
  GameEvent event;

  void Serialize(Buffer &buff) const;
  static EventMessage deserialize(Buffer &buff);
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
// "i'm leaving". nobody will be around to resend it, so it goes out a few
// times; losing all of them just means waiting for the timeout like before
constexpr int DISCONNECT_PACKETS = 3;
struct DisconnectPacket {

  void Serialize(Buffer &buff) const;
  static DisconnectPacket deserialize(Buffer &buff);
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
