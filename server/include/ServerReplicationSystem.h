#pragma once
#include "Protocol.h"
#include "Server.h"
#include "ServerWorld.h"

// for turning packets into actions
class ServerReplicationSystem {
public:
  ServerReplicationSystem(ServerWorld &c_game);

  bool HandlePacket(const ClientID clientID, Buffer &buff);
  // a reliable message. clients only get to send quick chat
  bool HandleMessage(const ClientID clientID, Buffer &buff);

private:
  ServerWorld &game;

  bool ApplyPlayerInputPacket(const PlayerInputPacket &p, ClientID id);
};
