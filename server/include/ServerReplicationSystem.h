#pragma once
#include "../server/include/Server.h"
#include "../shared/include/Protocol.h"
#include "ServerGame.h"

// for turning packets into actions
class ServerReplicationSystem {
public:
  ServerReplicationSystem(ServerGame &c_game);

  bool HandlePacket(const ClientID clientID, Buffer &buff);

private:
  ServerGame &game;

  bool ApplyPlayerInputPacket(const PlayerInputPacket &p);
};