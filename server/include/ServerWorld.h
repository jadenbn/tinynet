#pragma once
#include "GameTypes.h"
#include "ServerTypes.h"
#include <cstdint>
#include <vector>

class ServerWorld {
public:
  NetworkSafeVector2 playerPosition = {1.0f, 1.0f};
  uint8_t getNumPlayersConnected();

private:
  uint8_t numPlayersConnected = 0;
  std::vector<ServerTypes::ServerPlayer> players;
};