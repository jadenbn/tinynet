#include "../shared/include/Protocol.h"
#include "Connection.h"
#include "Packets.h"

void WorldSnapshot::Serialize(Buffer &buff) const {
  WriteChar(buff, PacketType::WorldSnapshot);
  WriteFloat(buff, this->player1X);
  WriteFloat(buff, this->player1Y);
}

WorldSnapshot WorldSnapshot::deserialize(Buffer &buff) {
  return {ReadFloat(buff), ReadFloat(buff)};
}

void PlayerInputPacket::Serialize(Buffer &buff) const {
  WriteChar(buff, PacketType::PlayerInput);
  WriteFloat(buff, this->x);
  WriteFloat(buff, this->y);
}

PlayerInputPacket PlayerInputPacket::deserialize(Buffer &buff) {
  return {ReadFloat(buff), ReadFloat(buff)};
}

void ConnectionRequest::Serialize(Buffer &buff) const {
  WriteChar(buff, PacketType::ConnectionRequest);
  WriteChar(buff, status);
  WriteInteger(buff, clientID);
}

ConnectionRequest ConnectionRequest::deserialize(Buffer &buff) {
  return {ReadChar(buff), ReadInteger(buff)};
}