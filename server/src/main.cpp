#include "../include/Server.h"
#include "Address.h"
#include <chrono>
#include <iostream>
#include <thread>

int main() {
  Server server(Address(127, 0, 0, 1, 3000));
  server.initialize();

  std::cout << "Starting server on 127.0.0.1:3000" << '\n';
  while (true) {
    server.Update();
    std::this_thread::sleep_for(std::chrono::milliseconds(16));
  }
}