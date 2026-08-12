#include "../shared/include/Protocol.h"
#include "Game.h"

// for turning packets into actions
class ClientReplicationSystem {
public:
  ClientReplicationSystem(Game &c_game);

  bool HandlePacket(Buffer &buff);

private:
  Game &game;

  bool ApplyPlayerInputPacket(const PlayerInputPacket &p);
};