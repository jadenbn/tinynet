// for handling gamestate
#pragma once
#include "Client.h"
#include "ClientReplicationSystem.h"
#include "game/ClientWorld.h"

class ClientGame {
public:
  ClientGame(Address &clientAddress, Address &serverAddress);
  void GameLoop();
  void NetworkInit();
  void GameInit();
  void HandleNetwork();
  void HandleInput();

private:
  Client client;
  ClientWorld clientWorld;
  ClientReplicationSystem replicationSystem;
};