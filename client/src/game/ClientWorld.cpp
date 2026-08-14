#include "game/ClientWorld.h"
#include "ClientReplicationSystem.h"
#include "raylib.h"
#include <iostream>

void ClientWorld::Update(float dt) {}
ClientWorld::ClientWorld() {};

void ClientWorld::Draw() {
  ClearBackground(WHITE);
  // std::cout << players.size() << '\n';

  for (Player &player : players) {
    player.DrawPlayer();

    DrawText(std::string(std::to_string(player.pos.x) + '\n' +
                         std::to_string(player.pos.y))
                 .c_str(),
             player.pos.x, player.pos.y, 12, BLACK);
  }
}