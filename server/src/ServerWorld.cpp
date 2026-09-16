#include "ServerWorld.h"
#include <iostream>

std::unordered_map<PlayerID, ServerTypes::ServerPlayer> ServerWorld::getPlayers() {
    return players;
}

bool ServerWorld::AddPlayer(const ServerTypes::ServerPlayer &player) {
  return players.try_emplace(player.playerID, player).second;
}

void ServerWorld::UpdatePlayerPosition(PlayerID id,
                                       NetworkSafeVector2 position) {
  auto ref = players.find(id);
  if (ref == players.end()) {
    std::cout << "player not found\n";
    return;
  }

  ref->second.position = {position.x, position.y};
}
