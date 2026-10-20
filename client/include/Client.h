#pragma once

#include "Connection.h"
#include "Protocol.h"

class Client {
public:
  Client(Address clientAddress, Address serverAddress);
  void Initialize();
  void UpdateConnection();
  bool GetIsConnected();
  float GetRTT();
  int ReceiveFromServer(Buffer &buff);
  Connection &GetConnection();
  NetworkConditions GetConditions();
  void SetConditions(const NetworkConditions &conditions);

  // tells the server we're leaving, so it frees our slot right away
  void Disconnect();
  // the server told us it's shutting down (or kicked us)
  bool WasDisconnected();

  template <typename Packet> bool SendPacket(const Packet &packet) {
    uint8_t scratch[MAX_PACKET_SIZE];
    Buffer buffer{scratch, 0, 0, sizeof(scratch)};
    packet.Serialize(buffer);
    return serverConnection.Send(socket, buffer);
  }

  // queued on the reliable channel, goes out with the next packet
  template <typename Message> bool SendReliable(const Message &message) {
    uint8_t scratch[ReliableChannel::MAX_MESSAGE_SIZE];
    Buffer buffer{scratch, 0, 0, sizeof(scratch)};
    message.Serialize(buffer);
    return serverConnection.SendReliable(buffer.data, buffer.index);
  }

private:
  Socket socket;
  Address localAddress;
  Connection serverConnection;
  bool disconnectedByServer = false;
};