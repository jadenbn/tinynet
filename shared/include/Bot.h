#pragma once
#include "GameTypes.h"
#include <vector>

// a stand-in outfield player: gets behind the ball, dribbles it at goal and
// shoots for a corner. used to fill a server for testing (client --bot) and
// by the soak test. it makes the same inputs a person would: a direction to
// move in, somewhere to face, and sometimes a kick
struct BotDecision {
  Vec3 move;   // world direction, length <= 1
  Vec3 aim;    // world point to face
  KickCommand kick;
};

// `others` is everyone else as we last saw them, to tell teammates apart
BotDecision BotThink(const PlayerState &me, const BallState &ball,
                     const std::vector<PlayerState> &others, uint32_t tick);
