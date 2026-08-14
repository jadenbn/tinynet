#include "game/ClientWorld.h"
#include "raylib.h"
#include "resource_dir.h"

void ClientWorld::GameLoop() {
  while (!WindowShouldClose()) {
    client.UpdateConnection();

    uint8_t scratch[MAX_PACKET_SIZE];
    Buffer packet = {scratch, 0, sizeof(scratch)};
    while (client.ReceiveFromServer(packet) > 0) {
      replicationSystem.HandlePacket(packet);
    }

    // mainPlayer.position = game.playerPosition;

    BeginDrawing();
    ClearBackground(WHITE);

    // MovePlayer(&mainPlayer, 200, client);

    // DrawText(std::string(std::to_string(game.playerPosition.x) + '\n' +
    //                      std::to_string(game.playerPosition.y))
    //              .c_str(),
    //          400, 400, 12, BLACK);

    // draw main player
    // mainPlayer.DrawPlayer();
    for (Player &player : players) {
      player.DrawPlayer();
    }

    EndDrawing();
  }

  // cleanup
  CloseWindow();
}

void ClientWorld::Init(int argc, char *argv[]) {
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
}