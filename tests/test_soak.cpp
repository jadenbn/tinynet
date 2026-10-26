#include "Bot.h"
#include "Client.h"
#include "ClientSimulation.h"
#include "GameServer.h"
#include <array>
#include <cassert>
#include <chrono>
#include <iostream>
#include <thread>

using Clock = std::chrono::steady_clock;

// a headless client: the same prediction the real one runs, with a bot on
// the controls. no window, so it can run as a test
struct BotClient {
  Client client;
  ClientSimulation simulation;
  PlayerID id = NO_PLAYER;
  uint32_t lastTick = 0;
  bool haveSnapshot = false;
  uint32_t ticks = 0;
  std::vector<GameEvent> goals, chat;
  float worstBallCorrection = 0;
  size_t mostPending = 0;

  BotClient(unsigned short port, Address server)
      : client(Address(127, 0, 0, 1, port), server) {}

  void Receive() {
    client.UpdateConnection();
    uint8_t data[MAX_PACKET_SIZE];
    Buffer buff{data, 0, 0, sizeof(data)};
    while (client.ReceiveFromServer(buff) > 0) {
      auto type = static_cast<PacketType>(packets::ReadChar(buff));
      if (type == PacketType::ConnectionAccepted && id == NO_PLAYER) {
        id = packets::ReadInteger(buff);
      } else if (type == PacketType::WorldSnapshot && id != NO_PLAYER) {
        WorldSnapshot snapshot = WorldSnapshot::deserialize(buff);
        if (haveSnapshot && !IsNewer(snapshot.tick, lastTick))
          continue; // late or duplicated
        haveSnapshot = true;
        lastTick = snapshot.tick;
        simulation.Reconcile(snapshot, id);
        // a kickoff teleport is snapped, not smoothed, so it doesn't count
        float error = std::sqrt(Dot(simulation.BallError(), simulation.BallError()));
        worstBallCorrection = std::max(worstBallCorrection, error);
      }
    }
    uint8_t message[ReliableChannel::MAX_MESSAGE_SIZE];
    int size;
    while ((size = client.GetConnection().ReceiveReliable(message, sizeof(message))) > 0) {
      Buffer messageBuff{message, 0, size, size};
      GameEvent event = EventMessage::deserialize(messageBuff).event;
      if (event.type == EventType::Goal)
        goals.push_back(event);
      if (event.type == EventType::QuickChat)
        chat.push_back(event);
    }
  }

  void Play() {
    simulation.DecayErrors(TICK_DT);
    if (id == NO_PLAYER) {
      client.SendPacket(ConnectionRequest{});
      return;
    }
    // now and then, say something, so there are always events to check
    if (ticks % 90 == 45)
      client.SendReliable(EventMessage{{EventType::QuickChat, NO_PLAYER, 0,
                                        static_cast<uint8_t>(ticks / 90 % QUICK_CHAT_PHRASES)}});
    BotDecision decision = BotThink(simulation.local, simulation.ball,
                                    simulation.Others(), ticks++);
    Vec3 aim = decision.aim - simulation.local.pos;
    float yaw = Dot(aim, aim) > 0.01f ? YawTowards(aim) : 0;
    simulation.AddInput(RelativeDirection(Normalize(decision.move), yaw), yaw,
                        decision.kick);
    mostPending = std::max(mostPending, simulation.pending.size());
    PlayerInputPacket packet;
    for (const auto &input : simulation.pending)
      if (packet.inputs.size() < INPUTS_PER_PACKET)
        packet.inputs.push_back(input);
    client.SendPacket(packet);
  }
};

int main() {
  Address address(127, 0, 0, 1, 3030);
  GameServer server(address);
  server.Initialize();
  // two on blue, one on orange (teams are balanced as people join)
  std::array<BotClient, 3> bots{BotClient(3031, address), BotClient(3032, address),
                                BotClient(3033, address)};
  for (auto &bot : bots) {
    bot.client.Initialize();
    bot.client.SetConditions({60, 20, 0.05f, 0.01f}); // the overlay's "mobile"
  }

  // ten seconds of real time at 60Hz
  constexpr int TICKS = 600;
  auto start = Clock::now();
  for (int tick = 0; tick < TICKS; tick++) {
    for (auto &bot : bots) {
      bot.Receive();
      bot.Play();
    }
    server.ReceivePackets();
    server.Step();
    std::this_thread::sleep_until(start + std::chrono::duration_cast<Clock::duration>(
        std::chrono::duration<double>((tick + 1) * static_cast<double>(TICK_DT))));
  }
  // let the last events land. the server has to keep stepping: its packets
  // are what carry any resends
  for (int i = 0; i < 60; i++) {
    for (auto &bot : bots) {
      bot.Receive();
      bot.client.SendPacket(Heartbeat{});
    }
    server.ReceivePackets();
    server.Step();
    std::this_thread::sleep_for(std::chrono::milliseconds(16));
  }

  const MatchState &match = server.World().match;
  int goals = match.score[0] + match.score[1];
  float worst = 0;
  size_t pending = 0;
  for (auto &bot : bots) {
    assert(bot.client.GetIsConnected());
    assert(bot.simulation.ready && bot.simulation.PredictingBall());
    // everyone saw every goal, once, in the same order
    assert(bot.goals.size() == static_cast<size_t>(goals));
    for (size_t i = 0; i < bot.goals.size(); i++) {
      assert(bot.goals[i].score == bots[0].goals[i].score);
      assert(bot.goals[i].player == bots[0].goals[i].player);
    }
    worst = std::max(worst, bot.worstBallCorrection);
    pending = std::max(pending, bot.mostPending);
  }
  // and every bit of chat since they joined, once each, in the order the
  // server sent it. someone whose handshake took a few tries may have missed
  // the first couple, so compare against the end of the longest list
  const std::vector<GameEvent> *longest = &bots[0].chat;
  for (const auto &bot : bots)
    if (bot.chat.size() > longest->size())
      longest = &bot.chat;
  for (const auto &bot : bots) {
    assert(bot.chat.size() + 3 >= longest->size());
    size_t offset = longest->size() - bot.chat.size();
    for (size_t i = 0; i < bot.chat.size(); i++) {
      assert(bot.chat[i].player == (*longest)[offset + i].player);
      assert(bot.chat[i].value == (*longest)[offset + i].value);
    }
  }
  assert(longest->size() >= 3 * 6);

  // 60ms + up to 20ms each way is ~10 ticks there and back; the input queue
  // should never get anywhere near the 2 second cap
  assert(pending < 40);

  ConnectionStats stats = bots[0].client.GetConnection().GetStats();
  std::cout << "3 bots, 10s on the mobile preset: " << longest->size()
            << " chat messages and " << goals << " goals ("
            << int(match.score[0]) << "-" << int(match.score[1]) << "), at most "
            << pending << " inputs ahead, biggest ball correction "
            << worst * 100 << "cm, one client lost " << stats.packetsLost
            << " packets\n";
  std::cout << "Soak test passed!\n";
}
