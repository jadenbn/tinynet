#include "Packets.h"
#include <cassert>
#include <cstdint>

bool ReceivedQueue::exists(uint32_t sequenceNumber) {
  auto index = sequenceNumber % SIZE;
  return isAcked[index] && numbers[index] == sequenceNumber;
}

void ReceivedQueue::insert(uint32_t sequenceNumber) {
  auto index = sequenceNumber % SIZE;
  numbers[index] = sequenceNumber;
  isAcked[sequenceNumber] = true;
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