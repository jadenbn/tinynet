#include "../shared/include/Protocol.h"
#include "Packets.h"
#include <iostream>

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
}

ConnectionRequest ConnectionRequest::deserialize(Buffer &buff) { return {}; }

void ConnectionAccepted::Serialize(Buffer &buff) const {
  WriteChar(buff, PacketType::ConnectionAccepted);
  WriteInteger(buff, clientID);
}

ConnectionAccepted ConnectionAccepted::deserialize(Buffer &buff) {
  return {ReadInteger(buff)};
};

void ConnectionRejected::Serialize(Buffer &buff) const {
  WriteChar(buff, PacketType::ConnectionRejected);
  WriteChar(buff, reason);
}

ConnectionRejected ConnectionRejected::deserialize(Buffer &buff) {
  return {ReadChar(buff)};
};