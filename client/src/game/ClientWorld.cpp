#include "game/ClientWorld.h"
#include "ClientReplicationSystem.h"
#include "raylib.h"

void ClientWorld::Update(float dt) {}
ClientWorld::ClientWorld() {};

bool ClientWorld::AddPlayer(PlayerID playerID) {
    return players.try_emplace(playerID, playerID).second;
}

void ClientWorld::Draw() {
  ClearBackground(WHITE);
  // std::cout << players.size() << '\n';
  for (auto &[id, ref]: players) {
    ref.DrawPlayer();

    DrawText(std::string(std::to_string(ref.pos.x) + '\n' +
                         std::to_string(ref.pos.y))
                 .c_str(),
             ref.pos.x, ref.pos.y, 12, BLACK);
  }
}
