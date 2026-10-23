#include "Ball.h"
#include "Goalkeeper.h"
#include "Movement.h"
#include "ServerWorld.h"
#include <cassert>
#include <iostream>
#include <numbers>
#include <random>

float Length(Vec3 v) { return std::sqrt(Dot(v, v)); }
float Angle(Vec3 a, Vec3 b) {
  return std::acos(std::clamp(Dot(Normalize(a), Normalize(b)), -1.0f, 1.0f));
}
BallState Resting(float x, float z) { return {{x, BALL_RADIUS, z}, {}, 0}; }

void test_stands_on_the_angle_bisector() {
  // team 0 defends the -x goal
  float line = OwnGoalLine(0);
  Vec3 centre = KeeperTarget(0, Resting(0, 0));
  assert(std::abs(centre.z) < 0.0001f);
  assert(std::abs(centre.x - (line + keeper::LINE_DEPTH)) < 0.0001f);

  // from out wide, the keeper shifts over so both posts are the same angle away
  for (Vec3 ball : {Vec3{-10, 0, 8}, Vec3{-14, 0, -6}, Vec3{-5, 0, 2}}) {
    Vec3 target = KeeperTarget(0, Resting(ball.x, ball.z));
    Vec3 toKeeper = target - ball;
    float left = Angle(Vec3{line, 0, -GOAL_HALF_WIDTH} - ball, toKeeper);
    float right = Angle(Vec3{line, 0, GOAL_HALF_WIDTH} - ball, toKeeper);
    assert(std::abs(left - right) < 0.001f);
    assert(target.z * ball.z > 0); // towards the ball's side
  }
  // the other team's keeper mirrors it
  Vec3 mirrored = KeeperTarget(1, Resting(10, 8));
  Vec3 original = KeeperTarget(0, Resting(-10, 8));
  assert(std::abs(mirrored.x + original.x) < 0.0001f);
  assert(std::abs(mirrored.z - original.z) < 0.0001f);
}

// where a shot really crosses the keeper's line
float Crossing(BallState shot) {
  float keeperX = OwnGoalLine(0) + keeper::LINE_DEPTH;
  while (shot.pos.x > keeperX)
    SimulateBall(shot);
  return shot.pos.z;
}

void test_reads_the_line_but_not_the_spin() {
  // a driven shot, with a bounce on the way: the keeper's spot on its line is
  // exactly where the physics says it'll be
  BallState driven{{-8, 1.2f, 1}, {-16, -2, 1.2f}, 0};
  assert(std::abs(KeeperTarget(0, driven).z - Crossing(driven)) < 0.0001f);

  // a shot from the right that looks like it's going wide, but the spin
  // curls it back inside the post. the keeper goes where it looked like it
  // was going, not where it ends up
  BallState curler{{-6, BALL_RADIUS, 5.5f}, {-16, 0, -2.5f}, -8};
  BallState unspun = curler;
  unspun.spin = 0;
  assert(Crossing(unspun) > GOAL_HALF_WIDTH && Crossing(curler) < GOAL_HALF_WIDTH);
  assert(std::abs(KeeperTarget(0, curler).z - Crossing(unspun)) < 0.0001f);
}

void test_arrives_without_wobbling() {
  PlayerState gk{1, {OwnGoalLine(0) + keeper::LINE_DEPTH, 0, -3}};
  gk.keeper = true;
  // ball out on the right, so the target is over to that side
  BallState ball = Resting(-12, 9);
  Vec3 target = KeeperTarget(0, ball);
  float startGap = Length(target - gk.pos), worstOvershoot = 0;
  for (int i = 0; i < 120; i++) {
    InputCommand input = KeeperInput(gk, ball);
    assert(ValidInput(input));
    SimulateMovement(gk, input);
    worstOvershoot = std::max(worstOvershoot, gk.pos.z - target.z);
  }
  assert(startGap > 2);
  assert(Length(target - gk.pos) < 0.15f);
  assert(worstOvershoot < 0.3f);
  // and it's looking at the ball
  Vec3 facing = Rotate(gk.orientation, {0, 0, 1});
  assert(Angle(facing, ball.pos - gk.pos) < 0.05f);
}

enum class Result { Saved, Scored, Missed };

// a shot at the +x goal (orange's keeper, team 1) with nobody else around.
// the ball sits still for a second first, so the keeper is set and its
// delayed view of the ball starts from before the kick, like a real shot
Result Shoot(BallState shot) {
  ServerWorld world;
  world.AddKeepers();
  world.ball = {shot.pos, {}, 0};
  for (int i = 0; i < 60; i++)
    world.Tick();
  world.ball = shot;
  for (int i = 0; i < 180; i++) {
    world.Tick();
    if (world.lastTouch == KEEPER_IDS[1])
      return Result::Saved;
    if (world.match.phase == MatchPhase::Goal)
      return Result::Scored;
  }
  return Result::Missed; // wide, over, or off the woodwork
}

