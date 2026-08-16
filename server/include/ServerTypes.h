#include "GameTypes.h"
#include "Server.h"

namespace ServerTypes {

struct ServerPlayer {
  ClientID owner;
  NetworkSafeVector2 position;
  PlayerID playerID;
};

} // namespace ServerTypes