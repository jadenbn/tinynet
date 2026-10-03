#pragma once
#include "Protocol.h"
#include "ServerTypes.h"
#include <unordered_map>

class ServerWorld {
public:
  bool AddPlayer(ClientID clientID);
  bool QueueInputs(ClientID clientID, const PlayerInputPacket &packet);
  void Tick();
  WorldSnapshot Snapshot() const;
  std::unordered_map<PlayerID, ServerTypes::ServerPlayer> players;
  std::unordered_map<ClientID, PlayerID> clientToPlayer;

private:
  uint32_t tick = 0;
};
