#pragma once
#include "Address.h"
#include "Packets.h"
#include "ReliableChannel.h"
#include "Socket.h"
#include <chrono>

// TODO: congestion avoidance; perhaps at a higher level of abstraction, though,
// like

// running totals since the connection started. rates (kbps, loss %) are up to
// whoever's reading them, by comparing two snapshots of these over time
struct ConnectionStats {
  uint64_t packetsSent = 0;
  uint64_t packetsReceived = 0;
  uint64_t bytesSent = 0; // includes our 16 byte header, not udp/ip's
  uint64_t bytesReceived = 0;
  uint64_t packetsAcked = 0;
  uint64_t packetsLost = 0;
  uint64_t duplicatesReceived = 0;
  uint64_t outOfOrderReceived = 0;
  float rtt = 0.0f; // ms, smoothed

  // reliable messages
  uint64_t messagesSent = 0;
  uint64_t messagesResent = 0;
  uint64_t messagesReceived = 0;
  int messagesUnacked = 0;
};

// represents an endpoint; client has one, server has many
class Connection {
public:
  Connection(Address remoteAddress);
  int Receive(Socket &sock, Buffer &out);
  bool Send(Socket &sock, const Buffer &buffer);
  Address GetAddress();
  float GetRTT();
  bool connected = false;
  bool timedOut();
  int ProcessReceived(Buffer &buff);

  // reliable, ordered messages, carried on whatever packets we're sending
  // anyway. see ReliableChannel.h
  bool SendReliable(const void *data, int size);
  int ReceiveReliable(void *out, int capacity);

  ConnectionStats GetStats();
  uint32_t GetSequenceNumber();       // the next one we'll send
  uint32_t GetRemoteSequenceNumber(); // newest one we've received
  PacketStatus GetSentStatus(uint32_t sequenceNumber);
  bool WasReceived(uint32_t sequenceNumber);

private:
  ConnectionStats stats;
  float rtt;
  bool initialPacketReceived = false;
  std::chrono::steady_clock::time_point lastReceivedTime;
  Address remoteAddress = Address();
  // starts at 1: a header acks its newest packet with no "nothing yet" flag,
  // so before the other side has heard from us it says "ack 0". if 0 were a
  // real packet, that would falsely ack it
  uint32_t sequenceNumber = 1;
  uint32_t remoteSequenceNumber = 0; // 0 = nothing received yet
  ReceivedQueue receivedQueue;
  SentQueue sentQueue;
  ReliableChannel reliable;
};