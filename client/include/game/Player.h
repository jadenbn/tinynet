#pragma once
#include "GameTypes.h"

class Player {
public:
  const PlayerID playerID;
  explicit Player(PlayerID id) : playerID(id) { state.playerID = id; }
  void DrawPlayer();
  PlayerState state;
};
