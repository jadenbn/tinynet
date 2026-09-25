#include "../common/include/NetworkSimulator.h"
#include <cassert>
#include <chrono>
#include <iostream>
#include <thread>

const Address address(127, 0, 0, 1, 3000);

void test_perfect_network_is_inactive() {
  NetworkSimulator sim;
  assert(!sim.IsActive());
}

void test_latency_holds_packets() {
  NetworkSimulator sim;
  sim.conditions.latency = 50.0f;

  char data[] = "ping";
  char out[16];
  Address from;
  sim.Push(address, data, sizeof(data));

  // too early
  assert(sim.Pop(from, out, sizeof(out)) == 0);

  std::this_thread::sleep_for(std::chrono::milliseconds(60));
  assert(sim.Pop(from, out, sizeof(out)) == sizeof(data));
  assert(from == address);
}

void test_full_loss_drops_everything() {
  NetworkSimulator sim;
  sim.conditions.loss = 1.0f;

  char data[] = "ping";
  char out[16];
  Address from;
  for (int i = 0; i < 100; i++) {
    sim.Push(address, data, sizeof(data));
  }

  assert(sim.Pop(from, out, sizeof(out)) == 0);
}

void test_duplicate_delivers_twice() {
  NetworkSimulator sim;
  sim.conditions.duplicate = 1.0f;

  char data[] = "ping";
  char out[16];
  Address from;
  sim.Push(address, data, sizeof(data));

  assert(sim.Pop(from, out, sizeof(out)) > 0);
  assert(sim.Pop(from, out, sizeof(out)) > 0);
  assert(sim.Pop(from, out, sizeof(out)) == 0);
}

int main() {
  test_perfect_network_is_inactive();
  test_latency_holds_packets();
  test_full_loss_drops_everything();
  test_duplicate_delivers_twice();
  std::cout << "All tests passed!" << std::endl;
}
