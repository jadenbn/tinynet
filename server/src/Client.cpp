#include "Packet.h"
#include "Socket.h"
#include <iostream>
#include <thread>

int main() {
  constexpr unsigned short clientPort = 3001;
  constexpr unsigned short serverPort = 3000;

  Address client(127, 0, 0, 1, clientPort);
  Address server(127, 0, 0, 1, serverPort);
  Socket clientSocket;

  const uint32_t gameProtocolHash = 0x12345678; // temporary

  if (!clientSocket.Open(client.GetPort())) {
    return 1;
  }

  char firstPacket[16];
  firstPacket[0] = (char)(htonl(gameProtocolHash));
  firstPacket[1] = (char)(htonl(gameProtocolHash));
  firstPacket[2] = (char)(htonl(gameProtocolHash));
  firstPacket[3] = (char)(htonl(gameProtocolHash));

  std::cout << "Initiating connection" << std::endl;

  for (int i = 0; i < 10; i++) {
    clientSocket.Send(server, &firstPacket, sizeof(firstPacket));
    std::this_thread::sleep_for(std::chrono::milliseconds(500));
  }
}