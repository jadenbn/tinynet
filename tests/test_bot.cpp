#include "Bot.h"
#include "Movement.h"
#include "ServerWorld.h"
#include <cassert>
#include <functional>
#include <iostream>

// bots straight into a ServerWorld, no network: they see the true state
void Run(ServerWorld &world, const std::vector<PlayerID> &bots, int ticks,
         const std::function<bool()> &done) {
  std::unordered_map<PlayerID, uint32_t> sequence;
  for (int tick = 0; tick < ticks && !done(); tick++) {
    for (PlayerID id : bots) {
      const PlayerState &me = world.players.at(id).state;
      std::vector<PlayerState> others;
      for (const auto &[otherID, other] : world.players)
        if (otherID != id)
          others.push_back(other.state);
      BotDecision decision = BotThink(me, world.ball, others, tick);
      Vec3 aim = decision.aim - me.pos;
      float yaw = Dot(aim, aim) > 0.01f ? YawTowards(aim) : 0;
      InputCommand input{++sequence[id], RelativeDirection(Normalize(decision.move), yaw),
                         yaw, decision.kick};
      assert(ValidInput(input));
      world.QueueInputs(id, {{input}});
    }
    world.Tick();
  }
}

float Length(Vec3 v) { return std::sqrt(Dot(v, v)); }

void test_scores_into_an_empty_net() {
  ServerWorld world;
  world.AddPlayer(0);
  Run(world, {0}, 600, [&] { return world.match.score[0] > 0; });
  assert(world.match.score[0] == 1 && world.match.score[1] == 0);
}

void test_beats_a_keeper_eventually() {
  ServerWorld world;
  world.AddKeepers();
  world.AddPlayer(0);
  Run(world, {0}, 60 * 60, [&] { return world.match.score[0] > 0; });
  assert(world.match.score[0] >= 1);
}

void test_gets_the_ball_out_of_corners() {
  for (Vec3 corner : {Vec3{-19.6f, BALL_RADIUS, 11.6f}, Vec3{19.6f, BALL_RADIUS, -11.6f},
                      Vec3{5, BALL_RADIUS, 11.6f}}) {
    ServerWorld world;
    world.AddPlayer(0);
    world.ball = {corner, {}, 0};
    world.players.at(0).state.pos = {corner.x * 0.9f, 0, corner.z * 0.85f};
    Run(world, {0}, 600, [&] {
      return Length(world.ball.pos - corner) > 4;
    });
    assert(Length(world.ball.pos - corner) > 4);
  }
}

void test_teammates_dont_both_chase() {
  ServerWorld world;
  world.AddPlayer(0);
  world.AddPlayer(1); // orange
  world.AddPlayer(2); // blue again
  // 0 is standing on the ball (and not moving: no inputs), 2 is a bot
  world.players.at(0).state.pos = {-1, 0, 0};
  world.players.at(2).state.pos = {-10, 0, 6};
  float closest = 100;
  Run(world, {2}, 180, [&] {
    closest = std::min(closest, Length(world.ball.pos - world.players.at(2).state.pos));
    return false;
  });
  // it hangs back for the rebound instead of piling in
  assert(closest > 3);
}

int main() {
  test_scores_into_an_empty_net();
  test_beats_a_keeper_eventually();
  test_gets_the_ball_out_of_corners();
  test_teammates_dont_both_chase();
  std::cout << "All bot tests passed!\n";
}
