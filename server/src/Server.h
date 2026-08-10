#pragma once
#include "Socket.h"
#include <cstdint>

class Address;

class Server {
public:
  Server(Address serverAddress);
  void listenForClient(uint32_t protocolHash);
  int receive();
  bool send(char *packet);
  bool isConnected();
  Address getServerAddress();
  Address getClientAddress();

private:
  Address clientAddress;
  Address serverAddress;
  Socket serverSocket;
  bool connectionEstablished = false;
  uint32_t protocolHash;
};
