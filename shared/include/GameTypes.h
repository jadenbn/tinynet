#pragma once
#include "VectorMath.h"
#include <cstdint>

using PlayerID = uint32_t;
constexpr float TICK_DT = 1.0f / 60;
constexpr float WORLD_BOUND = 100;
constexpr uint32_t MAX_PENDING_INPUTS = 120;
constexpr uint32_t INPUTS_PER_PACKET = 32;
// shortcut: one snapshot per datagram, add splitting before exceeding 24 players.
constexpr uint32_t MAX_PLAYERS = 24;

inline bool IsNewer(uint32_t a, uint32_t b) {
  return a != b && a - b < 0x80000000u;
}

struct InputCommand {
  uint32_t sequence = 0;
  Vec3 direction;
  float yaw = 0;
};

struct PlayerState {
  PlayerID playerID = 0;
  Vec3 pos;
  Vec3 velocity;
  Quat orientation;
  uint32_t lastInput = 0;
};
