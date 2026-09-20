#include "game/Player.h"
#include "raylib.h"

void Player::DrawPlayer() { DrawTexture(sprite, pos.x, pos.y, WHITE); }
Player::Player(PlayerID id_c) : playerID(id_c) {
  // initialize client and conect to server
  sprite = LoadTexture("github.png");
}

Player::~Player() {
  UnloadTexture(sprite);
} // make sure to UnloadTexture(mainPlayerSprite);
