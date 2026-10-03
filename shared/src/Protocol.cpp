#include "../shared/include/Protocol.h"
#include "GameTypes.h"
#include "Packets.h"
#include "ServerWorld.h"
#include <cstdint>
#include <utility>
#include "Movement.h"
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
bool ValidState(const PlayerState &state) {
  float norm = Dot(state.orientation, state.orientation);
  return IsFinite(state.pos) && IsFinite(state.velocity) &&
         std::abs(state.pos.x) <= WORLD_BOUND &&
         std::abs(state.pos.y) <= WORLD_BOUND &&
         std::abs(state.pos.z) <= WORLD_BOUND &&
         Dot(state.velocity, state.velocity) <= 36.01f &&
         std::isfinite(norm) && std::abs(norm-1) < 0.01f;
}
}

void WorldSnapshot::Serialize(Buffer &buff) const {
  if (players.size() > MAX_PLAYERS || 9 + players.size()*48 >
      static_cast<size_t>(buff.capacity - buff.index))
    throw std::length_error("snapshot exceeds datagram");
  packets::WriteChar(buff, PacketType::WorldSnapshot);
  packets::WriteInteger(buff, tick);
  packets::WriteInteger(buff, players.size());
  for (const auto &[id, state] : players) {
    if (id != state.playerID || !ValidState(state))
      throw std::invalid_argument("invalid player state");
    packets::WriteInteger(buff, id);
    WriteVec3(buff, state.pos);
    WriteVec3(buff, state.velocity);
    packets::WriteFloat(buff, state.orientation.x);
    packets::WriteFloat(buff, state.orientation.y);
    packets::WriteFloat(buff, state.orientation.z);
    packets::WriteFloat(buff, state.orientation.w);
    packets::WriteInteger(buff, state.lastInput);
  }
}

WorldSnapshot WorldSnapshot::deserialize(Buffer &buff) {
  Require(buff, 8);
  WorldSnapshot snapshot;
  snapshot.tick = packets::ReadInteger(buff);
  uint32_t count = packets::ReadInteger(buff);
  if (count > MAX_PLAYERS)
    throw std::invalid_argument("too many players");
  Require(buff, count*48);
  for (uint32_t i = 0; i < count; ++i) {
    PlayerState state;
    state.playerID = packets::ReadInteger(buff);
    state.pos = ReadVec3(buff);
    state.velocity = ReadVec3(buff);
    state.orientation = {packets::ReadFloat(buff), packets::ReadFloat(buff),
                         packets::ReadFloat(buff), packets::ReadFloat(buff)};
    state.lastInput = packets::ReadInteger(buff);
    if (!ValidState(state) || !snapshot.players.emplace(state.playerID, state).second)
      throw std::invalid_argument("invalid player state");
  }
  return snapshot;
}

void PlayerInputPacket::Serialize(Buffer &buff) const {
  if (inputs.size() > INPUTS_PER_PACKET || 2 + inputs.size()*20 >
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
  }
}

PlayerInputPacket PlayerInputPacket::deserialize(Buffer &buff) {
  Require(buff, 1);
  uint8_t count = packets::ReadChar(buff);
  if (count > INPUTS_PER_PACKET)
    throw std::invalid_argument("too many inputs");
  Require(buff, count*20);
  PlayerInputPacket packet;
  for (uint8_t i = 0; i < count; ++i) {
    InputCommand input{packets::ReadInteger(buff), ReadVec3(buff),
                       packets::ReadFloat(buff)};
    if (!ValidInput(input))
      throw std::invalid_argument("invalid input");
    packet.inputs.push_back(input);
  }
  return packet;
}

void ConnectionRequest::Serialize(Buffer &buff) const {
  packets::WriteChar(buff, PacketType::ConnectionRequest);
}

ConnectionRequest ConnectionRequest::deserialize(Buffer &buff) { return {}; }

void Heartbeat::Serialize(Buffer &buff) const {
  packets::WriteChar(buff, PacketType::Heartbeat);
}

Heartbeat Heartbeat::deserialize(Buffer &buff) { return {}; }

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
