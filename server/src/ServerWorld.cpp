#include "ServerWorld.h"
#include "Ball.h"
#include "Goalkeeper.h"
#include "Movement.h"
#include <algorithm>

namespace {

// rows of five across our own half: middle, then alternating either side
Vec3 SpawnPoint(uint8_t team, int index) {
  float side = team == 0 ? -1.0f : 1.0f;
  int slot = index % 5;
  float across = static_cast<float>((slot + 1) / 2) * (slot % 2 ? -3.5f : 3.5f);
  float depth = 6 + static_cast<float>(index / 5) * 4;
  return {side * depth, 0, across};
}

Quat FacingOpponents(uint8_t team) {
  constexpr float quarterTurn = 1.5707963267948966f;
  return Yaw(team == 0 ? quarterTurn : -quarterTurn);
}

} // namespace

bool ServerWorld::AddPlayer(ClientID clientID) {
  if (players.size() >= MAX_PLAYERS || players.contains(clientID))
    return false;

  int teamSize[2] = {};
  for (const auto &[id, player] : players)
    if (!player.bot)
      teamSize[player.state.team]++;
  uint8_t team = teamSize[1] < teamSize[0] ? 1 : 0;

  PlayerState state;
  state.playerID = clientID;
  state.team = team;
  state.pos = SpawnPoint(team, teamSize[team]);
  state.orientation = FacingOpponents(team);
  players.emplace(clientID, ServerTypes::ServerPlayer{clientID, state, {}});
  clientToPlayer.emplace(clientID, clientID);
  events.push_back({EventType::PlayerJoined, clientID, team});
  return true;
}

void ServerWorld::AddKeepers() {
  for (uint8_t team : {0, 1}) {
    PlayerState state;
    state.playerID = KEEPER_IDS[team];
    state.team = team;
    state.keeper = true;
    state.pos = KeeperTarget(team, ball);
    state.orientation = FacingOpponents(team);
    players.try_emplace(state.playerID,
                        ServerTypes::ServerPlayer{UINT32_MAX, state, {}, 0, true});
  }
}

void ServerWorld::RemovePlayer(ClientID clientID) {
  auto mapping = clientToPlayer.find(clientID);
  if (mapping == clientToPlayer.end())
    return;
  auto player = players.find(mapping->second);
  events.push_back({EventType::PlayerLeft, mapping->second, player->second.state.team});
  if (lastTouch == mapping->second)
    lastTouch = NO_PLAYER;
  players.erase(player);
  clientToPlayer.erase(mapping);
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

bool ServerWorld::QuickChat(ClientID clientID, uint8_t phrase) {
  constexpr uint32_t CHAT_COOLDOWN_TICKS = 30;
  auto mapping = clientToPlayer.find(clientID);
  if (mapping == clientToPlayer.end() || phrase >= QUICK_CHAT_PHRASES)
    return false;
  auto &player = players.at(mapping->second);
  if (tick < player.nextChatTick)
    return false;
  player.nextChatTick = tick + CHAT_COOLDOWN_TICKS;
  // who said it comes from the connection, never from the message itself
  events.push_back({EventType::QuickChat, mapping->second, player.state.team, phrase});
  return true;
}

void ServerWorld::Tick() {
  ++tick;
  recentBalls.push_back(ball);
  while (recentBalls.size() > keeper::REACTION_TICKS)
    recentBalls.pop_front();

  for (auto &[id, player] : players) {
    if (player.bot) {
      InputCommand input = KeeperInput(player.state, recentBalls.front());
      SimulateMovement(player.state, input);
      if (TryKick(player.state, input.kick, ball))
        lastTouch = id;
      continue;
    }
    auto next = player.pending.find(player.state.lastInput + 1);
    // shortcut: missing commands pause this player, add tick-aligned fallback for long outages.
    if (next == player.pending.end())
      continue;
    SimulateMovement(player.state, next->second);
    if (TryKick(player.state, next->second.kick, ball))
      lastTouch = id;
    player.pending.erase(next);
  }

  // the client predicts the ball in this same order: everyone moves (and
  // maybe kicks), then the ball moves, then it bumps into whoever's there
  SimulateBall(ball);
  for (auto &[id, player] : players)
    if (CollidePlayerBall(player.state, ball))
      lastTouch = id;

  UpdateMatch();
}

void ServerWorld::UpdateMatch() {
  if (match.phase != MatchPhase::Playing) {
    if (match.phaseTicks > 0 && --match.phaseTicks > 0)
      return;
    if (match.phase == MatchPhase::FullTime)
      match = MatchState{};
    match.phase = MatchPhase::Playing;
    Kickoff();
    return;
  }

  // +x is team 0's target. a ball in your own net is still a goal for them
  if (int side = GoalLineCrossed(ball)) {
    uint8_t team = side > 0 ? 0 : 1;
    match.score[team] = std::min<int>(match.score[team] + 1, 99);
    match.phase = MatchPhase::Goal;
    match.phaseTicks = GOAL_PAUSE_TICKS;
    auto scorer = players.find(lastTouch);
    bool ownGoal = scorer != players.end() && scorer->second.state.team != team;
    events.push_back({EventType::Goal, lastTouch, team,
                      static_cast<uint8_t>(ownGoal), match.score});
    return;
  }

  if (match.clock > 0 && --match.clock == 0) {
    match.phase = MatchPhase::FullTime;
    match.phaseTicks = FULL_TIME_PAUSE_TICKS;
    uint8_t winner = match.score[1] > match.score[0] ? 1 : 0;
    // value says whether it was a draw
    events.push_back({EventType::FullTime, NO_PLAYER, winner,
                      static_cast<uint8_t>(match.score[0] == match.score[1]),
                      match.score});
  }
}

void ServerWorld::Kickoff() {
  ball = BallState{};
  lastTouch = NO_PLAYER;
  // same spots every time: lowest id on each team gets the first one
  std::vector<PlayerID> ids;
  for (const auto &[id, player] : players)
    ids.push_back(id);
  std::sort(ids.begin(), ids.end());
  int index[2] = {};
  recentBalls.clear();
  for (PlayerID id : ids) {
    PlayerState &state = players.at(id).state;
    if (state.keeper) {
      state.pos = KeeperTarget(state.team, ball);
      state.velocity = {};
      state.orientation = FacingOpponents(state.team);
      continue;
    }
    state.pos = SpawnPoint(state.team, index[state.team]++);
    state.velocity = {};
    state.orientation = FacingOpponents(state.team);
  }
}

std::vector<GameEvent> ServerWorld::TakeEvents() {
  return std::exchange(events, {});
}

WorldSnapshot ServerWorld::Snapshot() const {
  WorldSnapshot snapshot;
  snapshot.tick = tick;
  snapshot.match = match;
  snapshot.ball = ball;
  for (const auto &[id, player] : players)
    snapshot.players.emplace(id, player.state);
  return snapshot;
}
