#include "../shared/include/Protocol.h"
#include "GameTypes.h"
#include "Packets.h"
#include "ServerWorld.h"
#include <cstdint>
#include <utility>
#include "Movement.h"
#include "Quantization.h"
#include <stdexcept>

namespace {
void Require(const Buffer &buff, int bytes) {
  if (buff.index < 0 || bytes < 0 || buff.index > buff.length ||
      bytes > buff.length - buff.index)
    throw std::invalid_argument("truncated game packet");
}
Vec3 ReadVec3(Buffer &buff) {
  return {packets::ReadFloat(buff), packets::ReadFloat(buff), packets::ReadFloat(buff)};
}
void WriteVec3(Buffer &buff, Vec3 v) {
  packets::WriteFloat(buff, v.x);
  packets::WriteFloat(buff, v.y);
  packets::WriteFloat(buff, v.z);
}
bool InWorld(Vec3 pos) {
  return IsFinite(pos) && std::abs(pos.x) <= WORLD_BOUND &&
         std::abs(pos.y) <= WORLD_BOUND && std::abs(pos.z) <= WORLD_BOUND;
}
bool ValidState(const PlayerState &state) {
  float norm = Dot(state.orientation, state.orientation);
  return InWorld(state.pos) && IsFinite(state.velocity) &&
         Dot(state.velocity, state.velocity) <= 36.01f &&
         std::isfinite(norm) && std::abs(norm-1) < 0.01f && state.team <= 1;
}
bool ValidBall(const BallState &ball) {
  // the hardest kick plus a sprint is under 30 m/s; leave plenty of room
  return InWorld(ball.pos) && IsFinite(ball.velocity) &&
         Dot(ball.velocity, ball.velocity) <= 60.0f*60.0f &&
         std::isfinite(ball.spin) && std::abs(ball.spin) <= 50;
}
bool ValidMatch(const MatchState &match) {
  return match.phase <= MatchPhase::FullTime && match.clock <= MATCH_TICKS &&
         match.phaseTicks <= std::max(GOAL_PAUSE_TICKS, FULL_TIME_PAUSE_TICKS);
}
void WritePosition(Buffer &buff, Vec3 pos, bool compressed) {
  if (compressed) {
    packets::WriteShort(buff, PackPosition(pos.x));
    packets::WriteShort(buff, PackPosition(pos.y));
    packets::WriteShort(buff, PackPosition(pos.z));
  } else {
    WriteVec3(buff, pos);
  }
}
Vec3 ReadPosition(Buffer &buff) {
  return {UnpackPosition(packets::ReadShort(buff)),
          UnpackPosition(packets::ReadShort(buff)),
          UnpackPosition(packets::ReadShort(buff))};
}
}

