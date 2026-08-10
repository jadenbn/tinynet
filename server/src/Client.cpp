#include "Packet.h"
#include "Socket.h"
#include <cstring>
#include <iostream>
#include <string>
#include <thread>

int main() {
  constexpr unsigned short clientPort = 3001;
  constexpr unsigned short serverPort = 3000;

  Address client(127, 0, 0, 1, clientPort);
  Address server(127, 0, 0, 1, serverPort);
  Socket clientSocket;

  const uint32_t gameProtocolHash = 0x12345678; // temporary
  auto gameProtocolHash_networked = htonl(gameProtocolHash);

  if (!clientSocket.Open(client.GetPort())) {
    return 1;
  }

  char firstPacket[16];
  std::memcpy(firstPacket, &gameProtocolHash_networked,
              sizeof(gameProtocolHash_networked));

  std::cout << "Initiating connection; hash is "
            << std::to_string(gameProtocolHash) << std::endl;

  for (int i = 0; i < 10; i++) {
    clientSocket.Send(server, firstPacket, sizeof(firstPacket));
    std::this_thread::sleep_for(std::chrono::milliseconds(500));
  }
}