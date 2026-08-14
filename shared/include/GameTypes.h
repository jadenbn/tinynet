#pragma once
#include <cstdint>

typedef uint32_t PlayerID;
struct NetworkSafeVector2 {
  float x;
  float y;
};

struct PlayerState {
public:
  PlayerID playerID;
  NetworkSafeVector2 pos;
};