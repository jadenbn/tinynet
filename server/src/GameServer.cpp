#include "GameServer.h"
#include <vector>

GameServer::GameServer(Address address) : server(address), replication(world) {}

void GameServer::Initialize() { server.Initialize(); }

void GameServer::ReceivePackets() {
  server.UpdateConnections();
  uint8_t scratch[MAX_PACKET_SIZE];
  Buffer buff{scratch, 0, 0, sizeof(scratch)};
  ClientID clientID;
  while (server.ReceiveFromClients(clientID, buff) > 0)
    replication.HandlePacket(clientID, buff);

  for (auto &[id, connection] : server.GetClientMap()) {
    uint8_t message[ReliableChannel::MAX_MESSAGE_SIZE];
    int size;
    while ((size = connection.ReceiveReliable(message, sizeof(message))) > 0) {
      Buffer messageBuff{message, 0, size, size};
      replication.HandleMessage(id, messageBuff);
    }
  }

  std::vector<ClientID> gone;
  for (const auto &[id, player] : world.players) {
    auto connection = server.GetClientMap().find(player.owner);
    if (connection == server.GetClientMap().end() || !connection->second.connected)
      gone.push_back(player.owner);
  }
  for (ClientID owner : gone)
    world.RemovePlayer(owner);
  for (auto it = server.GetClientMap().begin(); it != server.GetClientMap().end();) {
    if (!it->second.connected)
      it = server.GetClientMap().erase(it);
    else
      ++it;
  }
}

void GameServer::Step() {
  world.Tick();

  // queue events first so they ride along in this tick's snapshot packets
  for (const GameEvent &event : world.TakeEvents())
    for (const auto &[id, player] : world.players)
      server.SendReliable(player.owner, EventMessage{event});

  auto snapshot = world.Snapshot();
  for (const auto &[id, player] : world.players)
    server.SendPacket(player.owner, snapshot);
}

int GameServer::Shutdown() {
  std::vector<ClientID> ids;
  for (const auto &[id, connection] : server.GetClientMap())
    ids.push_back(id);
  for (ClientID id : ids)
    server.Disconnect(id);
  return static_cast<int>(ids.size());
}
