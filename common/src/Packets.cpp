#include "Packets.h"
#include <cassert>
#include <chrono>
#include <cstdint>
#include <sys/types.h>
#include <vector>

bool ReceivedQueue::exists(uint32_t sequenceNumber) {
  auto index = sequenceNumber % SIZE;
  return isAcked[index] && numbers[index] == sequenceNumber;
}

void ReceivedQueue::insert(uint32_t sequenceNumber) {
  auto index = sequenceNumber % SIZE;
  numbers[index] = sequenceNumber;
  isAcked[sequenceNumber] = true;
}

// returns whether or not was acked
bool SentQueue::ack(uint32_t sequenceNumber) {
  auto index = sequenceNumber % SENT_QUEUE_SIZE;
  if (exists(sequenceNumber)) {
    queue[index].acked = true;
    auto rtt =
        std::chrono::steady_clock::now() - queue[index].timeSent; // todo: use
    return true;
  }

  return false;
}

bool SentQueue::exists(uint32_t sequenceNumber) {
  auto index = sequenceNumber % SENT_QUEUE_SIZE;
  return (!queue[index].acked && queue[index].sequenceNumber == sequenceNumber);
}

void SentQueue::insert(uint32_t sequenceNumber,
                       std::chrono::steady_clock::time_point timeSent) {
  auto index = sequenceNumber % SENT_QUEUE_SIZE;
  queue[index] = {sequenceNumber, false, timeSent};
}

// returns whether or not was acked
void SentQueue::ackPacket(uint32_t sequenceNumber, uint32_t bitfield) {
  ack(sequenceNumber);
  for (int i = 0; i < 32; i++) {
    if (((bitfield >> i) & 1) == 1) {
      ack(sequenceNumber - i - 1);
    }
  }
}

std::vector<uint32_t> SentQueue::getLostPackets(uint32_t highestAckReceived) {
  std::vector<uint32_t> lost;
  auto now = std::chrono::steady_clock::now();

  for (int i = 0; i < SENT_QUEUE_SIZE; i++) {
    if (!queue[i].acked && queue[i].sequenceNumber > 0) {

      bool outOfWindow = highestAckReceived - 32 > queue[i].sequenceNumber;
      bool timedOut = queue[i].timeSent - now > PACKET_TIMEOUT;

      if (outOfWindow || timedOut) {
        lost.push_back(queue[i].sequenceNumber);
      }
    }
  }

  return lost;
}

void WriteInteger(Buffer &buff, uint32_t data) {
  assert(buff.index + 4 <= buff.size);

  // write as big endian
  buff.data[buff.index + 0] = static_cast<uint8_t>(data >> 24);
  buff.data[buff.index + 1] = static_cast<uint8_t>(data >> 16);
  buff.data[buff.index + 2] = static_cast<uint8_t>(data >> 8);
  buff.data[buff.index + 3] = static_cast<uint8_t>(data);

  buff.index += 4;
};

void WriteShort(Buffer &buff, uint16_t data) {
  assert(buff.index + 2 <= buff.size);

  // write as big endian
  buff.data[buff.index + 0] = static_cast<uint8_t>(data >> 8);
  buff.data[buff.index + 1] = static_cast<uint8_t>(data);

  buff.index += 2;
};

void WriteChar(Buffer &buff, uint8_t data) {
  assert(buff.index + 1 <= buff.size);

  // write as big endian
  buff.data[buff.index + 0] = static_cast<uint8_t>(data);

  buff.index += 1;
};

uint32_t ReadInteger(Buffer &buff) {
  assert(buff.index + 4 <= buff.size);

  uint8_t b1 = buff.data[buff.index + 3];
  uint8_t b2 = buff.data[buff.index + 2];
  uint8_t b3 = buff.data[buff.index + 1];
  uint8_t b4 = buff.data[buff.index + 0]; // msb
  buff.index += 4;
  return (static_cast<uint32_t>(b4) << 24 | static_cast<uint32_t>(b3) << 16) |
         static_cast<uint32_t>(b2) << 8 | static_cast<uint32_t>(b1);
}

uint16_t ReadShort(Buffer &buff) {
  assert(buff.index + 2 <= buff.size);

  uint8_t b1 = buff.data[buff.index + 1];
  uint8_t b2 = buff.data[buff.index + 0]; // msb
  buff.index += 2;
  return static_cast<uint32_t>(b2) << 8 | static_cast<uint32_t>(b1);
}

uint8_t ReadChar(Buffer &buff) {
  assert(buff.index + 1 <= buff.size);

  uint8_t b1 = buff.data[buff.index + 0];
  buff.index += 1;
  return b1;
}