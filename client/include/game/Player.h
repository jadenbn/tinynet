#include "GameTypes.h"
#include "raylib.h"
class Player {

public:
  const PlayerID playerID;
  Vector2 pos;
  Player(Texture sprite, Vector2 initialPosition);
  void DrawPlayer();
  ~Player(); // make sure to UnloadTexture(mainPlayerSprite);
private:
  Texture sprite;
};
