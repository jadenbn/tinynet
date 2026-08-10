#pragma once
#include "Address.h"
#include "Socket.h"
#include <cstdint>

class Client {
public:
  Client(Address clientAddress);
  void initialize(Address serverAddress, uint32_t protocolHash);
  int send(char *packet);
  void Update();
  void Update(float timePassed); // todo: for timeout

private:
  bool timedOut();
  Address clientAddress;
  Address serverAddress;
  Socket clientSocket;
  float timeout;
  bool isConnected;
  uint32_t protocolHash;
};