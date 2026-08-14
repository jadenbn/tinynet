#pragma once
#include "Protocol.h"

class ClientWorld;
// for turning packets into actions
class ClientReplicationSystem {
public:
  ClientReplicationSystem(ClientWorld &c_game);

  bool HandlePacket(Buffer &buff);

private:
  ClientWorld &game;

  bool ApplyPlayerInputPacket(const PlayerInputPacket &p);
  bool ApplyWorldSnapshot(const WorldSnapshot &p);
};