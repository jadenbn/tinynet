#pragma once
#include "Socket.h"
#include <cstdint>

class Address;

class Server {
public:
  Server(Address serverAddress);
  void initialize(uint32_t protocolHash);
  void Update();
  int receive();
  bool send(char *packet);
  bool getIsConnected();
  Address getServerAddress();
  Address getClientAddress();

private:
  bool timedOut();
  Address clientAddress;
  Address serverAddress;
  Socket serverSocket;
  bool isConnected = false;
  float timeout;
  uint32_t protocolHash;
};
