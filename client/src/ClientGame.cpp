#include "ClientGame.h"
#include "Client.h"
#include "ClientReplicationSystem.h"
#include "game/ClientWorld.h"
#include "raylib.h"
#include "resource_dir.h"

ClientGame::ClientGame(Address &clientAddress_c, Address &serverAddress_c)
    : client(clientAddress_c, serverAddress_c), clientWorld(),
      replicationSystem(clientWorld, client) {};

void ClientGame::NetworkInit() {
  client.Initialize();
  client.SendPacket(
      PlayerInputPacket{0.0f, 0.0f}); // our 'handshake' for now lol
}

void ClientGame::GameInit() {
  constexpr int WIDTH = 640;
  constexpr int HEIGHT = 480;

  SetConfigFlags(FLAG_VSYNC_HINT | FLAG_WINDOW_HIGHDPI);
  InitWindow(WIDTH, HEIGHT, "tinynet client");

  SearchAndSetResourceDir("resources");


  SetTargetFPS(144);
}

void ClientGame::GameLoop() {
  while (!WindowShouldClose()) {
    client.UpdateConnection();
    clientWorld.Draw();

    uint8_t scratch[MAX_PACKET_SIZE];
    Buffer packet = {scratch, 0, sizeof(scratch)};
    while (client.ReceiveFromServer(packet) > 0) {
      replicationSystem.HandlePacket(packet);
    }

    // mainPlayer.position = game.playerPosition;

    BeginDrawing();
    clientWorld.Draw();
    EndDrawing();
  }

  // cleanup
  CloseWindow();
}