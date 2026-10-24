#pragma once
#include "Protocol.h"
#include "ServerTypes.h"
#include <deque>
#include <unordered_map>
#include <vector>

// keepers get ids no client will ever have
constexpr PlayerID KEEPER_IDS[2] = {0xFFFF0000, 0xFFFF0001};

// the authoritative match: players, the ball, the score and the clock
class ServerWorld {
public:
  bool AddPlayer(ClientID clientID); // joins the smaller team
  void RemovePlayer(ClientID clientID);
  void AddKeepers(); // one per team, played by the server
  bool QueueInputs(ClientID clientID, const PlayerInputPacket &packet);
  // passes a quick chat phrase on to everyone, at most twice a second each
  bool QuickChat(ClientID clientID, uint8_t phrase);
  void Tick();
  WorldSnapshot Snapshot() const;
  // everything that happened since the last call
  std::vector<GameEvent> TakeEvents();

  std::unordered_map<PlayerID, ServerTypes::ServerPlayer> players;
  std::unordered_map<ClientID, PlayerID> clientToPlayer;
  BallState ball;
  MatchState match;
  PlayerID lastTouch = NO_PLAYER; // usually gets the credit for a goal
  PlayerID lastShot = NO_PLAYER;  // ...unless it was this shot, deflected in
  uint32_t lastShotTick = 0;
  uint32_t CurrentTick() const { return tick; }

private:
  uint32_t tick = 0;
  std::vector<GameEvent> events;
  std::deque<BallState> recentBalls; // what the keepers see, a bit late

  void Kickoff(); // everyone back to their spot, ball to the centre spot
  void Kicked(PlayerID id);
  PlayerID Scorer(uint8_t team) const;
  void UpdateMatch();
};
