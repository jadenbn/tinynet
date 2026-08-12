#pragma once
#include "Address.h"
#include "Packets.h"
#include "Socket.h"
#include <chrono>

// TODO: congestion avoidance; perhaps at a higher level of abstraction, though,
// like

typedef uint32_t ClientID;

// represents an endpoint; client has one, server has many
class Connection {
public:
  Connection(Address address);
  int Receive(Socket &sock, Buffer &out);
  bool Send(Socket &sock, const Buffer &buffer);
  Address GetAddress();
  float GetRTT();

private:
  float rtt;
  bool timedOut();
  bool initialPacketReceived = false;
  std::chrono::steady_clock::time_point lastReceivedTime;
  Address address = Address();
  Address remoteAddress = Address();
  bool connected = false;
  uint32_t sequenceNumber = 0;
  uint32_t remoteSequenceNumber = 0;
  ReceivedQueue receivedQueue;
  SentQueue sentQueue;
};