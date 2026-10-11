#include "../common/include/Connection.h"
#include "../common/include/ReliableChannel.h"
#include "../shared/include/Protocol.h"
#include <cassert>
#include <chrono>
#include <cstring>
#include <iostream>
#include <thread>

using Clock = std::chrono::steady_clock;
using namespace std::chrono_literals;

struct Packet {
  uint8_t data[MAX_PACKET_SIZE];
  Buffer buff{data, 0, 0, sizeof(data)};
};

// writes one packet's message section from `from` and reads it into `to`
void Deliver(ReliableChannel &from, ReliableChannel &to, uint32_t sequence,
             Clock::time_point now) {
  Packet packet;
  from.Write(packet.buff, sequence, now, 0);
  packet.buff.index = 0;
  assert(to.Read(packet.buff));
}

bool SendNumber(ReliableChannel &channel, uint32_t number) {
  uint8_t data[4];
  std::memcpy(data, &number, 4);
  return channel.Send(data, 4);
}

int ReceiveNumber(ReliableChannel &channel, uint32_t &number) {
  uint8_t data[ReliableChannel::MAX_MESSAGE_SIZE];
  int size = channel.Receive(data, sizeof(data));
  if (size == 4) {
    std::memcpy(&number, data, 4);
  }
  return size;
}

void test_messages_arrive_in_order() {
  ReliableChannel a, b;
  auto now = Clock::now();
  for (uint32_t i = 0; i < 3; i++) {
    assert(SendNumber(a, i));
  }
  Deliver(a, b, 0, now);

  uint32_t number;
  for (uint32_t i = 0; i < 3; i++) {
    assert(ReceiveNumber(b, number) == 4 && number == i);
  }
  assert(ReceiveNumber(b, number) == 0);

  assert(a.Unacked() == 3);
  a.OnPacketAcked(0);
  assert(a.Unacked() == 0);
}

void test_lost_messages_are_resent_and_reordered() {
  ReliableChannel a, b;
  auto start = Clock::now();
  SendNumber(a, 0);
  SendNumber(a, 1);

  // packet 0 carries 0 and 1, but gets lost
  Packet lost;
  a.Write(lost.buff, 0, start, 0);

  // 2 goes out in packet 1, which arrives. it can't be handed out yet
  SendNumber(a, 2);
  Deliver(a, b, 1, start + 10ms);
  uint32_t number;
  assert(ReceiveNumber(b, number) == 0);

  // too soon to resend anything
  Packet early;
  a.Write(early.buff, 2, start + 50ms, 0);
  assert(early.buff.data[0] == 0);

  // after the resend delay 0, 1 and 2 all go again; 2 is a duplicate for b
  Deliver(a, b, 3, start + 150ms);
  for (uint32_t i = 0; i < 3; i++) {
    assert(ReceiveNumber(b, number) == 4 && number == i);
  }
  assert(ReceiveNumber(b, number) == 0);
  assert(a.messagesSent == 3 && a.messagesResent == 3);
  assert(b.messagesReceived == 3);

  // acking the packet with the resends clears everything
  a.OnPacketAcked(3);
  assert(a.Unacked() == 0);
}

void test_acking_old_packets_out_of_order() {
  ReliableChannel a, b;
  auto now = Clock::now();
  SendNumber(a, 0);
  Deliver(a, b, 0, now);
  SendNumber(a, 1);
  Deliver(a, b, 1, now);

  a.OnPacketAcked(1);
  assert(a.Unacked() == 1); // 0 is still waiting
  a.OnPacketAcked(1);       // acking twice is harmless
  a.OnPacketAcked(0);
  assert(a.Unacked() == 0);
}

void test_malformed_sections_are_rejected_whole() {
  ReliableChannel a, b;
  SendNumber(a, 7);
  SendNumber(a, 8);
  Packet packet;
  a.Write(packet.buff, 0, Clock::now(), 0);
  int fullLength = packet.buff.length;

  for (int length = 0; length < fullLength; length++) {
    packet.buff.index = 0;
    packet.buff.length = length;
    assert(!b.Read(packet.buff));
  }
  uint32_t number;
  assert(ReceiveNumber(b, number) == 0); // nothing half-stored

  // too many messages, and a zero sized message
  uint8_t bad[] = {ReliableChannel::MAX_PER_PACKET + 1};
  Buffer tooMany{bad, 0, 1, 1};
  assert(!b.Read(tooMany));
  uint8_t empty[] = {1, 0, 0, 0};
  Buffer zero{empty, 0, 4, 4};
  assert(!b.Read(zero));

  packet.buff.index = 0;
  packet.buff.length = fullLength;
  assert(b.Read(packet.buff));
  assert(packet.buff.index == fullLength);
  assert(ReceiveNumber(b, number) == 4 && number == 7);
}

