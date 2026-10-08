#include "ServerWorld.h"
#include "Movement.h"

bool ServerWorld::AddPlayer(ClientID clientID) {
  if (players.size() >= MAX_PLAYERS)
    return false;
  PlayerState state;
  state.playerID = clientID;
  state.pos = {static_cast<float>(clientID % 4) * 2 - 3, 0, 0};
  if (!players.try_emplace(clientID, ServerTypes::ServerPlayer{clientID, state, {}}).second)
    return false;
  clientToPlayer.emplace(clientID, clientID);
  return true;
}

bool ServerWorld::QueueInputs(ClientID clientID, const PlayerInputPacket &packet) {
  auto mapping = clientToPlayer.find(clientID);
  if (mapping == clientToPlayer.end() || packet.inputs.size() > INPUTS_PER_PACKET)
    return false;
  auto &player = players.at(mapping->second);
  for (const auto &input : packet.inputs)
    if (!ValidInput(input))
      return false;
  for (const auto &input : packet.inputs) {
    uint32_t ahead = input.sequence - player.state.lastInput;
    if (ahead > 0 && ahead <= MAX_PENDING_INPUTS)
      player.pending.try_emplace(input.sequence, input);
  }
  return true;
}

void ServerWorld::Tick() {
  ++tick;
  for (auto &[id, player] : players) {
    auto next = player.pending.find(player.state.lastInput + 1);
    // shortcut: missing commands pause this player, add tick-aligned fallback for long outages.
    if (next == player.pending.end())
      continue;
    SimulateMovement(player.state, next->second);
    player.pending.erase(next);
  }
}

WorldSnapshot ServerWorld::Snapshot() const {
  WorldSnapshot snapshot;
  snapshot.tick = tick;
  for (const auto &[id, player] : players)
    snapshot.players.emplace(id, player.state);
  return snapshot;
}
