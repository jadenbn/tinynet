// for handling gamestate
#pragma once
#include "Player.h"
#include <unordered_map>
#include <vector>

class ClientWorld {
public:
  ClientWorld();
  void Draw();
  void Update(float dt);

  std::unordered_map<PlayerID, Player> players;
};
