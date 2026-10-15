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

// the pitch. x runs goal to goal, z touchline to touchline, y is up. walled
// in like futsal/indoor football, so the ball never goes out of play
constexpr float PITCH_HALF_LENGTH = 20; // goal lines at x = +-20
constexpr float PITCH_HALF_WIDTH = 12;  // side walls at z = +-12
constexpr float GOAL_HALF_WIDTH = 3;    // posts at z = +-3
constexpr float GOAL_HEIGHT = 2.4f;     // crossbar
constexpr float GOAL_DEPTH = 1.5f;      // back of the net
constexpr float POST_RADIUS = 0.1f;
constexpr float BALL_RADIUS = 0.3f;     // big for a football, easier to see
constexpr float PLAYER_RADIUS = 0.5f;
constexpr float PLAYER_HEIGHT = 1.8f;   // balls higher than this go over us

inline bool IsNewer(uint32_t a, uint32_t b) {
  return a != b && a - b < 0x80000000u;
}

enum class KickType : uint8_t { None, Ground, Chip };

// stored the way it goes over the wire, so the client predicts with exactly
// the numbers the server will see
struct KickCommand {
  KickType type = KickType::None;
  uint8_t power = 0; // 0-255 -> softest to hardest
  int8_t curve = 0;  // -127 bends left, 127 bends right
};

struct InputCommand {
  uint32_t sequence = 0;
  Vec3 direction;
  float yaw = 0;
  KickCommand kick; // kicks on this tick, if the ball's close enough
};

struct PlayerState {
  PlayerID playerID = 0;
  Vec3 pos;
  Vec3 velocity;
  Quat orientation;
  uint32_t lastInput = 0;
  uint8_t kickCooldown = 0; // ticks until we can kick again
};

struct BallState {
  Vec3 pos{0, BALL_RADIUS, 0}; // centre, so resting on the grass is y = radius
  Vec3 velocity;
  float spin = 0; // rad/s around the vertical axis, curves the ball
};
