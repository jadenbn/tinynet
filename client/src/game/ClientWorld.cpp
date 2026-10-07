#include "game/ClientWorld.h"
#include "raylib.h"

ClientWorld::ClientWorld() {}
bool ClientWorld::AddPlayer(PlayerID playerID) {
  return players.try_emplace(playerID, playerID).second;
}
void ClientWorld::Draw() {
  ClearBackground(RAYWHITE);
  Camera2D camera{{320,240}, {320+viewCenter.x*24, 240-viewCenter.z*24}, 0, 1};
  BeginMode2D(camera);
  int edge = static_cast<int>(WORLD_BOUND*24);
  for (int x = -edge; x <= edge; x += 24)
    DrawLine(320+x, 240-edge, 320+x, 240+edge, LIGHTGRAY);
  for (int y = -edge; y <= edge; y += 24)
    DrawLine(320-edge, 240+y, 320+edge, 240+y, LIGHTGRAY);
  if (hasLocal)
    DrawCircleLines(320+authoritative.x*24, 240-authoritative.z*24, 16, RED);
  for (auto &[id, player] : players)
    player.DrawPlayer();
  EndMode2D();
  DrawText("red ring: last server position", 12, 450, 16, DARKGRAY);
  DrawText("WASD move  |  Q/E turn  |  TAB network overlay", 12, 12, 16, DARKGRAY);
}
