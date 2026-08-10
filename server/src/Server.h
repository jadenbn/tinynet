#pragma once
#include "Socket.h"
#include <chrono>
#include <cstdint>
#include <ctime>

class Address;

static constexpr auto TIMEOUT_MS = std::chrono::milliseconds(10000);

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
  bool initialPacketReceived; // this is ugly i don't like it maybe change when
                              // posible future me
  std::chrono::steady_clock::time_point lastReceivedTime;
  Address clientAddress;
  Address serverAddress;
  Socket serverSocket;
  bool isConnected;
  float timeout;
  uint32_t protocolHash;
};
