#pragma once
#include "Movement.h"
#include <deque>

// Kept free of rendering so movement and input acknowledgement can be tested.
class ClientSimulation {
public:
  PlayerState local;
  bool ready = false;
  std::deque<InputCommand> pending;

  bool AddInput(Vec3 direction, float yaw) {
    if (!ready || pending.size() >= MAX_PENDING_INPUTS)
      return false;
    uint32_t sequence = pending.empty() ? local.lastInput + 1
                                       : pending.back().sequence + 1;
    InputCommand input{sequence, direction, yaw};
    if (!ValidInput(input))
      return false;
    pending.push_back(input);
    SimulateMovement(local, input);
    return true;
  }

  bool Reconcile(const PlayerState &state) {
    if (ready && ((state.lastInput != acknowledged &&
                   !IsNewer(state.lastInput, acknowledged)) ||
                  IsNewer(state.lastInput, local.lastInput)))
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
private:
  uint32_t acknowledged = 0;
};