void test_saves_the_easy_ones_not_the_corners() {
  // straight at it from distance
  assert(Shoot({{8, BALL_RADIUS, 0}, {14, 0, 0}, 0}) == Result::Saved);
  // into the far corner from close in, as hard as possible
  Vec3 corner = Normalize(Vec3{PITCH_HALF_LENGTH - 13, 0, 2.6f - 0});
  assert(Shoot({{13, BALL_RADIUS, 0}, corner * kick::MAX_SPEED, 0}) == Result::Scored);
}

// kicks a ball from `from` towards `aim` the way a player would, and returns
// how the keeper got on
Result Strike(Vec3 from, Vec3 aim, KickCommand kick) {
  PlayerState shooter{1};
  shooter.orientation = Yaw(YawTowards(aim - from));
  shooter.pos = from - Rotate(shooter.orientation, {0, 0, 0.9f});
  shooter.pos.y = 0;
  BallState shot{from, {}, 0};
  assert(TryKick(shooter, kick, shot));
  return Shoot(shot);
}

struct Gallery {
  int results[3] = {};
  void Add(Result r) { results[static_cast<int>(r)]++; }
  float SaveRate() const {
    return 100.0f * results[0] / std::max(1, results[0] + results[1]);
  }
  void Print(const char *name) const {
    std::cout << name << ": " << results[1] << " scored, " << results[0]
              << " saved, " << results[2] << " missed (keeper saves "
              << static_cast<int>(SaveRate()) << "% on target)\n";
  }
};

void test_shooting_gallery() {
  // seeded shots from around the box, measured rather than guessed
  std::mt19937 rng(7);
  std::uniform_real_distribution<float> unit(0, 1);
  auto from = [&](float near, float far) {
    float distance = near + unit(rng) * (far - near);
    float angle = (unit(rng) - 0.5f) * 1.4f;
    return Vec3{PITCH_HALF_LENGTH - distance * std::cos(angle), BALL_RADIUS,
                distance * std::sin(angle)};
  };
  float corner = GOAL_HALF_WIDTH - 0.45f;

  // anything goes: any power, anywhere in the goal, any curve
  Gallery random;
  for (int i = 0; i < 200; i++) {
    Vec3 aim{PITCH_HALF_LENGTH, 0, (unit(rng) * 2 - 1) * corner};
    KickType type = unit(rng) < 0.2f ? KickType::Chip : KickType::Ground;
    random.Add(Strike(from(6, 20), aim,
                      {type, static_cast<uint8_t>(120 + unit(rng) * 135),
                       static_cast<int8_t>((unit(rng) * 2 - 1) * 60)}));
  }
  // what you'd actually try: full power, into a corner
  Gallery placed, longRange;
  for (int i = 0; i < 100; i++) {
    Vec3 aim{PITCH_HALF_LENGTH, 0, (i % 2 ? 1 : -1) * corner};
    placed.Add(Strike(from(7, 13), aim, {KickType::Ground, 255, 0}));
    longRange.Add(Strike(from(13, 20), aim, {KickType::Ground, 255, 0}));
  }
  // a tame one at the middle of the goal
  Gallery tame;
  for (int i = 0; i < 100; i++) {
    Vec3 aim{PITCH_HALF_LENGTH, 0, (unit(rng) * 2 - 1) * 1.0f};
    tame.Add(Strike(from(10, 18), aim, {KickType::Ground, 140, 0}));
  }

  random.Print("200 random shots");
  placed.Print("100 driven into the corners from 7-13m");
  longRange.Print("100 driven into the corners from 13-20m");
  tame.Print("100 tame ones down the middle");
  // beatable with a good shot, not with a bad one, and a coin flip from range
  assert(placed.SaveRate() < 50);
  assert(longRange.SaveRate() > 25 && longRange.SaveRate() < 80);
  assert(tame.SaveRate() > 85);
  assert(random.SaveRate() > placed.SaveRate() && random.SaveRate() < tame.SaveRate());
}

void test_stays_in_its_box() {
  ServerWorld world;
  world.AddKeepers();
  std::mt19937 rng(3);
  std::uniform_real_distribution<float> x(-18, 18), z(-10, 10), v(-20, 20);
  for (int i = 0; i < 3000; i++) {
    if (i % 90 == 0)
      world.ball = {{x(rng), BALL_RADIUS, z(rng)}, {v(rng), 0, v(rng)}, 0};
    world.Tick();
    for (PlayerID id : KEEPER_IDS) {
      const PlayerState &gk = world.players.at(id).state;
      float depth = (gk.pos.x - OwnGoalLine(gk.team)) * (gk.team == 0 ? 1 : -1);
      assert(depth > 0 && depth < keeper::MAX_DEPTH + 0.5f);
      assert(std::abs(gk.pos.z) < GOAL_HALF_WIDTH + 1.5f);
    }
  }
}

int main() {
  test_stands_on_the_angle_bisector();
  test_reads_the_line_but_not_the_spin();
  test_arrives_without_wobbling();
  test_saves_the_easy_ones_not_the_corners();
  test_shooting_gallery();
  test_stays_in_its_box();
  std::cout << "All keeper tests passed!\n";
}
