#pragma once
#include "../shared/include/Protocol.h"
#include "ServerGame.h"

// for turning packets into actions
class ServerReplicationSystem {
public:
  ServerReplicationSystem(ServerGame &c_game);

  bool HandlePacket(Buffer &buff);

private:
  ServerGame &game;

  bool ApplyPlayerInputPacket(const PlayerInputPacket &p);
};