// for handling gamestate
#pragma once
#include "Player.h"
#include <unordered_map>

class ClientWorld {
public:
  ClientWorld();
  void Draw();
  Vec3 authoritative;
  Vec3 viewCenter;
  bool hasLocal = false;
  bool AddPlayer(PlayerID playerID);

  std::unordered_map<PlayerID, Player> players;
};
