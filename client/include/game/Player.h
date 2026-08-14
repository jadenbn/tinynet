#include "GameTypes.h"
#include "raylib.h"
#include "raymath.h"
class Player {

public:
  const PlayerID playerID;
  Player(PlayerID id, Texture sprite) : playerID(id) {};
  void DrawPlayer();
  ~Player(); // make sure to UnloadTexture(mainPlayerSprite);
private:
  Vector2 pos = Vector2Zero();
  Texture sprite;
};
