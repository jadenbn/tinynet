#include "../shared/include/Protocol.h"
#include "Packets.h"

void WorldSnapshot::Serialize(Buffer &buff) const {
  packets::WriteChar(buff, PacketType::WorldSnapshot);
  packets::WriteFloat(buff, this->player1X);
  packets::WriteFloat(buff, this->player1Y);
}

WorldSnapshot WorldSnapshot::deserialize(Buffer &buff) {
  return {packets::ReadFloat(buff), packets::ReadFloat(buff)};
}

void PlayerInputPacket::Serialize(Buffer &buff) const {
  packets::WriteChar(buff, PacketType::PlayerInput);
  packets::WriteFloat(buff, this->x);
  packets::WriteFloat(buff, this->y);
}

PlayerInputPacket PlayerInputPacket::deserialize(Buffer &buff) {
  return {packets::ReadFloat(buff), packets::ReadFloat(buff)};
}

void ConnectionRequest::Serialize(Buffer &buff) const {
  packets::WriteChar(buff, PacketType::ConnectionRequest);
}

ConnectionRequest ConnectionRequest::deserialize(Buffer &buff) { return {}; }

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