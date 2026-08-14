#include "GameTypes.h"
#include "raylib.h"
#include "raymath.h"
class Player {

public:
  const PlayerID playerID;
  Player(PlayerID id_c);
  void DrawPlayer();
  ~Player(); // make sure to UnloadTexture(mainPlayerSprite);
  Vector2 pos = Vector2Zero();

private:
  Texture sprite;
};
