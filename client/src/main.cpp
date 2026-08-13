#include "../shared/include/Protocol.h"
#include "Address.h"
#include "Client.h"
#include "ClientReplicationSystem.h"
#include "Packets.h"
#include "raylib.h"
#include "raymath.h"
#include "resource_dir.h"
#include <iostream>

class Player {

public:
  Vector2 position;
  Texture sprite;

  Player(Vector2 initialPosition, Texture playerSprite) {
    position = initialPosition;
    sprite = playerSprite;
  }

  void DrawPlayer() {
    // todo: make this cleaner
    // DrawTextureEx(sprite, {this->position.x, this->position.y}, 0.0f,
    // scaleFactor, BLUE);
    DrawTexture(sprite, this->position.x, this->position.y, BLUE);
  }
};

void MovePlayer(Player *p, int speed, Client &client) {
  Vector2 direction = {0.0f, 0.0f};

  if (IsKeyDown(KEY_W))
    direction.y -= 1.0f;
  if (IsKeyDown(KEY_A))
    direction.x -= 1.0f;
  if (IsKeyDown(KEY_S))
    direction.y += 1.0f;
  if (IsKeyDown(KEY_D))
    direction.x += 1.0f;

  if (direction.x != 0.0f || direction.y != 0.0f) {
    direction = Vector2Normalize(direction);
  }

  float deltaTime = GetFrameTime();

  if (Vector2Length(direction) != 0.0f) {
    client.SendPacket(
        PlayerInputPacket{p->position.x += direction.x * speed * deltaTime,
                          p->position.y += direction.y * speed * deltaTime});
  }

  // p->position.x += direction.x * speed * deltaTime;
  // p->position.y += direction.y * speed * deltaTime;
}

ClientGame game;
ClientReplicationSystem replicationSystem(game);

int main(int argc, char *argv[]) {
  if (argc < 2) {
    std::cerr << "Error: Please provide a port for this client.\n";
    std::cerr << "Usage: " << argv[0] << " <number>\n";
    return 1;
  }

  constexpr int WIDTH = 640;
  constexpr int HEIGHT = 480;

  SetConfigFlags(FLAG_VSYNC_HINT | FLAG_WINDOW_HIGHDPI);
  InitWindow(WIDTH, HEIGHT, "tinynet client");

  SearchAndSetResourceDir("resources");

  // initialize client and conect to server
  Address serverAddress(127, 0, 0, 1,
                        3000); // def unsecure but fine for now i thiknks
  Address clientAddress(127, 0, 0, 1, std::stoi(argv[1]));
  Client client(clientAddress, serverAddress);
  client.Initialize();
  client.SendPacket(
      PlayerInputPacket{0.0f, 0.0f}); // our 'handshake' for now lol

  // main player starts in center of screen
  Texture mainPlayerSprite = LoadTexture("github.png");
  Player mainPlayer({(float)(WIDTH - 20.0f), (float)(HEIGHT - 20.0f)},
                    mainPlayerSprite);

  SetTargetFPS(144);
  // game loop
  while (!WindowShouldClose()) {
    client.UpdateConnection();

    uint8_t scratch[MAX_PACKET_SIZE];
    Buffer packet = {scratch, 0, sizeof(scratch)};
    while (client.ReceiveFromServer(packet) > 0) {
      replicationSystem.HandlePacket(packet);
    }

    mainPlayer.position = game.playerPosition;

    BeginDrawing();
    ClearBackground(WHITE);

    MovePlayer(&mainPlayer, 200, client);

    DrawText(std::string(std::to_string(game.playerPosition.x) + '\n' +
                         std::to_string(game.playerPosition.y))
                 .c_str(),
             400, 400, 12, BLACK);

    // draw main player
    mainPlayer.DrawPlayer();

    EndDrawing();
  }

  // cleanup
  UnloadTexture(mainPlayerSprite);
  CloseWindow();
  return 0;
}
