#include "ClientSimulation.h"
#include "ServerWorld.h"
#include <cassert>
#include <iostream>
#include <numbers>

int main() {
  PlayerState player;
  SimulateMovement(player, {1, {0,0,1}, std::numbers::pi_v<float>/2});
  assert(player.pos.x > 0 && std::abs(player.pos.z) < 0.00001f);
  for (int i = 0; i < 600; ++i)
    SimulateMovement(player, {2, {0,0,1}, 0});
  assert(Dot(player.velocity, player.velocity) <= 36);
  float previousSpeed = Dot(player.velocity, player.velocity);
  SimulateMovement(player, {3, {}, 0});
  assert(Dot(player.velocity, player.velocity) < previousSpeed);
  for (int i = 0; i < 60; ++i)
    SimulateMovement(player, {4, {}, 0});
  assert(Dot(player.velocity, player.velocity) == 0);
  player.pos.z = WORLD_BOUND;
  SimulateMovement(player, {5, {0,0,1}, 0});
  assert(player.pos.z == WORLD_BOUND && player.velocity.z == 0);

  ServerWorld world;
  assert(world.AddPlayer(7));
  assert(!world.AddPlayer(7));
  assert(!world.QueueInputs(8, {{{1, {0,0,1}, 0}}}));
  assert(world.QueueInputs(7, {{{2, {0,0,1}, 0}}}));
  world.Tick(); // missing input 1: do not skip it
  assert(world.players.at(7).state.lastInput == 0);
  assert(world.QueueInputs(7, {{{1, {0,0,1}, 0}, {2, {0,0,1}, 0}}}));
  PlayerState expected = world.players.at(7).state;
  for (uint32_t sequence = 1; sequence <= 2; ++sequence) {
    SimulateMovement(expected, {sequence, {0,0,1}, 0});
    world.Tick();
  }
  assert(world.players.at(7).state.pos.z == expected.pos.z);
  world.QueueInputs(7, {{{1, {0,0,1}, 0}, {2, {0,0,1}, 0}}});
  world.Tick(); // duplicates cannot move us twice
  assert(world.players.at(7).state.pos.z == expected.pos.z);
  assert(world.Snapshot().tick == 4);
  assert(!world.QueueInputs(7, {{{3, {2,0,0}, 0}}}));
  assert(!ValidInput({1, {}, NAN}));
  assert(!ValidInput({1, {INFINITY,0,0}, 0}));
  assert(IsNewer(0, UINT32_MAX));
  world.players.at(7).state.lastInput = UINT32_MAX;
  world.QueueInputs(7, {{{0, {0,0,1}, 0}}});
  world.Tick();
  assert(world.players.at(7).state.lastInput == 0);

  ServerWorld full;
  for (uint32_t id = 0; id < MAX_PLAYERS; ++id)
    assert(full.AddPlayer(id));
  assert(!full.AddPlayer(MAX_PLAYERS));
  world.QueueInputs(7, {{{MAX_PENDING_INPUTS+1, {}, 0}}});
  assert(world.players.at(7).pending.empty());

  ClientSimulation client;
  client.Reconcile(expected);
  for (uint32_t i = 0; i < MAX_PENDING_INPUTS; ++i)
    assert(client.AddInput({}, 0));
  assert(!client.AddInput({}, 0));
  client.Reconcile(expected);
  assert(client.pending.size() == MAX_PENDING_INPUTS);
  std::cout << "All movement tests passed!\n";
}
