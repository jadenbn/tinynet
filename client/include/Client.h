#pragma once

#include "Connection.h"
class Client {
public:
  Client(Address clientAddress);
  void initialize(Address serverAddress);
  void UpdateConnection();
  Address GetAddress();
  Address GetRemoteAddress();
  bool GetIsConnected();
  float GetRTT();
  int Receive(Buffer &buff);

  template <typename Packet> bool SendPacket(const Packet &packet) {
    uint8_t scratch[MAX_PACKET_SIZE];
    Buffer buffer{scratch, 0, sizeof(scratch)};
    packet.Serialize(buffer);
    return connection.Send(buffer);
  }

private:
  Connection connection;
};