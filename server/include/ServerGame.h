#pragma once
#include "../shared/include/GameTypes.h"
#include <cstdint>

class ServerGame {
public:
  NetworkSafeVector2 playerPosition = {1.0f, 1.0f};
  uint8_t getNumPlayersConnected();

private:
  uint8_t numPlayersConnected = 0;
};