#include "../src/Client.h"
#include "../src/Server.h"
#include <cassert>
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
  server.initialize(0x12345678);

  Client client = Client(Address(127, 0, 0, 1, 3001));
  client.initialize(serverAddress, 0x12345678);

  int i = 0;

  while (true) {
    server.Update();
    if (i < 20 || (i > 300 && i < 500))
      client.Update();
    i++;

    std::this_thread::sleep_for(std::chrono::milliseconds(10));
  }
}

int main() {
  test_server_socket();
  test_server_listens_and_receives_packets_from_client();
  std::cout << "All tests passed!" << std::endl;
}