#include "Ball.h"
#include <cmath>

using namespace ball;

namespace {

Vec3 Horizontal(Vec3 v) { return {v.x, 0, v.z}; }

// bounces one velocity component off a wall: if it's heading into the wall,
// flip it and keep `restitution` of it
void Bounce(float &velocity, float intoWall, float restitution) {
  if (velocity * intoWall > 0) {
    velocity = -velocity * restitution;
  }
}

void CollideWithGoal(BallState &ball, const Vec3 &previous, float side) {
  const float r = BALL_RADIUS;
  float x = side * ball.pos.x;

  // the goal frame: two posts and a crossbar, all just cylinders
  float lineX = side * PITCH_HALF_LENGTH;
  for (float z : {-GOAL_HALF_WIDTH, GOAL_HALF_WIDTH}) {
    CollideWithBar(ball, {lineX, 0, z}, {lineX, GOAL_HEIGHT, z}, POST_RADIUS,
                   POST_BOUNCE);
  }
  CollideWithBar(ball, {lineX, GOAL_HEIGHT, -GOAL_HALF_WIDTH},
                 {lineX, GOAL_HEIGHT, GOAL_HALF_WIDTH}, POST_RADIUS,
                 POST_BOUNCE);

  if (side * previous.x > PITCH_HALF_LENGTH) {
    // already in the goal: the side nets, roof and back net hold it in
    float sideNet = GOAL_HALF_WIDTH - r;
    if (std::abs(ball.pos.z) > sideNet) {
      ball.pos.z = std::copysign(sideNet, ball.pos.z);
      Bounce(ball.velocity.z, ball.pos.z, NET_BOUNCE);
    }
    if (ball.pos.y > GOAL_HEIGHT - r) {
      ball.pos.y = GOAL_HEIGHT - r;
      Bounce(ball.velocity.y, 1, NET_BOUNCE);
    }
    float backNet = PITCH_HALF_LENGTH + GOAL_DEPTH - r;
    if (side * ball.pos.x > backNet) {
      ball.pos.x = side * backNet;
      Bounce(ball.velocity.x, side, NET_BOUNCE);
    }
  } else if (x > PITCH_HALF_LENGTH - r) {
    // reaching the end wall. the goal mouth is the one gap in it
    bool inMouth = std::abs(ball.pos.z) < GOAL_HALF_WIDTH &&
                   ball.pos.y < GOAL_HEIGHT;
    if (!inMouth) {
      ball.pos.x = side * (PITCH_HALF_LENGTH - r);
      Bounce(ball.velocity.x, side, WALL_BOUNCE);
    }
  }
}

} // namespace

bool OnGround(const BallState &ball) {
  return ball.pos.y <= BALL_RADIUS && ball.velocity.y == 0;
}

bool CollideWithBar(BallState &ball, Vec3 a, Vec3 b, float radius,
                    float restitution) {
  // closest point on the segment: project onto it, clamp to the ends
  Vec3 ab = b - a;
  float t = std::clamp(Dot(ball.pos - a, ab) / Dot(ab, ab), 0.0f, 1.0f);
  Vec3 closest = a + ab * t;
  Vec3 offset = ball.pos - closest;
  float distance = std::sqrt(Dot(offset, offset));
  float touching = BALL_RADIUS + radius;
  if (distance >= touching || distance == 0) {
    return false;
  }

  Vec3 normal = offset * (1 / distance);
  ball.pos = closest + normal * touching;
  // only the part of the velocity going into the bar bounces back
  float approach = Dot(ball.velocity, normal);
  if (approach < 0) {
    ball.velocity = ball.velocity - normal * ((1 + restitution) * approach);
  }
  return true;
}

