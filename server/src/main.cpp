#include "../include/Server.h"
#include "../include/ServerGame.h"
#include "../include/ServerReplicationSystem.h"
#include "../shared/include/Protocol.h"
#include "Address.h"
#include "Packets.h"
#include <chrono>
#include <iostream>
#include <thread>

int main() {
  Server server(Address(127, 0, 0, 1, 3000));
  server.initialize();

  ServerGame game;
  ServerReplicationSystem replicationSystem(game);

  std::cout << "Starting server on 127.0.0.1:3000" << '\n';
  while (true) {
    server.UpdateConnection();

    uint8_t tmp[MAX_PACKET_SIZE];
    Buffer buff = {tmp, 0, sizeof(MAX_PACKET_SIZE)};
    while (server.Receive(buff) > 0) {
      replicationSystem.HandlePacket(buff);
    }

    server.SendPacket(
        PlayerInputPacket{game.playerPosition.x, game.playerPosition.y});
    std::this_thread::sleep_for(std::chrono::milliseconds(16));
  }
}