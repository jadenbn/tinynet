#pragma once
#include "Protocol.h"
#include "ServerTypes.h"
#include <unordered_map>
#include <vector>

// the authoritative match: players, the ball, the score and the clock
class ServerWorld {
public:
  bool AddPlayer(ClientID clientID); // joins the smaller team
  void RemovePlayer(ClientID clientID);
  bool QueueInputs(ClientID clientID, const PlayerInputPacket &packet);
  void Tick();
  WorldSnapshot Snapshot() const;
  // everything that happened since the last call
  std::vector<GameEvent> TakeEvents();

  std::unordered_map<PlayerID, ServerTypes::ServerPlayer> players;
  std::unordered_map<ClientID, PlayerID> clientToPlayer;
  BallState ball;
  MatchState match;
  PlayerID lastTouch = NO_PLAYER; // gets the credit for a goal

private:
  uint32_t tick = 0;
  std::vector<GameEvent> events;

  void Kickoff(); // everyone back to their spot, ball to the centre spot
  void UpdateMatch();
};
