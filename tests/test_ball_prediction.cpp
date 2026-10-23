#include "Ball.h"
#include "ClientSimulation.h"
#include "ServerWorld.h"
#include <cassert>
#include <deque>
#include <functional>
#include <iostream>
#include <numbers>

float Length(Vec3 v) { return std::sqrt(Dot(v, v)); }
constexpr float FACING_PLUS_X = std::numbers::pi_v<float> / 2;

// a client and a server in lockstep with `delay` ticks of latency each way,
// no sockets. the whole point: does the client's prediction of the ball agree
// with what the server ends up doing?
struct Lockstep {
  ServerWorld server;
  ClientSimulation client;
  PlayerID id = 0;
  int delay;
  bool serialize; // send snapshots through the real (quantized) format
  int tick = 0;
  std::deque<std::pair<int, PlayerInputPacket>> toServer;
  std::deque<std::pair<int, WorldSnapshot>> toClient;
  float worstBall = 0, worstLocal = 0; // biggest correction after the first
  int reconciles = 0;
  std::function<void(int)> serverScript = [](int) {};

  Lockstep(int delay, bool serialize = false) : delay(delay), serialize(serialize) {
    server.AddPlayer(id);
  }

  void Step(Vec3 direction, float yaw, KickCommand kick = {}) {
    if (client.ready) {
      client.AddInput(direction, yaw, kick);
      PlayerInputPacket packet;
      for (const auto &input : client.pending)
        if (packet.inputs.size() < INPUTS_PER_PACKET)
          packet.inputs.push_back(input);
      toServer.push_back({tick + delay, packet});
    }

    while (!toServer.empty() && toServer.front().first <= tick) {
      server.QueueInputs(id, toServer.front().second);
      toServer.pop_front();
    }
    serverScript(tick);
    server.Tick();
    toClient.push_back({tick + delay, Wire(server.Snapshot())});

    while (!toClient.empty() && toClient.front().first <= tick) {
      Vec3 ballBefore = client.ball.pos, localBefore = client.local.pos;
      if (client.Reconcile(toClient.front().second, id) && reconciles++ > 0) {
        worstBall = std::max(worstBall, Length(client.ball.pos - ballBefore));
        worstLocal = std::max(worstLocal, Length(client.local.pos - localBefore));
      }
      toClient.pop_front();
    }
    tick++;
  }

  WorldSnapshot Wire(const WorldSnapshot &snapshot) const {
    if (!serialize)
      return snapshot;
    uint8_t data[MAX_PACKET_SIZE];
    Buffer buff{data, 0, 0, sizeof(data)};
    snapshot.Serialize(buff);
    buff.index = 1;
    return WorldSnapshot::deserialize(buff);
  }

  // run at the ball and shoot once it's in reach, like a person would
  void DribbleAndShoot(int ticks, int8_t curve, bool &kicked, bool &instant) {
    for (int i = 0; i < ticks; i++) {
      KickCommand kick;
      Vec3 toBall = client.ball.pos - client.local.pos;
      if (!kicked && client.ready && Length({toBall.x, 0, toBall.z}) < 1.1f &&
          client.local.kickCooldown == 0)
        kick = {KickType::Ground, 230, curve};
      Step({0, 0, 1}, FACING_PLUS_X, kick);
      if (kick.type != KickType::None) {
        kicked = true;
        // the ball's already flying on our screen, no round trip needed
        instant = Length(client.ball.velocity) > 15;
      }
    }
  }
};

void test_prediction_matches_the_server_exactly() {
  // 100ms each way
  Lockstep loop(6);
  bool kicked = false, instant = false;
  loop.DribbleAndShoot(200, 60, kicked, instant);
  assert(kicked && instant);
  // the server agreed with every touch, the kick and the curve
  assert(loop.reconciles > 100);
  assert(loop.worstBall == 0 && loop.worstLocal == 0);
  // and it went in
  assert(loop.server.lastTouch == loop.id);
  assert(loop.server.match.score[0] == 1);
}

