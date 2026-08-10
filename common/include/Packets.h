#pragma once
#include <cstdint>

constexpr static uint8_t MAX_PACKET_SIZE = 1400;

// utility header for writing and reading packet data

struct Buffer {
  // first byte specifies packet type!
  uint8_t *data;
  int index; // write ind
  int size;  // size of buff
};

void WriteInteger(Buffer &buff, uint32_t data);
void WriteShort(Buffer &buff, uint16_t data);
void WriteChar(Buffer &buff, uint8_t data);

uint32_t ReadInteger(Buffer &buff);
uint16_t ReadShort(Buffer &buff);
uint8_t ReadChar(Buffer &buff);