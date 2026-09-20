#pragma once
#include "GameTypes.h"
#include "ServerTypes.h"
#include <unordered_map>

class ServerWorld {
public:
  bool AddPlayer(ClientID clientID);
  void UpdatePlayerPosition(PlayerID id, NetworkSafeVector2 position);
  std::unordered_map<PlayerID, ServerTypes::ServerPlayer> players;
  std::unordered_map<ClientID, PlayerID> clientToPlayer;

private:
};
