#include "Ball.h"
#include "ServerWorld.h"
#include <cassert>
#include <iostream>
#include <optional>

// the last event of a type. by value, since TakeEvents() hands back a temporary
std::optional<GameEvent> Find(const std::vector<GameEvent> &events, EventType type) {
  std::optional<GameEvent> found;
  for (const auto &event : events)
    if (event.type == type)
      found = event;
  return found;
}

void test_teams_are_balanced() {
  ServerWorld world;
  for (ClientID id = 0; id < 4; id++)
    assert(world.AddPlayer(id));
  assert(world.players.at(0).state.team == 0);
  assert(world.players.at(1).state.team == 1);
  assert(world.players.at(2).state.team == 0);
  assert(world.players.at(3).state.team == 1);
  // everyone starts in their own half, facing the other goal
  for (const auto &[id, player] : world.players) {
    float side = player.state.team == 0 ? -1 : 1;
    assert(player.state.pos.x * side > 0);
    Vec3 forward = Rotate(player.state.orientation, {0, 0, 1});
    assert(forward.x * side < -0.99f);
  }
  auto events = world.TakeEvents();
  assert(events.size() == 4 && events[1].type == EventType::PlayerJoined);
  assert(events[1].player == 1 && events[1].team == 1);
  assert(world.TakeEvents().empty());

  // someone leaves orange, so the next one fills that gap
  world.RemovePlayer(1);
  assert(!world.players.contains(1));
  assert(Find(world.TakeEvents(), EventType::PlayerLeft)->player == 1);
  world.AddPlayer(4);
  assert(world.players.at(4).state.team == 1);
}

void test_goal_then_kickoff() {
  ServerWorld world;
  world.AddPlayer(0); // blue, attacking +x
  world.AddPlayer(1);
  world.TakeEvents();
  world.lastTouch = 0;
  world.ball = {{PITCH_HALF_LENGTH - 2, BALL_RADIUS, 0}, {10, 0, 0}, 0};

  int ticks = 0;
  while (world.match.phase == MatchPhase::Playing && ticks++ < 60)
    world.Tick();
  assert(world.match.phase == MatchPhase::Goal);
  assert(world.match.score[0] == 1 && world.match.score[1] == 0);
  auto events = world.TakeEvents();
  auto goal = Find(events, EventType::Goal);
  assert(goal && goal->player == 0 && goal->team == 0 && goal->value == 0);
  assert(goal->score[0] == 1);

  // the ball's still in the net during the pause: that's not another goal
  for (int i = 0; i < GOAL_PAUSE_TICKS - 1; i++)
    world.Tick();
  assert(world.match.score[0] == 1 && !Find(world.TakeEvents(), EventType::Goal));
  assert(world.match.phase == MatchPhase::Goal);

  world.players.at(0).state.pos = {5, 0, 5};
  world.Tick();
  assert(world.match.phase == MatchPhase::Playing);
  assert(world.ball.pos.x == 0 && world.ball.pos.z == 0);
  assert(world.players.at(0).state.pos.x < 0); // back in their own half
  assert(world.lastTouch == NO_PLAYER);
}

void test_own_goals_count_for_the_other_team() {
  ServerWorld world;
  world.AddPlayer(0); // blue, defending -x
  world.lastTouch = 0;
  world.ball = {{-PITCH_HALF_LENGTH + 2, BALL_RADIUS, 1}, {-10, 0, 0}, 0};
  for (int i = 0; i < 60 && world.match.phase == MatchPhase::Playing; i++)
    world.Tick();
  assert(world.match.score[1] == 1 && world.match.score[0] == 0);
  auto goal = Find(world.TakeEvents(), EventType::Goal);
  assert(goal && goal->team == 1 && goal->value == 1);
}

// blue (0) shoots at the +x goal, and it goes in off orange (1)
std::optional<GameEvent> DeflectedIn(uint32_t ticksSinceShot) {
  ServerWorld world;
  world.AddPlayer(0);
  world.AddPlayer(1);
  for (uint32_t i = 0; i < ticksSinceShot + 1; i++)
    world.Tick();
  world.lastShot = 0;
  world.lastShotTick = world.CurrentTick() - ticksSinceShot;
  world.lastTouch = 1;
  world.ball = {{PITCH_HALF_LENGTH - 1, BALL_RADIUS, 2}, {10, 0, 0}, 0};
  for (int i = 0; i < 30 && world.match.phase == MatchPhase::Playing; i++)
    world.Tick();
  return Find(world.TakeEvents(), EventType::Goal);
}

void test_deflections_count_for_the_shooter() {
  auto deflected = DeflectedIn(20);
  assert(deflected && deflected->player == 0 && deflected->team == 0);
  assert(deflected->value == 0); // not an own goal
  // ages after the shot, it's on the defender
  auto later = DeflectedIn(300);
  assert(later && later->player == 1 && later->value == 1);
}

void test_kicks_and_touches_are_credited() {
  ServerWorld world;
  world.AddPlayer(0);
  PlayerState &state = world.players.at(0).state;
  state.pos = {0, 0, -0.9f};
  world.ball = BallState{};
  // facing +z (yaw 0) at the ball, full power
  assert(world.QueueInputs(0, {{{1, {}, 0, {KickType::Ground, 255, 0}}}}));
  world.Tick();
  assert(world.lastTouch == 0 && world.lastShot == 0);
  assert(world.ball.velocity.z > kick::MAX_SPEED * 0.9f);
}

void test_full_time_then_a_new_match() {
  ServerWorld world;
  world.AddPlayer(0);
  world.match.score = {2, 1};
  world.match.clock = 3;
  for (int i = 0; i < 3; i++)
    world.Tick();
  assert(world.match.phase == MatchPhase::FullTime && world.match.clock == 0);
  auto result = Find(world.TakeEvents(), EventType::FullTime);
  assert(result && result->team == 0 && result->value == 0);
  assert(result->score[0] == 2 && result->score[1] == 1);

  // no goals while the result's up
  world.ball = {{PITCH_HALF_LENGTH - 1, BALL_RADIUS, 0}, {10, 0, 0}, 0};
  for (int i = 0; i < FULL_TIME_PAUSE_TICKS; i++)
    world.Tick();
  assert(world.match.phase == MatchPhase::Playing);
  assert(world.match.score[0] == 0 && world.match.score[1] == 0);
  assert(world.match.clock == MATCH_TICKS);
}

int main() {
  test_teams_are_balanced();
  test_goal_then_kickoff();
  test_own_goals_count_for_the_other_team();
  test_deflections_count_for_the_shooter();
  test_kicks_and_touches_are_credited();
  test_full_time_then_a_new_match();
  std::cout << "All match tests passed!\n";
}
