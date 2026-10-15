#pragma once
#include "GameTypes.h"

inline bool ValidInput(const InputCommand &input) {
  return IsFinite(input.direction) && input.direction.y == 0 &&
         Dot(input.direction, input.direction) <= 1.001f &&
         std::isfinite(input.yaw) && std::abs(input.yaw) <= 1000 &&
         input.kick.type <= KickType::Chip;
}

inline void SimulateMovement(PlayerState &player, const InputCommand &input) {
  constexpr float acceleration = 24, maxSpeed = 6, friction = 8;
  player.orientation = Yaw(input.yaw);
  Vec3 direction = input.direction;
  if (Dot(direction, direction) > 1)
    direction = Normalize(direction);
  Vec3 worldDirection = Rotate(player.orientation, direction);
  player.velocity = player.velocity + worldDirection * (acceleration * TICK_DT);
  player.pos = player.pos + player.velocity * TICK_DT;
  float speed = std::sqrt(Dot(player.velocity, player.velocity));
  if (speed > 0)
    player.velocity = player.velocity *
        (std::max(0.0f, std::min(speed, maxSpeed) - friction*TICK_DT) / speed);
  // the pitch is walled in; running into a wall stops you on that axis
  float *positions[] = {&player.pos.x, &player.pos.y, &player.pos.z};
  float *velocities[] = {&player.velocity.x, &player.velocity.y, &player.velocity.z};
  const float bounds[] = {PITCH_HALF_LENGTH - PLAYER_RADIUS, WORLD_BOUND,
                          PITCH_HALF_WIDTH - PLAYER_RADIUS};
  for (int i = 0; i < 3; ++i) {
    if (std::abs(*positions[i]) > bounds[i]) {
      *positions[i] = std::clamp(*positions[i], -bounds[i], bounds[i]);
      *velocities[i] = 0;
    }
  }
  if (player.kickCooldown > 0)
    player.kickCooldown--;
  player.lastInput = input.sequence;
}
