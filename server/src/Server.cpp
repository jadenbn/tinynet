#include "Packet.h"
#include "Socket.h"
#include <chrono>
#include <iostream>
#include <string>
#include <thread>

int main() {
  constexpr unsigned short serverPort = 3000;

  Address server(127, 0, 0, 1, serverPort);
  Socket serverSocket;

  const uint32_t gameProtocolHash = 0x12345678;
  std::cout << std::to_string(gameProtocolHash) << '\n';

  if (!serverSocket.Open(serverPort)) {
    return 1;
  }

  bool connectionEstablished = false;
  char buffer[256];
  int bytesRead;
  Address client;
  // server listens every 10ms
  std::cout << "Listening for connection; hash is "
            << std::to_string(gameProtocolHash) << std::endl;

  while (!connectionEstablished) {
    std::this_thread::sleep_for(std::chrono::milliseconds(10));
    bytesRead = serverSocket.Receive(client, buffer, sizeof(buffer));
    if (bytesRead > 0) {
      uint32_t receivedHeader = 0;
      std::memcpy(&receivedHeader, buffer, 4);
      receivedHeader = ntohl(receivedHeader);
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