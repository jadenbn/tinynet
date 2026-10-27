// for handling gamestate
#pragma once
#include "Protocol.h"
#include "PitchView.h"
#include "Player.h"
#include <unordered_map>
#include <vector>

// what the client draws: everyone as of their interpolated/predicted
// positions, the ball, and the match
class ClientWorld {
public:
  ClientWorld();
  void Draw(const PitchView &view, bool debug);
  bool AddPlayer(PlayerID playerID);
  // "you", "blue keeper", "player 3"
  const char *Name(PlayerID id) const;

  std::unordered_map<PlayerID, Player> players;
  PlayerID localID = NO_PLAYER;
  bool hasLocal = false;
  Vec3 authoritative; // our position in the last snapshot
  Vec3 serverBall;    // the ball in the last snapshot
  Vec3 ball{0, BALL_RADIUS, 0}; // where to draw it
  MatchState match;
  std::vector<Vec3> preview; // where a kick released right now would go

private:
  Vec3 lastBall;
  float roll = 0; // how far the ball's turned, for the spinning patch on it

  void DrawPitch(const PitchView &view) const;
  void DrawBall(const PitchView &view);
};
