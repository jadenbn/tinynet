#include "game/ClientWorld.h"
#include "ClientReplicationSystem.h"
#include "raylib.h"

void ClientWorld::Update(float dt) {}
ClientWorld::ClientWorld() {};

void ClientWorld::Draw() {
  ClearBackground(WHITE);
  // std::cout << players.size() << '\n';

  for (const auto &[id, ref]: players) {
    ref.DrawPlayer();

    DrawText(std::string(std::to_string(player.pos.x) + '\n' +
                         std::to_string(player.pos.y))
                 .c_str(),
             player.pos.x, player.pos.y, 12, BLACK);
  }
}