namespace {
// type + tick + player count, the match, then the ball
constexpr size_t HEADER_BYTES = 9, MATCH_BYTES = 7;
constexpr size_t BALL_BYTES = 22, RAW_BALL_BYTES = 28;
constexpr size_t PLAYER_BYTES = 32, RAW_PLAYER_BYTES = 50;

void WriteSnapshot(Buffer &buff, const WorldSnapshot &snapshot, bool compressed) {
  const auto &players = snapshot.players;
  size_t fixedBytes = HEADER_BYTES + MATCH_BYTES + (compressed ? BALL_BYTES : RAW_BALL_BYTES);
  size_t stateBytes = compressed ? PLAYER_BYTES : RAW_PLAYER_BYTES;
  if (players.size() > MAX_PLAYERS || fixedBytes + players.size()*stateBytes >
      static_cast<size_t>(buff.capacity - buff.index))
    throw std::length_error("snapshot exceeds datagram");
  if (!ValidMatch(snapshot.match) || !ValidBall(snapshot.ball))
    throw std::invalid_argument("invalid match or ball");
  packets::WriteChar(buff, PacketType::WorldSnapshot);
  packets::WriteInteger(buff, snapshot.tick);
  packets::WriteInteger(buff, players.size());

  const MatchState &match = snapshot.match;
  packets::WriteChar(buff, match.score[0]);
  packets::WriteChar(buff, match.score[1]);
  packets::WriteChar(buff, static_cast<uint8_t>(match.phase));
  packets::WriteShort(buff, match.clock);
  packets::WriteShort(buff, match.phaseTicks);

  // ball velocity + spin stay floats: the client replays them for prediction
  WritePosition(buff, snapshot.ball.pos, compressed);
  WriteVec3(buff, snapshot.ball.velocity);
  packets::WriteFloat(buff, snapshot.ball.spin);

  for (const auto &[id, state] : players) {
    if (id != state.playerID || !ValidState(state))
      throw std::invalid_argument("invalid player state");
    packets::WriteInteger(buff, id);
    WritePosition(buff, state.pos, compressed);
    WriteVec3(buff, state.velocity);
    if (compressed) {
      packets::WriteInteger(buff, PackQuaternion(state.orientation));
    } else {
      packets::WriteFloat(buff, state.orientation.x);
      packets::WriteFloat(buff, state.orientation.y);
      packets::WriteFloat(buff, state.orientation.z);
      packets::WriteFloat(buff, state.orientation.w);
    }
    packets::WriteInteger(buff, state.lastInput);
    packets::WriteChar(buff, state.team | (state.keeper ? 2 : 0));
    packets::WriteChar(buff, state.kickCooldown);
  }
}

}

void WorldSnapshot::Serialize(Buffer &buff) const { WriteSnapshot(buff, *this, true); }
void WorldSnapshot::SerializeRaw(Buffer &buff) const { WriteSnapshot(buff, *this, false); }

WorldSnapshot WorldSnapshot::deserialize(Buffer &buff) {
  Require(buff, 8);
  WorldSnapshot snapshot;
  snapshot.tick = packets::ReadInteger(buff);
  uint32_t count = packets::ReadInteger(buff);
  if (count > MAX_PLAYERS)
    throw std::invalid_argument("too many players");
  Require(buff, MATCH_BYTES + BALL_BYTES + count*PLAYER_BYTES);

  MatchState &match = snapshot.match;
  match.score = {packets::ReadChar(buff), packets::ReadChar(buff)};
  match.phase = static_cast<MatchPhase>(packets::ReadChar(buff));
  match.clock = packets::ReadShort(buff);
  match.phaseTicks = packets::ReadShort(buff);
  snapshot.ball.pos = ReadPosition(buff);
  snapshot.ball.velocity = ReadVec3(buff);
  snapshot.ball.spin = packets::ReadFloat(buff);
  if (!ValidMatch(match) || !ValidBall(snapshot.ball))
    throw std::invalid_argument("invalid match or ball");

  for (uint32_t i = 0; i < count; ++i) {
    PlayerState state;
    state.playerID = packets::ReadInteger(buff);
    state.pos = ReadPosition(buff);
    state.velocity = ReadVec3(buff);
    state.orientation = UnpackQuaternion(packets::ReadInteger(buff));
    state.lastInput = packets::ReadInteger(buff);
    uint8_t flags = packets::ReadChar(buff);
    state.team = flags & 1;
    state.keeper = flags & 2;
    state.kickCooldown = packets::ReadChar(buff);
    if (flags > 3 || !ValidState(state) || !snapshot.players.emplace(state.playerID, state).second)
      throw std::invalid_argument("invalid player state");
  }
  return snapshot;
}

constexpr int INPUT_BYTES = 23;

