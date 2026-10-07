#include "ClientSimulation.h"
#include "SnapshotHistory.h"
#include "ServerWorld.h"
#include <cassert>
#include <iostream>
#include <numbers>

bool Near(Vec3 a, Vec3 b) { return Dot(a-b, a-b) < 0.0000001f; }
int main() {
  ServerWorld server;
  server.AddPlayer(1);
  ClientSimulation client;
  client.Reconcile(server.players.at(1).state);
  for (int i = 0; i < 20; ++i)
    assert(client.AddInput({0,0,1}, 0.5f));
  PlayerState predicted = client.local;
  server.QueueInputs(1, {{client.pending.begin(), client.pending.end()}});
  for (int i = 0; i < 8; ++i)
    server.Tick();
  auto authoritative = server.players.at(1).state;
  assert(client.Reconcile(authoritative));
  assert(client.pending.size() == 12);
  assert(Near(client.local.pos, predicted.pos));
  assert(Near(client.local.velocity, predicted.velocity));
  assert(client.local.lastInput == 20);
  authoritative.pos.x += 1; // correction must survive the replay
  assert(client.Reconcile(authoritative));
  assert(std::abs(client.local.pos.x-predicted.pos.x-1) < 0.00001f);
  assert(!client.Reconcile(PlayerState{1, {}, {}, {}, 21})); // unsent input ACK
  assert(!client.Reconcile(PlayerState{1, {}, {}, {}, 7})); // stale input ACK
  assert(client.AddInput({}, 0));
  assert(client.pending.back().sequence == 21);
  for (int i = 8; i < 20; ++i)
    server.Tick();
  assert(client.Reconcile(server.players.at(1).state));
  assert(client.pending.size() == 1);

  SnapshotHistory history;
  WorldSnapshot a{100, {{1, PlayerState{1, {0,0,0}}}}};
  WorldSnapshot b{106, {{1, PlayerState{1, {10,0,0}, {},
                                  Yaw(std::numbers::pi_v<float>/2)}}}};
  assert(history.Push(a));
  assert(history.Push(b));
  assert(!history.Push(a));
  assert(!history.Push(b));
  auto middle = history.Sample(1, history.LatestTime()/2);
  assert(middle && std::abs(middle->pos.x-5) < 0.00001f);
  Vec3 facing = Rotate(middle->orientation, {0,0,1});
  assert(std::abs(facing.x-std::sqrt(0.5f)) < 0.00001f);
  assert(history.Sample(1, -1)->pos.x == 0);
  assert(history.Sample(1, 1)->pos.x == 10);
  assert(!history.Sample(2, 0.05));
  SnapshotHistory clock;
  clock.Advance(10); // connection setup time is not server simulation time
  assert(clock.Push(a, 0.05));
  assert(std::abs(clock.RenderTime()+0.05) < 0.000001);
  clock.Advance(0.05);
  assert(clock.Push(b, 0.05));
  assert(std::abs(clock.Sample(1, clock.RenderTime())->pos.x-5) < 0.00001f);
  SnapshotHistory wrapped;
  a.tick = UINT32_MAX;
  b.tick = 0;
  assert(wrapped.Push(a) && wrapped.Push(b));
  assert(std::abs(wrapped.LatestTime()-TICK_DT) < 0.000001);
  for (uint32_t i = 107; i < 200; ++i) {
    b.tick = i;
    assert(history.Push(b));
  }
  assert(history.Sample(1, -100)->pos.x == 10);
  std::cout << "All prediction + interpolation tests passed!\n";
}
