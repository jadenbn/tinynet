#pragma once
#include "Server.h"
#include "ServerReplicationSystem.h"
#include "ServerWorld.h"

// the whole server side of the game: the connections, the match, and the
// glue between them. main() just calls these on a fixed timestep
class GameServer {
public:
  explicit GameServer(Address address);
  void Initialize();

  // everything that's arrived: inputs, new players, reliable messages. also
  // drops players whose connection is gone
  void ReceivePackets();

  // one 60Hz tick: simulate, send events out, then snapshots
  void Step();

  // tells every client we're going
  int Shutdown();

  ServerWorld &World() { return world; }
  Server &Connections() { return server; }

private:
  Server server;
  ServerWorld world;
  ServerReplicationSystem replication;
};
