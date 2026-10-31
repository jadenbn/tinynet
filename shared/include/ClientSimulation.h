#pragma once
#include "Movement.h"
#include "Protocol.h"
#include <deque>
#include <vector>

// the client's prediction: our own player AND the ball, run ahead of the
// server by however many inputs it hasn't processed yet. that puts the ball in
// the same moment as us, so our touches and kicks happen instantly instead of
// a round trip later. kept free of rendering so it can be tested
class ClientSimulation {
public:
  static constexpr float SNAP_DISTANCE = 2.0f; // corrections bigger than this are teleports
  static constexpr float PLAYER_SMOOTHING = 0.1f; // s, time constant
  static constexpr float BALL_SMOOTHING = 0.06f;
  // the ball lives in our (predicted) timeframe when it's near us, and in
  // everyone else's (interpolated) when it isn't. blended in between
  static constexpr float OUR_BALL = 3.0f, THEIR_BALL = 8.0f; // metres away

  // 1 = draw our prediction, 0 = draw the interpolated past. smoothstep, so
  // the blend has no kinks as the ball comes and goes
  static float PredictionWeight(float distance) {
    float t = std::clamp((THEIR_BALL - distance) / (THEIR_BALL - OUR_BALL), 0.0f, 1.0f);
    return t * t * (3 - 2 * t);
  }

  PlayerState local;
  BallState ball;
  bool ready = false;
  std::deque<InputCommand> pending;

  bool AddInput(Vec3 direction, float yaw, KickCommand kick = {});

  // just our player, no ball
  bool Reconcile(const PlayerState &state);
  // our player, the ball, and everyone else (for the ball to bump into)
  bool Reconcile(const WorldSnapshot &snapshot, PlayerID localID);

  // where to draw things. corrections from the server don't jump: we keep
  // drawing where we were, and let that offset shrink over a few frames.
  // alpha blends between the last two ticks, so 144Hz frames look smooth too
  void DecayErrors(float dt);
  Vec3 RenderLocal(float alpha) const;
  Vec3 RenderBall(float alpha) const;
  Vec3 LocalError() const { return localError; }
  Vec3 BallError() const { return ballError; }
  bool PredictingBall() const { return predictBall; }
  // everyone else, as of the last snapshot
  const std::vector<PlayerState> &Others() const { return others; }

private:
  uint32_t acknowledged = 0;
  bool predictBall = false;
  std::vector<PlayerState> others; // as of the last snapshot
  int stepsSinceSnapshot = 0;
  Vec3 localError, ballError;
  Vec3 previousLocal, previousBall; // last tick's positions, for alpha

  void Step(const InputCommand &input);
  bool Stale(const PlayerState &state) const;
};
