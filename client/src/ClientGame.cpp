#include "ClientGame.h"
#include "Client.h"
#include "ClientReplicationSystem.h"
#include "Packets.h"
#include "Protocol.h"
#include "Server.h"
#include "game/ClientWorld.h"
#include "raylib.h"
#include "resource_dir.h"
#include <chrono>
#include <pthread.h>
#include <stdexcept>
#include <thread>

constexpr static int MAX_ATTEMPTS = 100;

ClientGame::ClientGame(Address &clientAddress_c, Address &serverAddress_c)
    : client(clientAddress_c, serverAddress_c), clientWorld(),
      replicationSystem(clientWorld, client) {};

void ClientGame::NetworkInit() {
  client.Initialize();

  // handshake
  bool connectionConfirmed = false;
  int attempts = 0;

  uint8_t scratch[MAX_PACKET_SIZE];
  Buffer buff{scratch, 0, 0, sizeof(scratch)};
  while (!connectionConfirmed && attempts < MAX_ATTEMPTS) {
    client.SendPacket(ConnectionRequest{});
    std::this_thread::sleep_for(std::chrono::milliseconds(16));
    if (client.ReceiveFromServer(buff) > 0) {
      PacketType type = static_cast<PacketType>(packets::ReadChar(buff));

      if (type == PacketType::ConnectionAccepted) {
        connectionConfirmed = true;
        ClientID id = packets::ReadInteger(buff);
        return;
      }
    }
    attempts++;
  }

  throw std::runtime_error("Couldn't connect to server.");
}

void ClientGame::GameInit() {
  constexpr int WIDTH = 640;
  constexpr int HEIGHT = 480;

  SetConfigFlags(FLAG_VSYNC_HINT | FLAG_WINDOW_HIGHDPI);
  InitWindow(WIDTH, HEIGHT, "tinynet client");

  SearchAndSetResourceDir("resources");

  SetTargetFPS(144);
}

void ClientGame::HandleInput() {};
void ClientGame::HandleNetwork() {};

void ClientGame::GameLoop() {
  while (!WindowShouldClose()) {
    client.UpdateConnection();
    clientWorld.Draw();
    HandleInput();
    HandleNetwork();

    uint8_t scratch[MAX_PACKET_SIZE];
    Buffer packet = {scratch, 0, 0, sizeof(scratch)};
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
