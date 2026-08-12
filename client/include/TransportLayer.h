#include "Game.h"
#include "Protocol.h"

// for turning packets into actions
class TransportLayer {
public:
  TransportLayer(Game &c_game);

  bool HandlePacket(Buffer &buff);

private:
  Game &game;

  bool ApplyPlayerInputPacket(const PlayerInputPacket &p);
};