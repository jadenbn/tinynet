#include "ClientGame.h"
#include "ClientReplicationSystem.h"
#include "game/ClientWorld.h"
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

int main(int argc, char *argv[]) {
  if (argc < 2) {
    std::cerr << "Error: Please provide a port for this client.\n";
    std::cerr << "Usage: " << argv[0] << " <number>\n";
    return 1;
  }

  Address serverAddress(127, 0, 0, 1,
                        3000); // def unsecure but fine for now i thiknks
  Address clientAddress(127, 0, 0, 1, std::stoi(argv[1]));
  ClientGame game(clientAddress, serverAddress);

  // game loop
  game.NetworkInit();
  game.GameInit();
  game.GameLoop();
  return 0;
}