void test_window_fills_until_acked() {
  ReliableChannel a, b;
  for (int i = 0; i < ReliableChannel::WINDOW; i++) {
    assert(SendNumber(a, i));
  }
  assert(!SendNumber(a, 999));
  uint8_t big[ReliableChannel::MAX_MESSAGE_SIZE + 1] = {};
  assert(!b.Send(big, sizeof(big)));

  // 16 per packet, so 16 packets to get them all out
  auto now = Clock::now();
  for (uint32_t sequence = 0; sequence < 16; sequence++) {
    Deliver(a, b, sequence, now);
    a.OnPacketAcked(sequence);
  }
  assert(a.Unacked() == 0);
  assert(SendNumber(a, 999));
}

void test_ids_wrap_around() {
  ReliableChannel a, b;
  auto now = Clock::now();
  uint32_t expected = 0;
  uint32_t sequence = 0;
  // past 65536 so the 16 bit ids wrap at least once
  while (expected < 70000) {
    for (int i = 0; i < 16; i++) {
      assert(SendNumber(a, expected + i));
    }
    Deliver(a, b, sequence, now);
    a.OnPacketAcked(sequence++);
    uint32_t number;
    for (int i = 0; i < 16; i++) {
      assert(ReceiveNumber(b, number) == 4 && number == expected++);
    }
  }
  assert(a.Unacked() == 0);
}

void test_messages_fit_around_the_payload() {
  ReliableChannel a;
  uint8_t data[ReliableChannel::MAX_MESSAGE_SIZE] = {};
  for (int i = 0; i < 4; i++) {
    a.Send(data, sizeof(data));
  }
  // room for the count byte plus one 64 byte message (3 byte header)
  uint8_t scratch[1 + 3 + 64 + 10];
  Buffer buff{scratch, 0, 0, sizeof(scratch)};
  a.Write(buff, 0, Clock::now(), 0);
  assert(scratch[0] == 1 && buff.length == 1 + 3 + 64);
}

// the real thing: two connections over udp, with the simulator dropping a
// quarter of all packets and shuffling the rest
void test_over_a_lossy_network() {
  Address addressA(127, 0, 0, 1, 3200), addressB(127, 0, 0, 1, 3201);
  Socket socketA, socketB;
  socketA.Open(3200);
  socketB.Open(3201);
  NetworkConditions bad{10.0f, 5.0f, 0.25f, 0.1f};
  socketA.SetConditions(bad);
  socketB.SetConditions(bad);
  Connection a(addressB), b(addressA);

  constexpr uint32_t COUNT = 300;
  uint32_t queued = 0, received = 0;
  auto deadline = Clock::now() + 10s;
  uint8_t payload[1] = {static_cast<uint8_t>(PacketType::Heartbeat)};
  Buffer heartbeat{payload, 1, 1, 1};

  while (received < COUNT && Clock::now() < deadline) {
    // a few new messages each "tick", while both sides keep packets flowing
    for (int i = 0; i < 3 && queued < COUNT; i++) {
      uint8_t data[4];
      std::memcpy(data, &queued, 4);
      if (a.SendReliable(data, 4)) {
        queued++;
      }
    }
    a.Send(socketA, heartbeat);
    b.Send(socketB, heartbeat);
    socketA.Update();
    socketB.Update();

    uint8_t scratch[MAX_PACKET_SIZE];
    Buffer buff{scratch, 0, 0, sizeof(scratch)};
    Address from;
    int bytes;
    while ((bytes = socketA.Receive(from, scratch, sizeof(scratch))) > 0) {
      buff = {scratch, 0, bytes, sizeof(scratch)};
      a.ProcessReceived(buff);
    }
    while ((bytes = socketB.Receive(from, scratch, sizeof(scratch))) > 0) {
      buff = {scratch, 0, bytes, sizeof(scratch)};
      b.ProcessReceived(buff);
    }

    uint8_t message[ReliableChannel::MAX_MESSAGE_SIZE];
    int size;
    while ((size = b.ReceiveReliable(message, sizeof(message))) > 0) {
      uint32_t number;
      assert(size == 4);
      std::memcpy(&number, message, 4);
      assert(number == received); // in order, exactly once
      received++;
    }
    std::this_thread::sleep_for(2ms);
  }

  ConnectionStats stats = a.GetStats();
  std::cout << "delivered " << received << "/" << COUNT
            << " messages over 25% loss, resent " << stats.messagesResent
            << ", " << stats.packetsLost << " packets lost\n";
  assert(received == COUNT);
  assert(stats.messagesResent > 0);
  assert(b.GetStats().messagesReceived == COUNT);
}

int main() {
  test_messages_arrive_in_order();
  test_lost_messages_are_resent_and_reordered();
  test_acking_old_packets_out_of_order();
  test_malformed_sections_are_rejected_whole();
  test_window_fills_until_acked();
  test_ids_wrap_around();
  test_messages_fit_around_the_payload();
  test_over_a_lossy_network();
  std::cout << "All reliable channel tests passed!\n";
}
