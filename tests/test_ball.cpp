#include "Ball.h"
#include <cassert>
#include <iostream>

float Speed(Vec3 v) { return std::sqrt(Dot(v, v)); }

BallState Ball(Vec3 pos, Vec3 velocity, float spin = 0) {
  return {pos, velocity, spin};
}

void test_resting_ball_stays_put() {
  BallState b;
  for (int i = 0; i < 600; i++) {
    SimulateBall(b);
  }
  assert(b.pos.x == 0 && b.pos.y == BALL_RADIUS && b.pos.z == 0);
  assert(Speed(b.velocity) == 0 && OnGround(b));
}

void test_flight_matches_projectile_motion() {
  // straight up at v0. with drag k, dv/dt = -g - kv, which solves to
  //   time to the top: t = ln(1 + k v0 / g) / k
  //   height at the top: h = v0 / k - (g / k^2) ln(1 + k v0 / g)
  // (without drag that's the usual v0 / g and v0^2 / 2g)
  float v0 = 8, k = ball::AIR_DRAG, g = ball::GRAVITY;
  BallState b = Ball({0, BALL_RADIUS, 0}, {0, v0, 0});
  float peak = 0;
  int ticks = 0;
  while (b.velocity.y > 0) {
    SimulateBall(b);
    peak = std::max(peak, b.pos.y - BALL_RADIUS);
    ticks++;
  }
  float log = std::log(1 + k * v0 / g);
  float analyticTime = log / k;
  float analyticPeak = v0 / k - g / (k * k) * log;
  assert(peak < v0 * v0 / (2 * g)); // drag only takes height away
  // stepping in ticks costs about v0 * dt / 2 of height, and up to a tick
  assert(std::abs(peak - analyticPeak) < v0 * TICK_DT);
  assert(std::abs(ticks * TICK_DT - analyticTime) < 2 * TICK_DT);
}

void test_bounces_get_lower_then_it_rolls() {
  BallState b = Ball({0, 2, 0}, {3, 0, 0});
  float lastPeak = 2;
  float peak = 0;
  int bounces = 0;
  for (int i = 0; i < 600 && !OnGround(b); i++) {
    float before = b.velocity.y;
    SimulateBall(b);
    peak = std::max(peak, b.pos.y);
    if (before < 0 && b.velocity.y >= 0) {
      assert(peak <= lastPeak);
      lastPeak = peak;
      peak = 0;
      bounces++;
    }
  }
  assert(bounces >= 2);
  assert(OnGround(b) && b.velocity.x > 0); // still rolling forward
}

void test_rolling_ball_stops() {
  // constant friction alone would stop it in v^2 / 2a; drag stops it sooner
  BallState b = Ball({-15, BALL_RADIUS, 0}, {10, 0, 0});
  int ticks = 0;
  while (Speed(b.velocity) > 0 && ticks < 1200) {
    SimulateBall(b);
    ticks++;
  }
  float distance = b.pos.x + 15;
  assert(Speed(b.velocity) == 0);
  assert(distance < 10 * 10 / (2 * ball::ROLL_FRICTION));
  assert(distance > 10);
}

void test_spin_curves_without_changing_speed() {
  Vec3 start{-10, 1.5f, 0};
  Vec3 kick{0, 0, 15};
  BallState plain = Ball(start, kick);
  BallState right = Ball(start, kick, 8);
  BallState left = Ball(start, kick, -8);
  for (int i = 0; i < 20; i++) {
    SimulateBall(plain);
    SimulateBall(right);
    SimulateBall(left);
  }
  assert(right.pos.x > plain.pos.x + 0.2f);
  assert(left.pos.x < plain.pos.x - 0.2f);
  assert(std::abs((right.pos.x - plain.pos.x) + (left.pos.x - plain.pos.x)) <
         0.0001f); // mirror images
  // turning the velocity doesn't speed it up or slow it down
  assert(std::abs(Speed(right.velocity) - Speed(plain.velocity)) < 0.0001f);
}

void test_side_walls_bounce() {
  BallState b = Ball({0, BALL_RADIUS, PITCH_HALF_WIDTH - 1}, {0, 0, 10});
  for (int i = 0; i < 30; i++) {
    SimulateBall(b);
    assert(b.pos.z <= PITCH_HALF_WIDTH - BALL_RADIUS);
  }
  assert(b.velocity.z < 0);
}

int Shoot(BallState b, int ticks = 120) {
  for (int i = 0; i < ticks; i++) {
    SimulateBall(b);
    if (int side = GoalLineCrossed(b)) {
      return side;
    }
  }
  return 0;
}

