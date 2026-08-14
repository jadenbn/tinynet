#include "Packets.h"
#include <cassert>
#include <chrono>
#include <cstdint>
#include <iostream>
#include <ratio>
#include <sys/types.h>
#include <vector>

bool ReceivedQueue::exists(uint32_t sequenceNumber) {
  auto index = sequenceNumber % SIZE;
  return isAcked[index] && numbers[index] == sequenceNumber;
}

void ReceivedQueue::insert(uint32_t sequenceNumber) {
  auto index = sequenceNumber % SIZE;
  numbers[index] = sequenceNumber;
  isAcked[index] = true;
}

// returns whether or not was acked
bool SentQueue::ack(uint32_t sequenceNumber) {
  auto index = sequenceNumber % SENT_QUEUE_SIZE;
  if (exists(sequenceNumber)) {
    queue[index].acked = true;
    float packetRtt =
        std::chrono::duration<float, std::milli>(
            std::chrono::steady_clock::now() - queue[index].timeSent)
            .count();

    if (packetRtt > PACKET_TIMEOUT) {
      return false;
    }

    if (globalRtt == 0.0f) {
      globalRtt = packetRtt;
    } else {
      globalRtt = (0.9 * (globalRtt)) + (0.1 * packetRtt);
    }
    return true;
  }

  return false;
}

bool SentQueue::exists(uint32_t sequenceNumber) {
  auto index = sequenceNumber % SENT_QUEUE_SIZE;
  return (!queue[index].acked &&
          queue[index].sequenceNumber == sequenceNumber &&
          (std::chrono::duration<float, std::milli>(
               std::chrono::steady_clock::now() - queue[index].timeSent)
               .count() <= PACKET_TIMEOUT));
}

void SentQueue::insert(uint32_t sequenceNumber,
                       std::chrono::steady_clock::time_point timeSent) {
  auto index = sequenceNumber % SENT_QUEUE_SIZE;
  queue[index] = {sequenceNumber, false, timeSent};
}

void SentQueue::ackPacket(uint32_t sequenceNumber, uint32_t bitfield) {
  ack(sequenceNumber);
  for (int i = 0; i < 32; i++) {
    if (((bitfield >> i) & 1) == 1) {
      ack(sequenceNumber - i - 1);
    }
  }
}

// when we actually make game packets we should also store the packet type
// in the SentPacketData struct, and resend it. for now we can get the packet
// sequence # that we lost, but not the data.
std::vector<uint32_t> SentQueue::getLostPackets(uint32_t highestAckReceived) {
  std::vector<uint32_t> lost;
  auto now = std::chrono::steady_clock::now();

  for (int i = 0; i < SENT_QUEUE_SIZE; i++) {
    if (!queue[i].acked && queue[i].sequenceNumber > 0) {

      bool outOfWindow = highestAckReceived - 32 > queue[i].sequenceNumber;
      bool timedOut =
          std::chrono::duration<float, std::milli>(now - queue[i].timeSent)
              .count() > PACKET_TIMEOUT;

      if (outOfWindow || timedOut) {
        lost.push_back(queue[i].sequenceNumber);
      }
    }
  }

  return lost;
}

void WriteInteger(Buffer &buff, uint32_t data) {
  assert(buff.index + 4 <= buff.capacity);

  // write as big endian
  buff.data[buff.index + 0] = static_cast<uint8_t>(data >> 24);
  buff.data[buff.index + 1] = static_cast<uint8_t>(data >> 16);
  buff.data[buff.index + 2] = static_cast<uint8_t>(data >> 8);
  buff.data[buff.index + 3] = static_cast<uint8_t>(data);

  buff.index += 4;
  buff.length += 4;
};

void WriteFloat(Buffer &buff, float data) {
  uint32_t int_bits = std::bit_cast<uint32_t>(data);
  WriteInteger(buff, int_bits);
}

void WriteShort(Buffer &buff, uint16_t data) {
  assert(buff.index + 2 <= buff.capacity);

  // write as big endian
  buff.data[buff.index + 0] = static_cast<uint8_t>(data >> 8);
  buff.data[buff.index + 1] = static_cast<uint8_t>(data);

  buff.index += 2;
  buff.length += 2;
};

void WriteChar(Buffer &buff, uint8_t data) {
  assert(buff.index + 1 <= buff.capacity);

  // write as big endian
  buff.data[buff.index + 0] = static_cast<uint8_t>(data);

  buff.index += 1;
  buff.length += 1;
};

uint32_t ReadInteger(Buffer &buff) {
  assert(buff.index + 4 <= buff.capacity);

  uint8_t b1 = buff.data[buff.index + 3];
  uint8_t b2 = buff.data[buff.index + 2];
  uint8_t b3 = buff.data[buff.index + 1];
  uint8_t b4 = buff.data[buff.index + 0]; // msb
  buff.index += 4;
  return (static_cast<uint32_t>(b4) << 24 | static_cast<uint32_t>(b3) << 16) |
         static_cast<uint32_t>(b2) << 8 | static_cast<uint32_t>(b1);
}

float ReadFloat(Buffer &buff) {
  uint32_t int_bits = ReadInteger(buff);
  return std::bit_cast<float>(int_bits);
}

uint16_t ReadShort(Buffer &buff) {
  assert(buff.index + 2 <= buff.capacity);

  uint8_t b1 = buff.data[buff.index + 1];
  uint8_t b2 = buff.data[buff.index + 0]; // msb
  buff.index += 2;
  return static_cast<uint32_t>(b2) << 8 | static_cast<uint32_t>(b1);
}

uint8_t ReadChar(Buffer &buff) {
  assert(buff.index + 1 <= buff.capacity);

  uint8_t b1 = buff.data[buff.index + 0];
  buff.index += 1;
  return b1;
}