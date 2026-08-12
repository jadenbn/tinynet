#include "Address.h"
#include "Client.h"
#include "Packets.h"
#include "Protocol.h"
#include "TransportLayer.h"
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

  // account for offset of sprite
  // DrawText(
  //     std::string(std::to_string(direction.x) + std::to_string(direction.y))
  //         .c_str(),
  //     p->position.x - (p->sprite.width / 2),
  //     p->position.y - 25 - (p->sprite.height / 2), 12, BLACK);

  if (Vector2Length(direction) != 0.0f) {
    client.SendPacket(PlayerInputPacket{p->position.x, p->position.y});
  }

  float deltaTime = GetFrameTime();
  p->position.x += direction.x * speed * deltaTime;
  p->position.y += direction.y * speed * deltaTime;

  direction = {0, 0};
}

Game game;
TransportLayer transportLayer(game);

int main() {

  constexpr int WIDTH = 640;
  constexpr int HEIGHT = 480;

  SetConfigFlags(FLAG_VSYNC_HINT | FLAG_WINDOW_HIGHDPI);
  InitWindow(WIDTH, HEIGHT, "tinynet client");

  SearchAndSetResourceDir("resources");

  Address clientAddress(127, 0, 0, 1, 3001);
  Client client(clientAddress);
  client.initialize(Address(127, 0, 0, 1, 3000));

  // main player starts in center of screen
  Texture mainPlayerSprite = LoadTexture("github.png");
  Player mainPlayer({(float)(WIDTH - 20.0f), (float)(HEIGHT - 20.0f)},
                    mainPlayerSprite);

  // game loop
  while (!WindowShouldClose()) {
    client.UpdateConnection();

    uint8_t scratch[MAX_PACKET_SIZE];
    Buffer packet = {scratch, 0, sizeof(scratch)};
    while (client.Receive(packet) > 0) {
      transportLayer.HandlePacket(packet);
    }

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
