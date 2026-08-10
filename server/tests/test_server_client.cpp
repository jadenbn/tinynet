#include "../src/Client.h"
#include "../src/Server.h"
#include <cassert>
#include <chrono>
#include <iostream>
#include <thread>

void test_server_socket() {
  Server s(Address(127, 0, 0, 1, 3000));
  assert(s.getServerAddress().GetPort() == 3000);
  assert(s.getServerAddress().GetA() == 127);
}

// brutal test name </3
// fix assertions later when we have a packet queue
void test_server_listens_and_receives_packets_from_client() {
  Address serverAddress = Address(127, 0, 0, 1, 3000);
  Server server = Server(serverAddress);
  server.initialize();

  Client client = Client(Address(127, 0, 0, 1, 3001));
  client.initialize(serverAddress);

  // run for 2 * timeout duration
  for (int i = 0; i < ((TIMEOUT_MS.count() * 2) / 10); i++) {
    client.Update();
    server.Update();

    // establish connection
    if (i < 200) {
      client.send(0);
      server.send(0);
    }

    // confirm connection between 100ms and 1s
    if (i > 10 && i < 100) {
      assert(client.getIsConnected());
      assert(server.getIsConnected());
      assert(client.getServerAddress() == server.getServerAddress());
    }

    // confirm both timeout
    if (i > (TIMEOUT_MS.count() + 100)) {
      assert(!client.getIsConnected());
      assert(!server.getIsConnected());
      assert(server.getClientAddress() == Address());
    }

    std::this_thread::sleep_for(std::chrono::milliseconds(10));
  }
}

int main() {
  test_server_socket();
  test_server_listens_and_receives_packets_from_client();
  std::cout << "All tests passed!" << std::endl;
}