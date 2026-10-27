#pragma once
#include "GameTypes.h"
#include "raylib.h"
#include <algorithm>

// metres on the pitch <-> pixels on screen. x goes right, z goes up the screen
struct PitchView {
  Vector2 centre{};
  float scale = 20; // pixels per metre

  // as big as fits in `area`, goals and walls included
  static PitchView Fit(Rectangle area) {
    float width = 2 * (PITCH_HALF_LENGTH + GOAL_DEPTH) + 1.5f;
    float height = 2 * PITCH_HALF_WIDTH + 1.5f;
    return {{area.x + area.width / 2, area.y + area.height / 2},
            std::min(area.width / width, area.height / height)};
  }

  Vector2 ToScreen(Vec3 p) const {
    return {centre.x + p.x * scale, centre.y - p.z * scale};
  }
  // the camera's tilted a little, so height shows up as "further up the
  // screen". the shadow stays on the grass, which is what sells it
  Vector2 ToScreenRaised(Vec3 p) const {
    Vector2 s = ToScreen(p);
    s.y -= std::max(0.0f, p.y - BALL_RADIUS) * scale * 0.6f;
    return s;
  }
  Vec3 ToWorld(Vector2 s) const {
    return {(s.x - centre.x) / scale, 0, (centre.y - s.y) / scale};
  }
};

inline Color TeamColor(uint8_t team, bool keeper = false) {
  Color blue{70, 140, 245, 255}, orange{250, 145, 50, 255};
  // keepers wear a darker kit, like they would on a real pitch
  Color keeperBlue{30, 70, 160, 255}, keeperOrange{185, 85, 20, 255};
  if (keeper)
    return team == 0 ? keeperBlue : keeperOrange;
  return team == 0 ? blue : orange;
}

inline const char *TeamName(uint8_t team) { return team == 0 ? "blue" : "orange"; }
