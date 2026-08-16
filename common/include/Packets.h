#pragma once
#include <chrono>
#include <cstddef>
#include <cstdint>

constexpr static std::size_t MAX_PACKET_SIZE = 1400;
constexpr static uint32_t SENT_QUEUE_SIZE = 256;

// tolerance for timeout when running getlostpackets on the sent q
constexpr static float PACKET_TIMEOUT = 1.0f; // ms
static constexpr auto CONNECTION_TIMEOUT_MS = std::chrono::milliseconds(10000);

// utility header for writing and reading packet data

// ring buffer for quickly handling our acks and such
struct ReceivedQueue {
  static constexpr auto SIZE = 1024;
  uint32_t numbers[SIZE] = {};
  bool isAcked[SIZE] = {};
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
  float globalRtt = 0.0f; // ms

  bool
  exists(uint32_t sequenceNumber); // in the future we can optimize for bandwith
                                   // and turn these fields from 8bit to 4bit
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
  int index;  // write ind
  int length; // size of buff
  int capacity;
};

void WriteInteger(Buffer &buff, uint32_t data);
void WriteShort(Buffer &buff, uint16_t data);
void WriteChar(Buffer &buff, uint8_t data);
void WriteFloat(Buffer &buff, float data);

float ReadFloat(Buffer &buff);
uint32_t ReadInteger(Buffer &buff);
uint16_t ReadShort(Buffer &buff);
uint8_t ReadChar(Buffer &buff);

bool ValidateHeader(Buffer &buff);

// packet types!