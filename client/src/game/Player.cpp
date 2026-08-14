#include "game/Player.h"
#include "raylib.h"

void Player::DrawPlayer() { DrawTexture(sprite, pos.x, pos.y, WHITE); }

Player::~Player() {
  UnloadTexture(sprite);
} // make sure to UnloadTexture(mainPlayerSprite);
