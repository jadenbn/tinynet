#include "ClientSimulation.h"
#include "Ball.h"
#include "Movement.h"
#include <cmath>

namespace {
// a correction moves the simulation by `delta`. draw where we were anyway,
// unless it's so big it must be a teleport (like a kickoff)
void Absorb(Vec3 &error, Vec3 &previous, Vec3 delta) {
  error = error - delta;
  previous = previous + delta;
  if (Dot(error, error) > ClientSimulation::SNAP_DISTANCE * ClientSimulation::SNAP_DISTANCE)
    error = {};
}
} // namespace

bool ClientSimulation::AddInput(Vec3 direction, float yaw, KickCommand kick) {
  if (!ready || pending.size() >= MAX_PENDING_INPUTS)
    return false;
  uint32_t sequence = pending.empty() ? local.lastInput + 1
                                      : pending.back().sequence + 1;
  InputCommand input{sequence, direction, yaw, kick};
  if (!ValidInput(input))
    return false;
  pending.push_back(input);
  previousLocal = local.pos;
  previousBall = ball.pos;
  Step(input);
  return true;
}

// one tick, in the same order as ServerWorld::Tick
void ClientSimulation::Step(const InputCommand &input) {
  SimulateMovement(local, input);
  if (!predictBall)
    return;
  TryKick(local, input.kick, ball);
  SimulateBall(ball);
  CollidePlayerBall(local, ball);

  // nobody else's inputs are here yet, so guess they keep going the way they
  // were (dead reckoning). only the ball uses this; they're still drawn from
  // real snapshots
  stepsSinceSnapshot++;
  float ahead = stepsSinceSnapshot * TICK_DT;
  for (PlayerState other : others) {
    other.pos = other.pos + other.velocity * ahead;
    other.pos.x = std::clamp(other.pos.x, -PITCH_HALF_LENGTH, PITCH_HALF_LENGTH);
    other.pos.z = std::clamp(other.pos.z, -PITCH_HALF_WIDTH, PITCH_HALF_WIDTH);
    CollidePlayerBall(other, ball);
  }
}

bool ClientSimulation::Stale(const PlayerState &state) const {
  // older than what we've already applied, or acking an input we never sent
  return ready && ((state.lastInput != acknowledged &&
                    !IsNewer(state.lastInput, acknowledged)) ||
                   IsNewer(state.lastInput, local.lastInput));
}

bool ClientSimulation::Reconcile(const PlayerState &state) {
  if (Stale(state))
    return false;
  while (!pending.empty() && !IsNewer(pending.front().sequence, state.lastInput))
    pending.pop_front();
  local = state;
  acknowledged = state.lastInput;
  for (const auto &input : pending)
    SimulateMovement(local, input);
  ready = true;
  return true;
}

bool ClientSimulation::Reconcile(const WorldSnapshot &snapshot, PlayerID localID) {
  auto self = snapshot.players.find(localID);
  if (self == snapshot.players.end() || Stale(self->second))
    return false;

  bool wasReady = ready, hadBall = predictBall;
  Vec3 oldLocal = local.pos, oldBall = ball.pos;

  others.clear();
  for (const auto &[id, state] : snapshot.players)
    if (id != localID)
      others.push_back(state);
  ball = snapshot.ball;
  predictBall = true;
  stepsSinceSnapshot = 0;

  // rewind to what the server says, then replay everything it hasn't seen
  while (!pending.empty() && !IsNewer(pending.front().sequence, self->second.lastInput))
    pending.pop_front();
  local = self->second;
  acknowledged = local.lastInput;
  for (const auto &input : pending)
    Step(input);
  ready = true;

  if (wasReady)
    Absorb(localError, previousLocal, local.pos - oldLocal);
  else
    previousLocal = local.pos;
  if (hadBall)
    Absorb(ballError, previousBall, ball.pos - oldBall);
  else
    previousBall = ball.pos;
  return true;
}

void ClientSimulation::DecayErrors(float dt) {
  // exponential decay: the same fraction goes every second however often
  // we're called, so it looks the same at 60 or 144 fps
  localError = localError * std::exp(-dt / PLAYER_SMOOTHING);
  ballError = ballError * std::exp(-dt / BALL_SMOOTHING);
}

Vec3 ClientSimulation::RenderLocal(float alpha) const {
  return Lerp(previousLocal, local.pos, alpha) + localError;
}

Vec3 ClientSimulation::RenderBall(float alpha) const {
  return Lerp(previousBall, ball.pos, alpha) + ballError;
}
