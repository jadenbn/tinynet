#pragma once
#include "raylib.h"
#include <cstdint>

typedef uint32_t PlayerID;
struct NetworkSafeVector2 {
  float x;
  float y;
};

class Player {
  PlayerID playerID;
  Vector2 pos;
  Player(Texture sprite, Vector2 initialPosition);
  void DrawPlayer();
  ~Player(); // make sure to UnloadTexture(mainPlayerSprite);
};