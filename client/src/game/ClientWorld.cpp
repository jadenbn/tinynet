#include "game/ClientWorld.h"
#include "Client.h"
#include "ClientReplicationSystem.h"
#include "raylib.h"
#include "resource_dir.h"

ClientWorld::ClientWorld(Address &clientAddress, Address &serverAddress)
    : client(clientAddress, serverAddress), replicationSystem(*this) {
  client.Initialize();
  client.SendPacket(
      PlayerInputPacket{0.0f, 0.0f}); // our 'handshake' for now lol
}

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
  // Texture playerSprite = LoadTexture("github.png");

  SetTargetFPS(144);
}