#include "ServerWorld.h"

uint8_t ServerWorld::getNumPlayersConnected() { return numPlayersConnected; }

bool ServerWorld::AddPlayer(const ServerTypes::ServerPlayer &player) {
  return players.try_emplace(player.playerID, player).second;
}
