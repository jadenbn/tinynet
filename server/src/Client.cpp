#include "Client.h"
#include "Packet.h"
#include "Socket.h"
#include <cstring>
#include <iostream>
#include <string>
#include <thread>

Client::Client(Address clientAddress) {
  this->clientAddress = clientAddress;
  clientSocket = Socket();
}

void Client::connect(Address serverAddress, uint32_t protocolHash) {
  this->serverAddress = serverAddress;
  clientSocket.Open(clientAddress.GetPort());
  uint32_t protocolHashNetworked = htonl(protocolHash);

  char firstPacket[16];
  std::memcpy(firstPacket, &protocolHashNetworked,
              sizeof(protocolHashNetworked));

  std::cout << "Initiating connection; hash is " << std::to_string(protocolHash)
            << std::endl;

  for (int i = 0; i < 10; i++) {
    clientSocket.Send(serverAddress, firstPacket, sizeof(firstPacket));
    std::this_thread::sleep_for(std::chrono::milliseconds(500));
  }
}

void Client::Update() {}

int main() {
  Client client = Client(Address(127, 0, 0, 1, 3001));
  client.connect(Address(127, 0, 0, 1, 3000), 0x12345678);
}