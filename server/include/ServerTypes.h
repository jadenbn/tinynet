#pragma once
#include "GameTypes.h"
#include "Server.h"
#include <map>

namespace ServerTypes {
struct ServerPlayer {
  ClientID owner;
  PlayerState state;
  std::map<uint32_t, InputCommand> pending;
  uint32_t nextChatTick = 0; // quick chat rate limit
  bool bot = false;          // a keeper, no client behind it
};
}
