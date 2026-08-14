// for handling gamestate
#pragma once
#include "../shared/include/GameTypes.h"
#include "Client.h"
#include "ClientReplicationSystem.h"
#include <vector>

class ClientGame {
public:
  void GameLoop();
  void Init(int argc, char *argv[]);

  std::vector<Player> players;

private:
  Client client;
  ClientReplicationSystem replicationSystem;
};