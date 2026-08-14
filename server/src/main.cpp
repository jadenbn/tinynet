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
  server.Initialize();

  ServerGame game;
  ServerReplicationSystem replicationSystem(game);

  std::cout << "Starting server on 127.0.0.1:3000" << '\n';
  while (true) {
    server.UpdateConnections();

    uint8_t tmp[MAX_PACKET_SIZE];
    Buffer buff = {tmp, 0, 0, sizeof(tmp)};
    ClientID clientId;
    while (server.ReceiveFromClients(clientId, buff) > 0) {
      replicationSystem.HandlePacket(clientId, buff);
    }

    for (auto &[clientId, connection] : server.GetClientMap()) {
      server.SendPacket(clientId,
                        WorldSnapshot{game.playerPosition.x,
                                      game.playerPosition.y}); // worldstate
    }
    std::this_thread::sleep_for(std::chrono::milliseconds(16));
  }
}