void PlayerInputPacket::Serialize(Buffer &buff) const {
  if (inputs.size() > INPUTS_PER_PACKET || 2 + inputs.size()*INPUT_BYTES >
      static_cast<size_t>(buff.capacity - buff.index))
    throw std::length_error("too many inputs");
  packets::WriteChar(buff, PacketType::PlayerInput);
  packets::WriteChar(buff, static_cast<uint8_t>(inputs.size()));
  for (const auto &input : inputs) {
    if (!ValidInput(input))
      throw std::invalid_argument("invalid input");
    packets::WriteInteger(buff, input.sequence);
    WriteVec3(buff, input.direction);
    packets::WriteFloat(buff, input.yaw);
    packets::WriteChar(buff, static_cast<uint8_t>(input.kick.type));
    packets::WriteChar(buff, input.kick.power);
    packets::WriteChar(buff, static_cast<uint8_t>(input.kick.curve));
  }
}

PlayerInputPacket PlayerInputPacket::deserialize(Buffer &buff) {
  Require(buff, 1);
  uint8_t count = packets::ReadChar(buff);
  if (count > INPUTS_PER_PACKET)
    throw std::invalid_argument("too many inputs");
  Require(buff, count*INPUT_BYTES);
  PlayerInputPacket packet;
  for (uint8_t i = 0; i < count; ++i) {
    InputCommand input{packets::ReadInteger(buff), ReadVec3(buff),
                       packets::ReadFloat(buff)};
    input.kick.type = static_cast<KickType>(packets::ReadChar(buff));
    input.kick.power = packets::ReadChar(buff);
    input.kick.curve = static_cast<int8_t>(packets::ReadChar(buff));
    if (!ValidInput(input))
      throw std::invalid_argument("invalid input");
    packet.inputs.push_back(input);
  }
  return packet;
}

void EventMessage::Serialize(Buffer &buff) const {
  packets::WriteChar(buff, static_cast<uint8_t>(event.type));
  packets::WriteInteger(buff, event.player);
  packets::WriteChar(buff, event.team);
  packets::WriteChar(buff, event.value);
  packets::WriteChar(buff, event.score[0]);
  packets::WriteChar(buff, event.score[1]);
}

EventMessage EventMessage::deserialize(Buffer &buff) {
  Require(buff, 9);
  GameEvent event;
  event.type = static_cast<EventType>(packets::ReadChar(buff));
  event.player = packets::ReadInteger(buff);
  event.team = packets::ReadChar(buff);
  event.value = packets::ReadChar(buff);
  event.score = {packets::ReadChar(buff), packets::ReadChar(buff)};
  if (event.type > EventType::QuickChat || event.team > 1 ||
      (event.type == EventType::QuickChat && event.value >= QUICK_CHAT_PHRASES))
    throw std::invalid_argument("invalid event");
  return {event};
}

void ConnectionRequest::Serialize(Buffer &buff) const {
  packets::WriteChar(buff, PacketType::ConnectionRequest);
}

ConnectionRequest ConnectionRequest::deserialize(Buffer &buff) { return {}; }

void Heartbeat::Serialize(Buffer &buff) const {
  packets::WriteChar(buff, PacketType::Heartbeat);
}

Heartbeat Heartbeat::deserialize(Buffer &buff) { return {}; }

void DisconnectPacket::Serialize(Buffer &buff) const {
  packets::WriteChar(buff, PacketType::Disconnect);
}

DisconnectPacket DisconnectPacket::deserialize(Buffer &buff) { return {}; }

void ConnectionAccepted::Serialize(Buffer &buff) const {
  packets::WriteChar(buff, PacketType::ConnectionAccepted);
  packets::WriteInteger(buff, clientID);
}

ConnectionAccepted ConnectionAccepted::deserialize(Buffer &buff) {
  return {packets::ReadInteger(buff)};
};

void ConnectionRejected::Serialize(Buffer &buff) const {
  packets::WriteChar(buff, PacketType::ConnectionRejected);
  packets::WriteChar(buff, reason);
}

ConnectionRejected ConnectionRejected::deserialize(Buffer &buff) {
  return {packets::ReadChar(buff)};
};
