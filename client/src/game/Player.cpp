#include "game/Player.h"
#include "raylib.h"

void Player::DrawPlayer(const PitchView &view, bool local) const {
  Vector2 centre = view.ToScreen(state.pos);
  float radius = PLAYER_RADIUS * view.scale;
  Color kit = TeamColor(state.team, state.keeper);

  DrawEllipse(centre.x + 2, centre.y + 3, radius, radius * 0.8f, Fade(BLACK, 0.25f));
  if (state.keeper) // their reach, arms out
    DrawCircleV(centre, KEEPER_RADIUS * view.scale, Fade(kit, 0.25f));
  DrawCircleV(centre, radius, kit);
  if (local)
    DrawRing(centre, radius, radius + 2.5f, 0, 360, 32, RAYWHITE);

  Vec3 forward = Rotate(state.orientation, {0, 0, 1});
  DrawLineEx(centre, {centre.x + forward.x * radius * 1.7f, centre.y - forward.z * radius * 1.7f},
             3, Fade(BLACK, 0.55f));

  const char *label = state.keeper ? "GK" : local ? "you" : TextFormat("%u", playerID);
  int width = MeasureText(label, 10);
  DrawText(label, centre.x - width / 2, centre.y - radius - 13, 10, Fade(RAYWHITE, 0.9f));
}
