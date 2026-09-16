#pragma once
#include "GameTypes.h"
#include "ServerTypes.h"
#include <unordered_map>

class ServerWorld {
public:
  bool AddPlayer(const ServerTypes::ServerPlayer &player);
  void UpdatePlayerPosition(PlayerID id, NetworkSafeVector2 position);
  std::unordered_map<PlayerID, ServerTypes::ServerPlayer> getPlayers();

private:
  std::unordered_map<PlayerID, ServerTypes::ServerPlayer> players;
};
