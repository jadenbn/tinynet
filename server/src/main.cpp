#include "Server.h"
#include "ServerReplicationSystem.h"
#include "ServerWorld.h"
#include <chrono>
#include <csignal>
#include <iostream>
#include <thread>
#include <vector>

namespace {
volatile std::sig_atomic_t running = 1;
void Stop(int) { running = 0; }
} // namespace

int main() {
  std::signal(SIGINT, Stop);
  std::signal(SIGTERM, Stop);
  Server server(Address(127, 0, 0, 1, 3000));
  server.Initialize();
  ServerWorld game;
  ServerReplicationSystem replication(game);
  using Clock = std::chrono::steady_clock;
  auto previous = Clock::now();
  double accumulator = 0;
  std::cout << "Starting server on 127.0.0.1:3000\n";
  while (running) {
    auto now = Clock::now();
    accumulator += std::min(std::chrono::duration<double>(now-previous).count(), 0.25);
    previous = now;
    server.UpdateConnections();
    uint8_t scratch[MAX_PACKET_SIZE];
    Buffer buff{scratch, 0, 0, sizeof(scratch)};
    ClientID clientID;
    while (server.ReceiveFromClients(clientID, buff) > 0)
      replication.HandlePacket(clientID, buff);
    std::vector<ClientID> gone;
    for (const auto &[id, player] : game.players) {
      auto connection = server.GetClientMap().find(player.owner);
      if (connection == server.GetClientMap().end() || !connection->second.connected)
        gone.push_back(player.owner);
    }
    for (ClientID owner : gone)
      game.RemovePlayer(owner);
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

  // ctrl+c: let everyone know instead of leaving them to time out
  std::vector<ClientID> ids;
  for (const auto &[id, connection] : server.GetClientMap())
    ids.push_back(id);
  for (ClientID id : ids)
    server.Disconnect(id);
  std::cout << "\nServer stopped, disconnected " << ids.size() << " clients\n";
}
