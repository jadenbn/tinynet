#include "ServerWorld.h"
#include "Server.h"
#include <iostream>

bool ServerWorld::AddPlayer(ClientID clientID) {
  PlayerID playerID = clientID;

  if (!players
           .try_emplace(
               playerID,
               ServerTypes::ServerPlayer{clientID, {0.0f, 0.0f}, playerID})
           .second) {
    return false;
  }

  clientToPlayer.emplace(clientID, playerID);
  return true;

  // return players.try_emplace(player.playerID, player).second;
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
