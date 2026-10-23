#pragma once
#include "GameTypes.h"

// the server plays a keeper for each team, so there's something to beat even
// on your own. they go through the same input -> SimulateMovement path as
// everyone else, so clients can't tell them apart from a person

namespace keeper {
// tuned with the shooting gallery in test_keeper.cpp
constexpr int REACTION_TICKS = 15;  // what a keeper "sees" is 250ms old
constexpr float SPEED = 0.65f;      // slower to get going than a person
constexpr float LINE_DEPTH = 1.2f;  // how far off its goal line it stands
constexpr float MAX_DEPTH = 4.0f;   // the furthest it'll come out
constexpr float LOOKAHEAD = 1.5f;   // seconds of ball flight it predicts
} // namespace keeper

// the goal line a team defends: team 0 attacks +x, so it defends -x
float OwnGoalLine(uint8_t team);

// where the keeper wants to stand. normally on the line splitting the angle
// between the ball and the two posts, which covers both sides of the goal
// equally. if a shot is coming, wherever it'll cross the keeper's line
Vec3 KeeperTarget(uint8_t team, const BallState &ball);

// run to the target (easing off as it gets there), face the ball, and boot it
// away when it's close enough
InputCommand KeeperInput(const PlayerState &keeper, const BallState &ball);
