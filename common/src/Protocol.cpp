#include "Protocol.h"
#include "Packets.h"

void PlayerInputPacket::Serialize(Buffer &buff) {
  WriteChar(buff, PacketType::PlayerInput);
  WriteFloat(buff, this->x);
  WriteFloat(buff, this->y);
}

PlayerInputPacket PlayerInputPacket::deserialize(Buffer &buff) {
  return {ReadFloat(buff), ReadFloat(buff)};
}