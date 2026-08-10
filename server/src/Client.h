#pragma once
#include "Address.h"
#include "Socket.h"
#include <chrono>
#include <cstdint>

class Client {
public:
  Client(Address clientAddress);
  void initialize(Address serverAddress, uint32_t protocolHash);
  void Update();
  int receive();
  bool send(char *packet);
  bool getIsConnected();
  Address getClientAddress();
  Address getServerAddress();

private:
  bool timedOut();
  bool initialPacketReceived;

  std::chrono::steady_clock::time_point lastReceivedTime;
  Address clientAddress;
  Address serverAddress;
  Socket clientSocket;
  float timeout;
  bool isConnected;
  uint32_t protocolHash;
};