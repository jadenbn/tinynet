#pragma once
#include "../shared/include/Protocol.h"
#include "ClientGame.h"

// for turning packets into actions
class ClientReplicationSystem {
public:
  ClientReplicationSystem(ClientGame &c_game);

  bool HandlePacket(Buffer &buff);

private:
  ClientGame &game;

  bool ApplyPlayerInputPacket(const PlayerInputPacket &p);
  bool ApplyWorldSnapshot(const WorldSnapshot &p);
};