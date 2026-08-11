#pragma once
#include "Address.h"
#include "Packets.h"
#include "Socket.h"
#include <chrono>

class Client {
public:
  Client(Address clientAddress);
  void initialize(Address serverAddress);
  void Update();
  int receive(Buffer &out);
  bool send(const Buffer &buffer);
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
  bool isConnected;
  float timeout;
  uint32_t sequenceNumber = 0;
  uint32_t remoteSequenceNumber = 0;
  ReceivedQueue receivedQueue;
  SentQueue sentQueue;
};