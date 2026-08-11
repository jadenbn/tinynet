#pragma once
#include "Address.h"
#include "Packets.h"
#include "Socket.h"
#include <chrono>

class Connection {
public:
  Connection(Address address);
  bool open(Address address);
  void connect(Address remoteAddress); // should only be used by a client!
  void Update();
  int receive(Buffer &out);
  bool send(const Buffer &buffer);
  bool getIsConnected();
  Address getClientAddress();
  Address getServerAddress();

private:
  bool timedOut();
  bool initialPacketReceived = false;

  std::chrono::steady_clock::time_point lastReceivedTime;
  Address address;
  Address remoteAddress;
  Socket socket;
  bool isConnected = false;
  uint32_t sequenceNumber = 0;
  uint32_t remoteSequenceNumber = 0;
  ReceivedQueue receivedQueue;
  SentQueue sentQueue;
};