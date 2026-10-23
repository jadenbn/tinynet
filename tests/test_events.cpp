#include "Client.h"
#include "GameServer.h"
#include <cassert>
#include <chrono>
#include <iostream>
#include <thread>

using Clock = std::chrono::steady_clock;

bool Rejected(uint8_t *data, int length) {
  Buffer buff{data, 0, length, length};
  try {
    EventMessage::deserialize(buff);
  } catch (const std::invalid_argument &) {
    return true;
  }
  return false;
}

void test_event_messages_round_trip() {
  uint8_t data[ReliableChannel::MAX_MESSAGE_SIZE];
  Buffer buff{data, 0, 0, sizeof(data)};
  EventMessage{{EventType::Goal, 7, 1, 1, {2, 3}}}.Serialize(buff);
  assert(buff.length == 9);
  buff.index = 0;
  GameEvent event = EventMessage::deserialize(buff).event;
  assert(event.type == EventType::Goal && event.player == 7);
  assert(event.team == 1 && event.value == 1);
  assert(event.score[0] == 2 && event.score[1] == 3);

  for (int length = 0; length < 9; length++)
    assert(Rejected(data, length));
  data[0] = 99; // no such event
  assert(Rejected(data, 9));
  data[0] = static_cast<uint8_t>(EventType::QuickChat);
  data[6] = QUICK_CHAT_PHRASES; // no such phrase
  assert(Rejected(data, 9));
  data[6] = 0;
  data[5] = 2; // no such team
  assert(Rejected(data, 9));
}

void test_quick_chat_is_rate_limited() {
  ServerWorld world;
  world.AddPlayer(3);
  world.TakeEvents();
  assert(world.QuickChat(3, 1));
  assert(!world.QuickChat(3, 2)); // too soon
  assert(!world.QuickChat(3, QUICK_CHAT_PHRASES));
  assert(!world.QuickChat(4, 0)); // not playing
  for (int i = 0; i < 30; i++)
    world.Tick();
  assert(world.QuickChat(3, 2));
  auto events = world.TakeEvents();
  assert(events.size() == 2);
  assert(events[0].type == EventType::QuickChat && events[0].player == 3);
  assert(events[0].value == 1 && events[1].value == 2);
}

// a client as far as this test cares: connect, keep packets flowing, and
// write down every event that comes in
struct TestClient {
  Client client;
  PlayerID id = NO_PLAYER;
  std::vector<GameEvent> events;

  TestClient(unsigned short port, Address server)
      : client(Address(127, 0, 0, 1, port), server) {}

  void Update() {
    client.UpdateConnection();
    uint8_t data[MAX_PACKET_SIZE];
    Buffer buff{data, 0, 0, sizeof(data)};
    while (client.ReceiveFromServer(buff) > 0) {
      if (static_cast<PacketType>(packets::ReadChar(buff)) ==
          PacketType::ConnectionAccepted)
        id = packets::ReadInteger(buff);
    }
    uint8_t message[ReliableChannel::MAX_MESSAGE_SIZE];
    int size;
    while ((size = client.GetConnection().ReceiveReliable(message, sizeof(message))) > 0) {
      Buffer messageBuff{message, 0, size, size};
      events.push_back(EventMessage::deserialize(messageBuff).event);
    }
    if (id == NO_PLAYER)
      client.SendPacket(ConnectionRequest{});
    else
      client.SendPacket(Heartbeat{});
  }

  int Count(EventType type) const {
    int count = 0;
    for (const auto &event : events)
      count += event.type == type;
    return count;
  }
};

void test_events_reach_everyone_once_over_a_bad_network() {
  Address address(127, 0, 0, 1, 3020);
  GameServer server(address);
  server.Initialize();
  TestClient a(3021, address), b(3022, address);
  for (TestClient *c : {&a, &b}) {
    c->client.Initialize();
    c->client.SetConditions({20, 10, 0.2f, 0.05f});
  }

  auto pump = [&](auto done) {
    auto deadline = Clock::now() + std::chrono::seconds(5);
    while (!done() && Clock::now() < deadline) {
      a.Update();
      b.Update();
      server.ReceivePackets();
      server.Step();
      std::this_thread::sleep_for(std::chrono::milliseconds(5));
    }
    return done();
  };

  assert(pump([&] { return a.id != NO_PLAYER && b.id != NO_PLAYER; }));
  assert(a.client.SendReliable(EventMessage{{EventType::QuickChat, 999, 0, 2}}));
  ServerWorld &world = server.World();
  world.lastTouch = a.id;
  // off to the side of the keeper, already on its way in
  world.ball = {{PITCH_HALF_LENGTH - 0.5f, BALL_RADIUS, 2.4f}, {10, 0, 0}, 0};

  assert(pump([&] {
    return a.Count(EventType::Goal) && b.Count(EventType::Goal) &&
           a.Count(EventType::QuickChat) && b.Count(EventType::QuickChat);
  }));
  // let any resends turn up, then check nothing came twice
  pump([] { return false; });
  for (TestClient *c : {&a, &b}) {
    assert(c->Count(EventType::Goal) == 1);
    assert(c->Count(EventType::QuickChat) == 1);
  }
  for (const auto &event : b.events) {
    if (event.type == EventType::QuickChat) {
      // the server says who it was from, not the client
      assert(event.player == a.id && event.value == 2);
    }
    if (event.type == EventType::Goal) {
      assert(event.player == a.id && event.score[0] + event.score[1] == 1);
    }
  }
  // a saw b join; b saw itself join
  assert(a.Count(EventType::PlayerJoined) >= 1 && b.Count(EventType::PlayerJoined) >= 1);

  ConnectionStats stats = server.Connections().GetClientMap().at(b.id).GetStats();
  std::cout << "events delivered over 20% loss; server resent " << stats.messagesResent
            << " of " << stats.messagesSent << " messages to one client\n";
}

int main() {
  test_event_messages_round_trip();
  test_quick_chat_is_rate_limited();
  test_events_reach_everyone_once_over_a_bad_network();
  std::cout << "All event tests passed!\n";
}
