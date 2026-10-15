#pragma once
#include "GameTypes.h"

// ball physics, shared so the server and the client's prediction run the
// exact same code. everything steps at TICK_DT

namespace ball {
constexpr float GRAVITY = 9.81f;
constexpr float AIR_DRAG = 0.12f;      // 1/s, exponential
constexpr float ROLL_FRICTION = 2.2f;  // m/s^2 of slowing down on the grass
constexpr float ROLL_DRAG = 0.25f;     // 1/s, on top of the friction
constexpr float BOUNCE = 0.55f;        // vertical speed kept off the grass
constexpr float GRASS_GRIP = 0.9f;     // horizontal speed kept on a bounce
constexpr float SETTLE_SPEED = 0.6f;   // slower bounces than this just roll
constexpr float WALL_BOUNCE = 0.7f;
constexpr float POST_BOUNCE = 0.6f;
constexpr float NET_BOUNCE = 0.15f;    // the net soaks most of it up
constexpr float CURVE = 0.045f;        // rad/s of turning per rad/s of spin
constexpr float SPIN_DECAY_AIR = 0.4f; // 1/s
constexpr float SPIN_DECAY_GROUND = 3.0f;
} // namespace ball

// resting or rolling on the grass, as opposed to in the air
bool OnGround(const BallState &ball);

// one tick of flight, bounces, rolling, curve, walls and the goal frame
void SimulateBall(BallState &ball);

// +1 if the whole ball is over the +x goal line, -1 for the -x one, else 0.
// the goal mouth is the only gap in the end walls, so over the line = goal
int GoalLineCrossed(const BallState &ball);

// pushes the ball out of a cylinder around segment a-b (posts, crossbar) and
// bounces it off. true if they touched
bool CollideWithBar(BallState &ball, Vec3 a, Vec3 b, float radius,
                    float restitution);

namespace kick {
constexpr float REACH = 0.45f;     // gap between us and the ball we can kick over
constexpr float MAX_HEIGHT = 1.0f; // ball centre higher than this is out of reach
constexpr float CONE = 0.25f;      // cos of the widest angle we can kick at, ~75deg
constexpr float MIN_SPEED = 5, MAX_SPEED = 24;       // ground shots, m/s
constexpr float CHIP_MIN_SPEED = 5, CHIP_MAX_SPEED = 15; // chips, along the grass
constexpr float CHIP_MIN_LIFT = 3, CHIP_MAX_LIFT = 8;    // chips, upwards
constexpr float MAX_SPIN = 8;      // rad/s at full curve
constexpr float CARRY = 0.3f;      // how much of our running speed the ball gets
constexpr uint8_t COOLDOWN = 15;   // ticks between kicks
constexpr float DRIBBLE_BOUNCE = 0.2f; // running into the ball, not kicking it
} // namespace kick

// kicks the ball where the player is facing, if it's in reach and in front of
// them. call after SimulateMovement for the same input
bool TryKick(PlayerState &player, const KickCommand &command, BallState &ball);

// players are cylinders the ball bounces off; running into it dribbles it.
// true if they touched
bool CollidePlayerBall(const PlayerState &player, BallState &ball);
