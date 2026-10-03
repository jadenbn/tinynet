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
    return true;
  }

  void Reconcile(const PlayerState &state) {
    while (!pending.empty() && !IsNewer(pending.front().sequence, state.lastInput))
      pending.pop_front();
    local = state;
    ready = true;
  }
};
