#include "Ball.h"
#include "Movement.h"
#include <cassert>
#include <iostream>
#include <numbers>

float Speed(Vec3 v) { return std::sqrt(Dot(v, v)); }

// a player at the origin facing +z, ball just in front
PlayerState Player() { return {1, {0, 0, 0}, {}, Yaw(0)}; }
BallState BallAt(float x, float z) { return {{x, BALL_RADIUS, z}, {}, 0}; }

void test_kicks_go_where_we_face() {
  PlayerState player = Player();
  BallState b = BallAt(0, 0.9f);
  assert(TryKick(player, {KickType::Ground, 255, 0}, b));
  assert(std::abs(b.velocity.z - kick::MAX_SPEED) < 0.0001f);
  assert(b.velocity.x == 0 && b.velocity.y == 0 && b.spin == 0);
  assert(player.kickCooldown == kick::COOLDOWN);

  // facing right (+x) instead
  player = Player();
  player.orientation = Yaw(std::numbers::pi_v<float> / 2);
  b = BallAt(0.9f, 0);
  assert(TryKick(player, {KickType::Ground, 0, 0}, b));
  assert(std::abs(b.velocity.x - kick::MIN_SPEED) < 0.0001f);
  assert(std::abs(b.velocity.z) < 0.0001f);
}

void test_power_chip_and_curve() {
  PlayerState player = Player();
  BallState soft = BallAt(0, 0.9f), hard = soft, chip = soft, bent = soft;
  TryKick(player, {KickType::Ground, 60, 0}, soft);
  player.kickCooldown = 0;
  TryKick(player, {KickType::Ground, 240, 0}, hard);
  assert(Speed(hard.velocity) > Speed(soft.velocity));

  player.kickCooldown = 0;
  TryKick(player, {KickType::Chip, 255, 0}, chip);
  assert(chip.velocity.y == kick::CHIP_MAX_LIFT);
  SimulateBall(chip);
  assert(!OnGround(chip));

  player.kickCooldown = 0;
  TryKick(player, {KickType::Ground, 255, -127}, bent);
  assert(bent.spin == -kick::MAX_SPIN);
}

void test_out_of_reach() {
  PlayerState player = Player();
  float reach = PLAYER_RADIUS + BALL_RADIUS + kick::REACH;
  BallState far = BallAt(0, reach + 0.01f);
  BallState behind = BallAt(0, -0.9f);
  BallState high = BallAt(0, 0.9f);
  high.pos.y = kick::MAX_HEIGHT + 0.1f;
  for (BallState *b : {&far, &behind, &high}) {
    assert(!TryKick(player, {KickType::Ground, 255, 0}, *b));
    assert(Speed(b->velocity) == 0);
  }
  // no kick, or still cooling down from the last one
  BallState close = BallAt(0, 0.9f);
  assert(!TryKick(player, {}, close));
  player.kickCooldown = 1;
  assert(!TryKick(player, {KickType::Ground, 255, 0}, close));
  // right at the edge of the cone still counts (~75 degrees)
  float angle = 70 * std::numbers::pi_v<float> / 180;
  BallState wide = BallAt(std::sin(angle) * 0.9f, std::cos(angle) * 0.9f);
  player.kickCooldown = 0;
  assert(TryKick(player, {KickType::Ground, 255, 0}, wide));
}

void test_running_into_the_ball_dribbles_it() {
  PlayerState player = Player();
  BallState b = BallAt(0, 3);
  bool touched = false;
  for (uint32_t i = 1; i <= 120; i++) {
    SimulateMovement(player, {i, {0, 0, 1}, 0});
    SimulateBall(b);
    touched = CollidePlayerBall(player, b) || touched;
    // never inside the player
    Vec3 gap{b.pos.x - player.pos.x, 0, b.pos.z - player.pos.z};
    assert(Speed(gap) >= PLAYER_RADIUS + BALL_RADIUS - 0.0001f);
  }
  assert(touched);
  // pushed along ahead of us the whole way
  assert(b.pos.z > player.pos.z && std::abs(b.pos.x) < 0.0001f);
  assert(b.velocity.z > 0);
}

void test_ball_bounces_off_a_standing_player() {
  PlayerState player = Player();
  BallState b{{0, BALL_RADIUS, 0.75f}, {0, 0, -10}, 5};
  assert(CollidePlayerBall(player, b));
  assert(b.velocity.z == 10 * kick::DRIBBLE_BOUNCE);
  assert(b.spin == 0);

  BallState over{{0, PLAYER_HEIGHT + BALL_RADIUS + 0.1f, 0}, {0, 0, -10}, 0};
  assert(!CollidePlayerBall(player, over));
}

int main() {
  test_kicks_go_where_we_face();
  test_power_chip_and_curve();
  test_out_of_reach();
  test_running_into_the_ball_dribbles_it();
  test_ball_bounces_off_a_standing_player();
  std::cout << "All kick tests passed!\n";
}
