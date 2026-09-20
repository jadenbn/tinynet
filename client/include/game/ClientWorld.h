// for handling gamestate
#pragma once
#include "Player.h"
#include <unordered_map>

class ClientWorld {
public:
  ClientWorld();
  void Draw();
  void Update(float dt);
  bool AddPlayer(PlayerID playerID);

  std::unordered_map<PlayerID, Player> players;
};
