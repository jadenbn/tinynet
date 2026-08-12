#pragma once
#include "Address.h"
#include "Packets.h"
#include "Socket.h"
#include <chrono>

// TODO: congestion avoidance; perhaps at a higher level of abstraction, though,
// like

class Connection {
public:
  Connection(Address address);
  Connection();
  bool Open();
  void Connect(Address remoteAddress); // should only be used by a client!
  void Update();
  int Receive(Buffer &out);
  bool Send(const Buffer &buffer);
  bool GetIsConnected();
  Address GetAddress();
  void SetAddress(Address address);
  Address GetRemoteAddress();
  float GetRTT();

private:
  bool timedOut();
  bool initialPacketReceived = false;

  std::chrono::steady_clock::time_point lastReceivedTime;
  Address address = Address();
  Address remoteAddress = Address();
  Socket socket;
  bool isConnected = false;
  uint32_t sequenceNumber = 0;
  uint32_t remoteSequenceNumber = 0;
  ReceivedQueue receivedQueue;
  SentQueue sentQueue;
};