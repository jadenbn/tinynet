#pragma once

#include "Connection.h"
class Client {
public:
  Client(Address clientAddress, Address serverAddress);
  void Initialize(Address serverAddress);
  void UpdateConnection();
  Address GetAddress();
  Address GetRemoteAddress();
  bool GetIsConnected();
  float GetRTT();
  int ReceiveFromServer(Buffer &buff);

  template <typename Packet> bool SendPacket(const Packet &packet) {
    uint8_t scratch[MAX_PACKET_SIZE];
    Buffer buffer{scratch, 0, sizeof(scratch)};
    packet.Serialize(buffer);
    return serverConnection.Send(socket, buffer);
  }

private:
  Socket socket;
  Address localAddress;
  Connection serverConnection;
};