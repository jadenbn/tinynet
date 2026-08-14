#include "GameTypes.h"
#include "raylib.h"
#include "raymath.h"
class Player {

public:
  const PlayerID playerID;
  Player(PlayerID id_c, Texture sprite_c) : playerID(id_c), sprite(sprite_c) {};
  void DrawPlayer();
  ~Player(); // make sure to UnloadTexture(mainPlayerSprite);
private:
  Vector2 pos = Vector2Zero();
  Texture sprite;
};
