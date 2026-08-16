#pragma once
#include "Protocol.h"
#include "Server.h"
#include "ServerWorld.h"

// for turning packets into actions
class ServerReplicationSystem {
public:
  ServerReplicationSystem(ServerWorld &c_game);

  bool HandlePacket(const ClientID clientID, Buffer &buff);

private:
  ServerWorld &game;

  bool ApplyPlayerInputPacket(const PlayerInputPacket &p);
  bool ApplyConnectionRequest(const ConnectionRequest &p);
};