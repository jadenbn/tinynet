#pragma once
#include "Packets.h"
#include "Socket.h"
#include <chrono>
#include <ctime>

class Address;

static constexpr auto TIMEOUT_MS = std::chrono::milliseconds(10000);

class Server {
public:
  Server(Address serverAddress);
  void initialize();
  void Update();
  int receive(Buffer &out);
  bool send(const Buffer &buffer);
  bool getIsConnected();
  Address getServerAddress();
  Address getClientAddress();

private:
  bool timedOut();
  bool initialPacketReceived = false; // this is ugly i don't like it maybe change when
                              // posible future me
  std::chrono::steady_clock::time_point lastReceivedTime;
  Address clientAddress;
  Address serverAddress;
  Socket serverSocket;
  bool isConnected = false;
  float timeout;
  uint32_t sequenceNumber = 0;
  uint32_t remoteSequenceNumber = 0;
  ReceivedQueue receivedQueue;
  SentQueue sentQueue;
};