void test_quantized_snapshots_are_close_enough() {
  // centimetre positions mean every snapshot nudges things a little
  Lockstep loop(6, true);
  bool kicked = false, instant = false;
  loop.DribbleAndShoot(200, -60, kicked, instant);
  assert(kicked && instant);
  std::cout << "worst correction through quantized snapshots: ball "
            << loop.worstBall * 100 << "cm, player " << loop.worstLocal * 100
            << "cm\n";
  assert(loop.worstBall < 0.02f && loop.worstLocal < 0.01f);
}

void test_someone_else_getting_there_first_is_smoothed() {
  Lockstep loop(6);
  // a defender, controlled on the server, who steps across the ball's path
  // the moment it's kicked. dead reckoning can't see that coming
  PlayerID defender = 1;
  loop.server.AddPlayer(defender);
  loop.server.players.at(defender).state.pos = {8, 0, -1.5f};
  uint32_t sequence = 0;
  loop.serverScript = [&](int) {
    bool shot = Length(loop.server.ball.velocity) > 8;
    Vec3 across = shot ? Vec3{0, 0, 1} : Vec3{};
    loop.server.QueueInputs(defender, {{{++sequence, across, 0}}});
  };

  bool kicked = false, blocked = false;
  float worstJump = 0;
  for (int i = 0; i < 160; i++) {
    KickCommand kick;
    Vec3 toBall = loop.client.ball.pos - loop.client.local.pos;
    if (!kicked && loop.client.ready && Length({toBall.x, 0, toBall.z}) < 1.1f)
      kick = {KickType::Ground, 160, 0};
    kicked = kicked || kick.type != KickType::None;

    // where we'd draw the ball at the end of last tick, which is the same
    // moment as the start of this one, either side of any correction
    loop.client.DecayErrors(TICK_DT);
    Vec3 before = loop.client.RenderBall(1);
    float correctionsBefore = loop.worstBall;
    // run at the ball, then stand and watch once it's struck
    loop.Step(kicked ? Vec3{} : Vec3{0, 0, 1}, FACING_PLUS_X, kick);
    blocked = blocked || loop.server.lastTouch == defender;
    if (loop.worstBall > correctionsBefore) {
      Vec3 after = loop.client.RenderBall(0);
      worstJump = std::max(worstJump, Length(after - before));
    }
  }
  assert(kicked && blocked);
  std::cout << "a surprise block: biggest correction " << loop.worstBall * 100
            << "cm, biggest jump on screen " << worstJump * 100 << "cm\n";
  // we got it wrong (dead reckoning saw most of it coming, not all of it),
  // but the correction never shows up as a jump on screen
  assert(loop.worstBall > 0.05f);
  assert(worstJump < 0.001f);
  // corrections fade out in a fraction of a second
  float error = Length(loop.client.BallError());
  for (int i = 0; i < 30; i++)
    loop.client.DecayErrors(TICK_DT);
  assert(Length(loop.client.BallError()) <= error * 0.01f + 0.0001f);
}

void test_kickoffs_snap_instead_of_sliding() {
  ClientSimulation client;
  WorldSnapshot snapshot;
  snapshot.players[0] = PlayerState{0, {-6, 0, 0}};
  snapshot.ball = {{15, BALL_RADIUS, 2}, {}, 0};
  assert(client.Reconcile(snapshot, 0));
  // back to the centre spot: that's a teleport, not something to smooth
  snapshot.tick = 1;
  snapshot.ball = BallState{};
  assert(client.Reconcile(snapshot, 0));
  assert(Length(client.BallError()) == 0);
  assert(Length(client.RenderBall(1) - snapshot.ball.pos) == 0);
  // and nothing to reconcile against without us in it
  assert(!client.Reconcile(WorldSnapshot{}, 0));
}

int main() {
  test_prediction_matches_the_server_exactly();
  test_quantized_snapshots_are_close_enough();
  test_someone_else_getting_there_first_is_smoothed();
  test_kickoffs_snap_instead_of_sliding();
  std::cout << "All ball prediction tests passed!\n";
}
