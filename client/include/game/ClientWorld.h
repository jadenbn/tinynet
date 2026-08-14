// for handling gamestate
#pragma once
#include "Client.h"
#include "ClientReplicationSystem.h"
#include "Player.h"
#include <vector>

class ClientWorld {
public:
  ClientWorld();
  void GameLoop();
  void Init(int argc, char *argv[]);

  std::vector<Player> players;

private:
  Client client;
  ClientReplicationSystem replicationSystem;
};