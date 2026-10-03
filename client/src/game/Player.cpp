#include "game/Player.h"
#include "raylib.h"

void Player::DrawPlayer() {
  Vector2 center{320 + state.pos.x*24, 240 - state.pos.z*24};
  Vec3 forward = Rotate(state.orientation, {0,0,1});
  Color color = playerID % 2 ? ORANGE : SKYBLUE;
  DrawCircleV(center, 12, color);
  DrawLineEx(center, {center.x+forward.x*24, center.y-forward.z*24}, 3, DARKGRAY);
  DrawText(TextFormat("%u", playerID), center.x-4, center.y-5, 12, BLACK);
}
