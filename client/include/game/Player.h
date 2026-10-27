#pragma once
#include "GameTypes.h"
#include "PitchView.h"

class Player {
public:
  const PlayerID playerID;
  explicit Player(PlayerID id) : playerID(id) { state.playerID = id; }
  void DrawPlayer(const PitchView &view, bool local) const;
  PlayerState state;
};
