#pragma once
#include "Connection.h"
#include "Packets.h"

class Address;

class Server {
public:
  Server(Address serverAddress);
  void initialize();
  void Update();

  Address GetAddress();
  Address GetRemoteAddress();
  bool GetIsConnected();

  template <typename Packet> bool SendPacket(const Packet &packet) {
    uint8_t scratch[MAX_PACKET_SIZE];
    Buffer buff = Buffer(scratch, 0, sizeof(scratch));
    packet.Serialize(buff);
    return connection.Send(buff);
  }

private:
  Connection connection;
};
