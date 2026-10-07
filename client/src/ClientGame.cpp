#include "ClientGame.h"
#include "Client.h"
#include "ClientReplicationSystem.h"
#include "GameTypes.h"
#include "Packets.h"
#include "Protocol.h"
#include "Server.h"
#include "game/ClientWorld.h"
#include "raylib.h"
#include "raymath.h"
#include "resource_dir.h"
#include <chrono>
#include <pthread.h>
#include <stdexcept>
#include <thread>
#include <numbers>

constexpr static int MAX_ATTEMPTS = 100;

ClientGame::ClientGame(Address &clientAddress_c, Address &serverAddress_c)
    : client(clientAddress_c, serverAddress_c), clientWorld(),
      replicationSystem(clientWorld, simulation) {};

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

      if (type == PacketType::ConnectionRejected)
        throw std::runtime_error("Server is full");
      if (type == PacketType::ConnectionAccepted && buff.length - buff.index >= 4) {
        connectionConfirmed = true;
        ClientID id = packets::ReadInteger(buff);
        this->id = id;
        replicationSystem.localID = id;
        return;
      }
    }
    attempts++;
  }

  throw std::runtime_error("Couldn't connect to server.");
}

void ClientGame::GameInit() {
  constexpr int WIDTH = 960; // 640 for the game + 320 for the net overlay
  constexpr int HEIGHT = 480;

  SetConfigFlags(FLAG_VSYNC_HINT | FLAG_WINDOW_HIGHDPI);
  InitWindow(WIDTH, HEIGHT, "tinynet client");

  SearchAndSetResourceDir("resources");

  SetTargetFPS(144);
}

void ClientGame::HandleInput() {
  Vec3 direction{static_cast<float>(IsKeyDown(KEY_D)-IsKeyDown(KEY_A)), 0,
                 static_cast<float>(IsKeyDown(KEY_W)-IsKeyDown(KEY_S))};
  if (IsKeyDown(KEY_Q)) yaw -= 2*TICK_DT;
  if (IsKeyDown(KEY_E)) yaw += 2*TICK_DT;
  yaw = std::remainder(yaw, 2*std::numbers::pi_v<float>);
  simulation.AddInput(Normalize(direction), yaw);
  PlayerInputPacket packet;
  for (const auto &input : simulation.pending) {
    if (packet.inputs.size() == INPUTS_PER_PACKET)
      break;
    packet.inputs.push_back(input);
  }
  client.SendPacket(packet);
}
void ClientGame::HandleNetwork() {
  client.UpdateConnection();
  uint8_t scratch[MAX_PACKET_SIZE];
  Buffer packet{scratch, 0, 0, sizeof(scratch)};
  while (client.ReceiveFromServer(packet) > 0)
    replicationSystem.HandlePacket(packet, client.GetRTT()/2000.0);
}

void ClientGame::GameLoop() {
  double previous = GetTime(), accumulator = 0;
  while (!WindowShouldClose()) {
    double now = GetTime();
    double dt = std::min(now-previous, 0.25);
    accumulator += dt;
    previous = now;
    HandleNetwork();
    while (accumulator >= TICK_DT) {
      HandleInput();
      accumulator -= TICK_DT;
    }
    replicationSystem.Update(dt);
    overlay.Update(client);
    BeginDrawing();
    clientWorld.Draw();
    overlay.Draw(client);
    EndDrawing();
  }
  CloseWindow();
}