void test_goals_need_the_whole_ball_in_the_mouth() {
  float lineX = PITCH_HALF_LENGTH;
  // straight down the middle, both ends
  assert(Shoot(Ball({lineX - 5, BALL_RADIUS, 0}, {15, 0, 0})) == 1);
  assert(Shoot(Ball({-lineX + 5, BALL_RADIUS, 0}, {-15, 0, 0})) == -1);
  // wide of the post, or over the bar: the end wall sends it back
  BallState wide = Ball({lineX - 5, BALL_RADIUS, 5}, {15, 0, 0});
  assert(Shoot(wide) == 0);
  BallState high = Ball({lineX - 5, 3.5f, 0}, {15, 0.5f, 0});
  assert(Shoot(high, 30) == 0);
  for (int i = 0; i < 30; i++) {
    SimulateBall(wide);
  }
  assert(wide.velocity.x < 0 && wide.pos.x < lineX);
}

void test_posts_and_crossbar_deflect() {
  float lineX = PITCH_HALF_LENGTH;
  // dead centre on the post: comes straight back out
  BallState post = Ball({lineX - 3, BALL_RADIUS, GOAL_HALF_WIDTH}, {12, 0, 0});
  bool hit = false;
  for (int i = 0; i < 30 && !hit; i++) {
    SimulateBall(post);
    hit = post.velocity.x < 0;
  }
  assert(hit && GoalLineCrossed(post) == 0);

  // clipping the inside of the post: deflected in
  assert(Shoot(Ball({lineX - 3, BALL_RADIUS, GOAL_HALF_WIDTH - 0.25f},
                    {12, 0, 0})) == 1);

  // dead centre on the crossbar comes back out...
  BallState bar = Ball({lineX - 1, GOAL_HEIGHT, 0}, {12, 0, 0});
  for (int i = 0; i < 30; i++) {
    SimulateBall(bar);
  }
  assert(bar.velocity.x < 0 && GoalLineCrossed(bar) == 0);
  // ...but off the underside it goes down and in
  assert(Shoot(Ball({lineX - 1, GOAL_HEIGHT - 0.35f, 0}, {12, 0, 0})) == 1);
}

void test_the_net_holds_the_ball() {
  // a fast shot into the corner, then plenty of time to try and escape
  BallState b = Ball({PITCH_HALF_LENGTH - 4, 1, 1.5f}, {22, 1, 4}, 6);
  bool scored = false;
  for (int i = 0; i < 300; i++) {
    SimulateBall(b);
    scored = scored || GoalLineCrossed(b) == 1;
    if (b.pos.x > PITCH_HALF_LENGTH) {
      assert(std::abs(b.pos.z) <= GOAL_HALF_WIDTH - BALL_RADIUS);
      assert(b.pos.y <= GOAL_HEIGHT - BALL_RADIUS);
      assert(b.pos.x <= PITCH_HALF_LENGTH + GOAL_DEPTH - BALL_RADIUS);
    }
  }
  assert(scored);
}

void test_bar_collision_is_closest_point_on_segment() {
  // past the end of the segment, the end point is what we bump into
  BallState b = Ball({0, 0, 2.3f}, {0, 0, -1});
  assert(CollideWithBar(b, {0, 0, -2}, {0, 0, 2}, 0.1f, 0.5f));
  assert(std::abs(b.pos.z - 2.4f) < 0.0001f);
  assert(std::abs(b.velocity.z - 0.5f) < 0.0001f);
  // moving away already: pushed out, but not bounced
  BallState away = Ball({0, 0.35f, 0}, {0, 1, 0});
  assert(CollideWithBar(away, {0, 0, -2}, {0, 0, 2}, 0.1f, 0.5f));
  assert(std::abs(away.pos.y - 0.4f) < 0.0001f && away.velocity.y == 1);
  BallState clear = Ball({0, 1, 0}, {});
  assert(!CollideWithBar(clear, {0, 0, -2}, {0, 0, 2}, 0.1f, 0.5f));
}

int main() {
  test_resting_ball_stays_put();
  test_flight_matches_projectile_motion();
  test_bounces_get_lower_then_it_rolls();
  test_rolling_ball_stops();
  test_spin_curves_without_changing_speed();
  test_side_walls_bounce();
  test_goals_need_the_whole_ball_in_the_mouth();
  test_posts_and_crossbar_deflect();
  test_the_net_holds_the_ball();
  test_bar_collision_is_closest_point_on_segment();
  std::cout << "All ball tests passed!\n";
}
