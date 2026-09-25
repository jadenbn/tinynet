#pragma once
#include "Address.h"
#include <chrono>
#include <cstdint>
#include <random>
#include <vector>

// fakes a bad network so we can watch the protocol deal with it. lives under
// the socket, so nothing above it (acks, rtt, the game) can tell the
// difference between this and a genuinely bad connection

struct NetworkConditions {
  float latency = 0.0f;   // ms, added in each direction
  float jitter = 0.0f;    // ms, random +/- on top of latency
  float loss = 0.0f;      // 0-1 chance a packet never arrives
  float duplicate = 0.0f; // 0-1 chance a packet arrives twice
};

class NetworkSimulator {
public:
  NetworkConditions conditions;

  // true while we're simulating, or still holding packets from before
  bool IsActive() const;

  // rolls for loss/duplication, then holds the packet until it "arrives"
  void Push(const Address &address, const void *data, int size);

  // copies out the next packet that's arrived, 0 if nothing's ready yet
  int Pop(Address &address, void *data, int size);

private:
  struct DelayedPacket {
    std::chrono::steady_clock::time_point deliverAt;
    Address address;
    std::vector<uint8_t> data;
  };

  // allocates per packet; fine for a debug tool, wouldn't be for the real path
  std::vector<DelayedPacket> queue;
  std::mt19937 rng{std::random_device{}()};

  float Roll(); // 0-1
  void Schedule(const Address &address, const void *data, int size);
};
