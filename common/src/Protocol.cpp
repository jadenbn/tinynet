#include "Protocol.h"
#include "Packets.h"

struct PlayerInputPacket::PlayerInputPacket {
  float x;
  float y;

  void Serialize(Buffer &buff) {
    WriteChar(buff, PacketType::PlayerInput);
    WriteFloat(buff, this->x);
    WriteFloat(buff, this->y);
  }
  const PlayerInputPacket deserialize(Buffer buff) {
    return {ReadFloat(buff), ReadFloat(buff)};
  }
};