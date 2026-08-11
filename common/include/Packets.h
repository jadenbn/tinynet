#pragma once
#include <chrono>
#include <cstdint>

constexpr static uint8_t MAX_PACKET_SIZE = 1400;
constexpr static uint8_t SENT_QUEUE_SIZE = 256;

// tolerance for timeout when running getlostpackets on the sent q
constexpr static std::chrono::milliseconds PACKET_TIMEOUT =
    std::chrono::milliseconds(150);

// utility header for writing and reading packet data

// ring buffer for quickly handling our acks and such
struct ReceivedQueue {
  static constexpr auto SIZE = 1024;
  uint32_t numbers[SIZE];
  bool isAcked[SIZE];
  bool exists(uint32_t sequenceNumber);
  void insert(uint32_t sequenceNumber);
};
struct SentPacketMetadata {
  uint32_t sequenceNumber;
  bool acked;
  std::chrono::steady_clock::time_point timeSent; // to calculate rtt
};

struct SentQueue {
  SentPacketMetadata queue[SENT_QUEUE_SIZE];

  bool exists(uint32_t sequenceNumber);
  void insert(uint32_t sequenceNumber,
              std::chrono::steady_clock::time_point timeSent);
  void ackPacket(uint32_t sequenceNumber, uint32_t bitfield);
  std::vector<uint32_t> getLostPackets(uint32_t highestAckReceived);

private:
  bool ack(uint32_t sequenceNumber);
};
struct Buffer {
  // shape of data:
  // data[0] - data[3] - protocol hash
  // data[4] - data[7] - sequence number
  // data[8] - data[11] - ack
  // data[12] - data[15] - ack bitfield
  uint8_t *data;
  int index; // write ind
  int size;  // size of buff
};

void WriteInteger(Buffer &buff, uint32_t data);
void WriteShort(Buffer &buff, uint16_t data);
void WriteChar(Buffer &buff, uint8_t data);
void WritePacketHeader(Buffer &buff, uint32_t protocolHash,
                       uint32_t sequenceNumber, uint32_t ack,
                       uint32_t ackBitfield);

uint32_t ReadInteger(Buffer &buff);
uint16_t ReadShort(Buffer &buff);
uint8_t ReadChar(Buffer &buff);