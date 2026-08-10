#pragma once
#include <cstdint>

class Packet {
public:
  Packet();

private:
  uint32_t protocol_id; // hash string "tinynet" through std::hash; used to
                        // filter out packets that aren't from client/server
  char *data[];         // change type later
};