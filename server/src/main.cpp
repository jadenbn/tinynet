#include "Server.h"
#include "ServerWorld.h"
#include "ServerReplicationSystem.h"
#include "Protocol.h"
#include "Address.h"
#include "Packets.h"
#include "ServerWorld.h"
#include <chrono>
#include <iostream>
#include <thread>

// eventually probably turn into ServerGame to mirror client but will see
int main() {
  Server server(Address(127, 0, 0, 1, 3000));
  server.Initialize();

  ServerWorld game;
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