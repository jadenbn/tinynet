#pragma once
#include "Address.h"
#include "Socket.h"
#include <cstdint>

class Client {
public:
  Client(Address clientAddress);
  void connect(Address serverAddress, uint32_t protocolHash);
  int sendPacket(char *packet);
  void Update();                 
  void Update(float timePassed); // todo: for timeout

private:
  Address clientAddress;
  Address serverAddress;
  Socket clientSocket;
  float timeout;
};