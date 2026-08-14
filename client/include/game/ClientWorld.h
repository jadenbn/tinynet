// for handling gamestate
#pragma once
#include "Player.h"
#include <vector>

class ClientWorld {
public:
  ClientWorld();
  void Draw();
  void Update(float dt);

  std::vector<Player> players;
};