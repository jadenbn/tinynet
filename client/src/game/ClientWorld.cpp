#include "game/ClientWorld.h"
#include "raylib.h"

void ClientWorld::Update(float dt) {}
ClientWorld::ClientWorld() {}
bool ClientWorld::AddPlayer(PlayerID playerID) {
  return players.try_emplace(playerID, playerID).second;
}
void ClientWorld::Draw() {
  ClearBackground(RAYWHITE);
  for (int x = 8; x < 640; x += 24)
    DrawLine(x, 0, x, 480, LIGHTGRAY);
  for (int y = 0; y < 480; y += 24)
    DrawLine(0, y, 640, y, LIGHTGRAY);
  for (auto &[id, player] : players)
    player.DrawPlayer();
  DrawText("WASD move  |  Q/E turn  |  TAB network overlay", 12, 12, 16, DARKGRAY);
}
