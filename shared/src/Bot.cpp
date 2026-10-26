#include "Bot.h"
#include "Ball.h"
#include <cmath>

namespace {
Vec3 Flat(Vec3 v) { return {v.x, 0, v.z}; }
float Length(Vec3 v) { return std::sqrt(Dot(v, v)); }
} // namespace

BotDecision BotThink(const PlayerState &me, const BallState &ball,
                     const std::vector<PlayerState> &others, uint32_t tick) {
  float attack = me.team == 0 ? 1.0f : -1.0f;
  // pick a corner, and switch every few seconds so it isn't predictable
  float corner = (tick / 240 + me.playerID) % 2 ? 1.0f : -1.0f;
  Vec3 target{attack * PITCH_HALF_LENGTH, 0, corner * (GOAL_HALF_WIDTH - 0.5f)};
  Vec3 b = Flat(ball.pos);
  Vec3 toTarget = Normalize(target - b);
  Vec3 toBall = b - Flat(me.pos);
  float distance = Length(toBall);

  BotDecision decision;
  decision.aim = b;

  // a teammate's closer: hang back and wide for a rebound instead of both
  // chasing it
  for (const PlayerState &other : others) {
    if (other.team != me.team || other.keeper)
      continue;
    float theirs = Length(b - Flat(other.pos));
    if (theirs + 1 < distance || (std::abs(theirs - distance) <= 1 && other.playerID < me.playerID)) {
      float wide = b.z > 0 ? -5.0f : 5.0f;
      Vec3 spot{std::clamp(b.x - attack * 5, -PITCH_HALF_LENGTH + 2, PITCH_HALF_LENGTH - 2),
                0, std::clamp(b.z + wide, -PITCH_HALF_WIDTH + 2, PITCH_HALF_WIDTH - 2)};
      Vec3 toSpot = spot - Flat(me.pos);
      decision.move = Length(toSpot) > 0.5f ? Normalize(toSpot) : Vec3{};
      return decision;
    }
  }
  bool behind = distance > 0.01f && Dot(toBall * (1 / distance), toTarget) > 0.7f;
  if (behind && distance < 2.5f) {
    // lined up: run through the ball towards goal, then hit it
    decision.move = Normalize(toTarget + toBall * (1 / distance));
    decision.aim = target;
    float toGoal = Length(target - b);
    if (distance < 1.1f && toGoal < 15 && me.kickCooldown == 0)
      decision.kick = {KickType::Ground, static_cast<uint8_t>(toGoal < 9 ? 200 : 245),
                       static_cast<int8_t>(tick % 3 == 0 ? -corner * 70 : 0)};
  } else {
    // get round to the spot behind the ball. if we're on the wrong side,
    // go wide of it first so we don't knock it backwards
    Vec3 spot = b - toTarget * 1.2f;
    if (Dot(toBall, toTarget) < 0) {
      Vec3 side{-toTarget.z, 0, toTarget.x};
      if (Dot(side, Flat(me.pos) - b) < 0)
        side = side * -1;
      spot = b + side * 1.5f - toTarget * 0.5f;
    }
    decision.move = Normalize(spot - Flat(me.pos));
  }
  return decision;
}
