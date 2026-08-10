#include "Packet.h"
#include "Socket.h"
#include <chrono>
#include <functional>
#include <iostream>
#include <string>
#include <string_view>
#include <thread>

int main() {
  std::hash<std::string_view> hasher;
  constexpr unsigned short serverPort = 3000;

  Address server(127, 0, 0, 1, serverPort);
  Socket serverSocket;

  const uint32_t gameProtocolHash = hasher("tinynet");
  std::cout << std::to_string(gameProtocolHash) << '\n';

  if (!serverSocket.Open(serverPort)) {
    return 1;
  }

  bool connectionEstablished = false;
  char buffer[256];
  int bytesRead;
  Address client;
  // server listens every 10ms
  std::cout << "Listening for connection" << std::endl;

  while (!connectionEstablished) {
    std::this_thread::sleep_for(std::chrono::milliseconds(10));
    bytesRead = serverSocket.Receive(client, buffer, sizeof(buffer));
    if (bytesRead > 0) {
      char first = buffer[0];
      char second = buffer[1];
      char third = buffer[2];
      char fourth = buffer[3];
      uint32_t receivedHeader =
          first << 24 | second << 16 | third << 8 | fourth;
      std::cout << std::to_string(receivedHeader) << '\n';
      if (receivedHeader == gameProtocolHash) {
        std::cout << "Connection established with address "
                  << std::to_string(client.GetAddress()) << " and port "
                  << std::to_string(client.GetPort());
        break;
      }
    }
  }
}