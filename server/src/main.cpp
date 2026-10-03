#include "Server.h"
#include "ServerReplicationSystem.h"
#include "ServerWorld.h"
#include <chrono>
#include <iostream>
#include <thread>

int main() {
  Server server(Address(127, 0, 0, 1, 3000));
  server.Initialize();
  ServerWorld game;
  ServerReplicationSystem replication(game);
  using Clock = std::chrono::steady_clock;
  auto previous = Clock::now();
  double accumulator = 0;
  std::cout << "Starting server on 127.0.0.1:3000\n";
  while (true) {
    auto now = Clock::now();
    accumulator += std::min(std::chrono::duration<double>(now-previous).count(), 0.25);
    previous = now;
    server.UpdateConnections();
    uint8_t scratch[MAX_PACKET_SIZE];
    Buffer buff{scratch, 0, 0, sizeof(scratch)};
    ClientID clientID;
    while (server.ReceiveFromClients(clientID, buff) > 0)
      replication.HandlePacket(clientID, buff);
    for (auto it = game.players.begin(); it != game.players.end();) {
      auto connection = server.GetClientMap().find(it->second.owner);
      if (connection == server.GetClientMap().end() || !connection->second.connected) {
        game.clientToPlayer.erase(it->second.owner);
        it = game.players.erase(it);
      } else {
        ++it;
      }
    }
    for (auto it = server.GetClientMap().begin(); it != server.GetClientMap().end();) {
      if (!it->second.connected)
        it = server.GetClientMap().erase(it);
      else
        ++it;
    }
    while (accumulator >= TICK_DT) {
      game.Tick();
      auto snapshot = game.Snapshot();
      for (const auto &[id, player] : game.players)
        server.SendPacket(player.owner, snapshot);
      accumulator -= TICK_DT;
    }
    std::this_thread::sleep_for(std::chrono::milliseconds(1));
  }
}
