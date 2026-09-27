#include "../common/include/Connection.h"
#include "../shared/include/Protocol.h"
#include <cassert>
#include <chrono>
#include <iostream>

void test_loss_is_counted_once_out_of_ack_window() {
  SentQueue sent;
  auto now = std::chrono::steady_clock::now();
  for (uint32_t i = 0; i <= 40; i++) {
    sent.insert(i, now);
  }

  // other side got 0-20, except 5
  uint32_t allButFive = ~(1U << (20 - 5 - 1));
  sent.ackPacket(20, allButFive & ((1U << 20) - 1));
  assert(sent.totalAcked == 20);
  assert(sent.totalLost == 0);
  assert(sent.status(5) == PacketStatus::Pending); // still in the window

  // later they got 21-40. 5 can't be acked anymore
  sent.ackPacket(40, 0xFFFFFFFF);
  assert(sent.totalAcked == 40);
  assert(sent.totalLost == 1);
  assert(sent.status(5) == PacketStatus::Lost);
  assert(sent.status(6) == PacketStatus::Acked);
  assert(sent.status(41) == PacketStatus::Unknown);
}

void test_acks_for_packets_never_sent_are_ignored() {
  SentQueue sent;
  sent.insert(0, std::chrono::steady_clock::now());
  sent.ackPacket(4000000000, 0xFFFFFFFF);
  assert(sent.totalAcked == 0);
  assert(sent.totalLost == 0);
}

void receive(Connection &conn, uint32_t sequenceNumber) {
  uint8_t data[64];
  Buffer buff = {data, 0, 0, sizeof(data)};
  packets::WriteInteger(buff, PROTOCOL_HASH);
  packets::WriteInteger(buff, sequenceNumber);
  packets::WriteInteger(buff, 0);
  packets::WriteInteger(buff, 0);
  packets::WriteChar(buff, PacketType::Heartbeat);
  buff.index = 0;
  conn.ProcessReceived(buff);
}

void test_duplicate_and_out_of_order_are_counted() {
  Connection conn(Address(127, 0, 0, 1, 3000));
  receive(conn, 5);
  receive(conn, 3); // late
  receive(conn, 3); // twice

  ConnectionStats stats = conn.GetStats();
  assert(stats.packetsReceived == 3);
  assert(stats.bytesReceived == 3 * 17);
  assert(stats.outOfOrderReceived == 1);
  assert(stats.duplicatesReceived == 1);
  assert(conn.WasReceived(3));
  assert(!conn.WasReceived(4));
}

int main() {
  test_loss_is_counted_once_out_of_ack_window();
  test_acks_for_packets_never_sent_are_ignored();
  test_duplicate_and_out_of_order_are_counted();
  std::cout << "All tests passed!" << std::endl;
}
