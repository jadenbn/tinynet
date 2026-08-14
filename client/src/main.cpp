#include "ClientReplicationSystem.h"
#include <iostream>

// void MovePlayer(Player *p, int speed, Client &client) {
//   Vector2 direction = {0.0f, 0.0f};

//   if (IsKeyDown(KEY_W))
//     direction.y -= 1.0f;
//   if (IsKeyDown(KEY_A))
//     direction.x -= 1.0f;
//   if (IsKeyDown(KEY_S))
//     direction.y += 1.0f;
//   if (IsKeyDown(KEY_D))
//     direction.x += 1.0f;

//   if (direction.x != 0.0f || direction.y != 0.0f) {
//     direction = Vector2Normalize(direction);
//   }

//   float deltaTime = GetFrameTime();

//   if (Vector2Length(direction) != 0.0f) {
//     client.SendPacket(
//         PlayerInputPacket{p->position.x += direction.x * speed * deltaTime,
//                           p->position.y += direction.y * speed * deltaTime});
//   }

//   // p->position.x += direction.x * speed * deltaTime;
//   // p->position.y += direction.y * speed * deltaTime;
// }

ClientGame game;
ClientReplicationSystem replicationSystem(game);

int main(int argc, char *argv[]) {
  if (argc < 2) {
    std::cerr << "Error: Please provide a port for this client.\n";
    std::cerr << "Usage: " << argv[0] << " <number>\n";
    return 1;
  }

  // game loop
  game.Init(argc, argv);
  game.GameLoop();
  return 0;
}
