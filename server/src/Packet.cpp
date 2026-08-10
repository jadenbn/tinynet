#include "Packet.h"
#include <cassert>
#include <cstdint>

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