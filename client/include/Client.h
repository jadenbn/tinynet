#pragma once

#include "Connection.h"
class Client {
public:
  Client(Address clientAddress);
  Connection connection = Connection();
  void initialize(Address serverAddress);
  void Update();
  Address GetAddress();
  Address GetRemoteAddress();
  bool GetIsConnected();

  template <typename Packet> bool SendPacket(const Packet &packet) {
    uint8_t scratch[MAX_PACKET_SIZE];
    Buffer buffer{scratch, 0, sizeof(scratch)};
    packet.Serialize(buffer);
    return connection.Send(buffer);
  }

private:
};