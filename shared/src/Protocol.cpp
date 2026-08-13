#include "../shared/include/Protocol.h"
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