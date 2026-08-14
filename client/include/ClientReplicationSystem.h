#pragma once
#include "Client.h"
#include "Protocol.h"

class ClientWorld;
// for turning packets into actions
class ClientReplicationSystem {
public:
  ClientReplicationSystem(ClientWorld &c_game, Client &client);

  bool HandlePacket(Buffer &buff);

private:
  ClientWorld &game;
  Client &client;

  bool ApplyPlayerInputPacket(const PlayerInputPacket &p);
  bool ApplyWorldSnapshot(const WorldSnapshot &p);
};