#include "Packet.h"
#include "Socket.h"
#include <functional>
#include <iostream>
#include <ratio>
#include <string_view>
#include <thread>

int main() {
  std::hash<std::string_view> hasher;
  constexpr unsigned short clientPort = 3001;
  constexpr unsigned short serverPort = 3000;

  Address client(127, 0, 0, 1, clientPort);
  Address server(127, 0, 0, 1, serverPort);
  Socket clientSocket;

  const uint32_t gameProtocolHash = hasher("tinynet");
  std::cout << std::to_string(gameProtocolHash) << '\n';

  if (!clientSocket.Open(client.GetPort())) {
    return 1;
  }

  char firstPacket[16];
  firstPacket[0] = (char)(gameProtocolHash >> 24);
  firstPacket[1] = (char)(gameProtocolHash >> 16);
  firstPacket[2] = (char)(gameProtocolHash >> 8);
  firstPacket[3] = (char)(gameProtocolHash);

  std::cout << "Initiating connection" << std::endl;

  for (int i = 0; i < 10; i++) {
    clientSocket.Send(server, &firstPacket, sizeof(firstPacket));
    std::this_thread::sleep_for(std::chrono::milliseconds(500));
  }
}