void SimulateBall(BallState &ballState) {
  BallState &b = ballState;
  const float r = BALL_RADIUS;
  bool grounded = OnGround(b);
  Vec3 previous = b.pos;

  if (!grounded) {
    b.velocity.y -= GRAVITY * TICK_DT;
  }

  // sidespin pushes the ball sideways, at right angles to where it's going
  // (the magnus effect). a force at right angles changes direction but not
  // speed, so it just turns the velocity at a rate proportional to spin. we
  // turn it with the same yaw quaternion players use
  b.velocity = Rotate(Yaw(CURVE * b.spin * TICK_DT), b.velocity);

  // exp() rather than (1 - k*dt) so the slowdown doesn't depend on tick rate
  b.velocity = b.velocity * std::exp(-AIR_DRAG * TICK_DT);
  if (grounded) {
    Vec3 flat = Horizontal(b.velocity);
    float speed = std::sqrt(Dot(flat, flat));
    if (speed > 0) {
      float slower = std::max(0.0f, speed - ROLL_FRICTION * TICK_DT) *
                     std::exp(-ROLL_DRAG * TICK_DT);
      b.velocity = flat * (slower / speed);
    }
  }
  b.spin *= std::exp(-(grounded ? SPIN_DECAY_GROUND : SPIN_DECAY_AIR) *
                     TICK_DT);

  // semi-implicit euler, like players: new velocity, then move with it
  b.pos = b.pos + b.velocity * TICK_DT;

  if (b.pos.y < r) {
    b.pos.y = r;
    if (b.velocity.y < 0) {
      b.velocity.y = -b.velocity.y * BOUNCE;
      b.velocity.x *= GRASS_GRIP;
      b.velocity.z *= GRASS_GRIP;
      if (b.velocity.y < SETTLE_SPEED) {
        b.velocity.y = 0;
      }
    }
  }

  // side walls
  float sideWall = PITCH_HALF_WIDTH - r;
  if (std::abs(b.pos.z) > sideWall) {
    b.pos.z = std::copysign(sideWall, b.pos.z);
    Bounce(b.velocity.z, b.pos.z, WALL_BOUNCE);
  }

  CollideWithGoal(b, previous, 1);
  CollideWithGoal(b, previous, -1);
}

bool TryKick(PlayerState &player, const KickCommand &command, BallState &b) {
  if (command.type == KickType::None || player.kickCooldown > 0) {
    return false;
  }

  Vec3 toBall = Horizontal(b.pos - player.pos);
  float distance = std::sqrt(Dot(toBall, toBall));
  if (distance > PLAYER_RADIUS + BALL_RADIUS + kick::REACH ||
      b.pos.y > kick::MAX_HEIGHT) {
    return false;
  }
  // the dot product of two unit vectors is the cosine of the angle between
  // them, so this is "is the ball within ~75 degrees of where we're facing"
  Vec3 forward = Rotate(player.orientation, {0, 0, 1});
  if (distance > 0 && Dot(forward, toBall * (1 / distance)) < kick::CONE) {
    return false;
  }

  float power = command.power / 255.0f;
  auto lerp = [power](float a, float b) { return a + (b - a) * power; };
  if (command.type == KickType::Chip) {
    b.velocity = forward * lerp(kick::CHIP_MIN_SPEED, kick::CHIP_MAX_SPEED) +
                 Vec3{0, lerp(kick::CHIP_MIN_LIFT, kick::CHIP_MAX_LIFT), 0};
  } else {
    b.velocity = forward * lerp(kick::MIN_SPEED, kick::MAX_SPEED);
  }
  b.velocity = b.velocity + Horizontal(player.velocity) * kick::CARRY;
  b.spin = command.curve / 127.0f * kick::MAX_SPIN;
  player.kickCooldown = kick::COOLDOWN;
  return true;
}

bool CollidePlayerBall(const PlayerState &player, BallState &b) {
  if (b.pos.y - BALL_RADIUS > PLAYER_HEIGHT) {
    return false; // over their head
  }
  Vec3 offset = Horizontal(b.pos - player.pos);
  float distance = std::sqrt(Dot(offset, offset));
  float touching = PLAYER_RADIUS + BALL_RADIUS;
  if (distance >= touching) {
    return false;
  }

  Vec3 normal = distance > 0 ? offset * (1 / distance)
                             : Rotate(player.orientation, {0, 0, 1});
  b.pos.x = player.pos.x + normal.x * touching;
  b.pos.z = player.pos.z + normal.z * touching;

  // a player is way heavier than the ball, so they're a moving wall: the
  // ball keeps its speed relative to them, flipped and mostly absorbed
  Vec3 relative = Horizontal(b.velocity - player.velocity);
  float approach = Dot(relative, normal);
  if (approach < 0) {
    b.velocity = b.velocity - normal * ((1 + kick::DRIBBLE_BOUNCE) * approach);
    b.spin = 0;
  }
  return true;
}

int GoalLineCrossed(const BallState &ball) {
  if (ball.pos.x > PITCH_HALF_LENGTH + BALL_RADIUS) {
    return 1;
  }
  if (ball.pos.x < -PITCH_HALF_LENGTH - BALL_RADIUS) {
    return -1;
  }
  return 0;
}
