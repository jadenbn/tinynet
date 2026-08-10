#include "../common/include/Packets.h"
#include <cassert>
#include <charconv>
#include <cstdint>
#include <iostream>

void test_all() {
  Buffer buff;
  uint8_t data[128];
  buff.size = 128;
  buff.index = 0;
  buff.data = data;

  WriteInteger(buff, 16161616);
  assert(buff.index == 4);

  WriteShort(buff, 24213);
  assert(buff.index == 6);

  WriteChar(buff, 4);
  assert(buff.index == 7);

  buff.index = 0;
  assert(ReadInteger(buff) == 16161616);
  assert(ReadShort(buff) == 24213);
  assert(ReadChar(buff) == 4);
  assert(buff.index == 7);
}

int main() {
  test_all();
  std::cout << "All tests passed!" << std::endl;
}