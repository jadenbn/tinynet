#include "Goalkeeper.h"
#include "Ball.h"
#include <cmath>

using namespace keeper;

namespace {
Vec3 Horizontal(Vec3 v) { return {v.x, 0, v.z}; }
float Length(Vec3 v) { return std::sqrt(Dot(v, v)); }

// which way is "out of our goal, into the pitch" along x
float Outwards(uint8_t team) { return team == 0 ? 1.0f : -1.0f; }

Vec3 ClampToBox(uint8_t team, Vec3 target) {
  float line = OwnGoalLine(team), out = Outwards(team);
  float depth = std::clamp(out * (target.x - line), 0.6f, MAX_DEPTH);
  return {line + out * depth, 0,
          std::clamp(target.z, -GOAL_HALF_WIDTH - 1, GOAL_HALF_WIDTH + 1)};
}
} // namespace

float OwnGoalLine(uint8_t team) {
  return team == 0 ? -PITCH_HALF_LENGTH : PITCH_HALF_LENGTH;
}

Vec3 KeeperTarget(uint8_t team, const BallState &ball) {
  float line = OwnGoalLine(team), out = Outwards(team);
  float keeperX = line + out * LINE_DEPTH;
  Vec3 b = Horizontal(ball.pos);

  // a shot on the way: run the same physics forward and see where it'll
  // cross our line, bounces and all. keepers read the line of a shot but not
  // the spin on it, so a well struck curler can still beat them
  if (out * ball.velocity.x < -1) {
    BallState future = ball;
    future.spin = 0;
    for (int i = 0; i < static_cast<int>(LOOKAHEAD / TICK_DT); i++) {
      SimulateBall(future);
      if (out * (future.pos.x - keeperX) <= 0) {
        if (std::abs(future.pos.z) < GOAL_HALF_WIDTH + 1)
          return ClampToBox(team, {keeperX, 0, future.pos.z});
        break; // going wide, no need to chase it
      }
    }
  }

  // already past us: get back towards the near post
  if (out * (b.x - keeperX) <= 0.1f)
    return ClampToBox(team, {keeperX, 0, b.z});

  // the angle bisector: add the unit vectors from the ball to each post.
  // anywhere on that line is the same angle from either post
  Vec3 toLeft = Normalize(Vec3{line, 0, -GOAL_HALF_WIDTH} - b);
  Vec3 toRight = Normalize(Vec3{line, 0, GOAL_HALF_WIDTH} - b);
  Vec3 bisector = Normalize(toLeft + toRight);
  // follow it from the ball until it reaches the keeper's line: solve
  // b.x + t * bisector.x = keeperX for t
  float t = (keeperX - b.x) / bisector.x;
  return ClampToBox(team, b + bisector * t);
}

InputCommand KeeperInput(const PlayerState &keeper, const BallState &ball) {
  InputCommand input;
  input.sequence = keeper.lastInput + 1;
  Vec3 toBall = Horizontal(ball.pos - keeper.pos);
  input.yaw = Length(toBall) > 0.01f ? YawTowards(toBall)
                                     : YawTowards(Vec3{Outwards(keeper.team), 0, 0});

  // "arrive": aim for a velocity that points at the target and shrinks as we
  // get close, and push towards that. heading straight for the target at full
  // speed would overshoot and wobble about
  Vec3 offset = KeeperTarget(keeper.team, ball) - keeper.pos;
  offset.y = 0;
  float distance = Length(offset);
  Vec3 wanted = distance > 0.001f ? offset * (std::min(6.0f, distance * 4) / distance)
                                  : Vec3{};
  Vec3 steer = wanted - Horizontal(keeper.velocity);
  Vec3 world = Length(steer) > 0.2f ? Normalize(steer) * SPEED : Vec3{};

  // inputs are relative to where we're facing, so undo our own rotation
  input.direction = Rotate(Conjugate(Yaw(input.yaw)), world);
  input.direction.y = 0;

  // boot it clear, but never towards our own goal
  Vec3 forward = Rotate(Yaw(input.yaw), {0, 0, 1});
  bool facingOut = forward.x * Outwards(keeper.team) > 0.3f;
  if (facingOut && Length(toBall) < PLAYER_RADIUS + BALL_RADIUS + kick::REACH)
    input.kick = {KickType::Chip, 190, 0};
  return input;
}
