#pragma once
#include "Packets.h"
#include <chrono>
#include <cstdint>

// reliable, ordered messages piggybacked on our unreliable packets. every
// packet carries a small message section after the header:
//   [u8 count] then per message [u16 id][u8 size][size bytes]
// a message keeps getting resent until a packet that carried it is acked, and
// the receiver hands them out in id order, each exactly once. the packet ack
// system already tells us which packets arrived, so we just remember which
// messages rode in which packet
//
// everything lives in fixed size ring buffers, so nothing allocates per
// message or per packet
class ReliableChannel {
public:
  using Clock = std::chrono::steady_clock;

  static constexpr uint16_t WINDOW = 256;    // max messages waiting for an ack
  static constexpr int MAX_MESSAGE_SIZE = 64; // bytes; game events are tiny
  static constexpr int MAX_PER_PACKET = 16;
  // never resend sooner than this, even if rtt is tiny
  static constexpr auto MIN_RESEND_DELAY = std::chrono::milliseconds(100);

  // queues a message. false if it's too big or the window is full, which
  // means the other side hasn't acked anything in a while
  bool Send(const void *data, int size);

  // copies out the next in-order message. 0 if the next one hasn't arrived
  int Receive(void *out, int capacity);

  // writes the message section for an outgoing packet: anything never sent,
  // plus anything unacked that's been waiting longer than ~1.25 rtt. only uses
  // the space left in buff, so the game payload always fits
  void Write(Buffer &buff, uint32_t packetSequence, Clock::time_point now,
             float rttMs);

  // reads the message section of an incoming packet. a malformed section
  // returns false without storing anything, so the packet can be dropped
  bool Read(Buffer &buff);

  // the packet ack system says this packet arrived, so its messages did too
  void OnPacketAcked(uint32_t packetSequence);

  int Unacked() const;
  uint64_t messagesSent = 0;   // first sends only
  uint64_t messagesResent = 0;
  uint64_t messagesReceived = 0; // delivered to the game, not duplicates

private:
  struct Outgoing {
    bool used = false;
    bool sent = false;
    uint16_t id = 0;
    uint8_t size = 0;
    uint8_t data[MAX_MESSAGE_SIZE] = {};
    Clock::time_point lastSent;
  };
  struct Incoming {
    bool used = false;
    uint16_t id = 0;
    uint8_t size = 0;
    uint8_t data[MAX_MESSAGE_SIZE] = {};
  };
  // which messages went out in which packet. acks only reach 33 packets back,
  // so a record is long gone (acked or lost) before its slot gets reused
  struct PacketRecord {
    bool used = false;
    uint32_t sequence = 0;
    uint8_t count = 0;
    uint16_t ids[MAX_PER_PACKET] = {};
  };

  Outgoing outgoing[WINDOW];
  Incoming incoming[WINDOW];
  PacketRecord packets[SENT_QUEUE_SIZE];
  uint16_t oldestUnacked = 0; // everything before this has been acked
  uint16_t nextSendId = 0;
  uint16_t nextReceiveId = 0;
};
