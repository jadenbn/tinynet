#pragma once
#include "GameTypes.h"
#include "ServerTypes.h"
#include <cstdint>
#include <unordered_map>

class ServerWorld {
public:
  bool AddPlayer(const ServerTypes::ServerPlayer &player);
  NetworkSafeVector2 playerPosition = {1.0f, 1.0f};
  uint8_t getNumPlayersConnected();

private:
  uint8_t numPlayersConnected = 0;
  std::unordered_map<PlayerID, ServerTypes::ServerPlayer> players;
};