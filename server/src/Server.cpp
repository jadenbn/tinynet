#include "Server.h"
#include "Address.h"
#include "Client.h"
#include "Packet.h"
#include "Socket.h"
#include <chrono>
#include <iostream>
#include <string>
#include <thread>

Server::Server(Address serverAddress) {
  this->serverAddress = serverAddress;
  serverSocket = Socket();
}

bool Server::send(char *packet) {
  if (!isConnected) {
    return false;
  }

  // WE NEED TO PREPEND THIS PACKET WITH THE PROTOCOL HASH

  return serverSocket.Send(clientAddress, packet, sizeof(packet));
}

int Server::receive() {
  char buffer[256]; // todo: turn into packet
  int bytesRead;

  bytesRead = serverSocket.Receive(clientAddress, buffer, sizeof(buffer));
  if (bytesRead > 0) {
    uint32_t receivedHeader = 0;
    std::memcpy(&receivedHeader, buffer, 4);
    receivedHeader = ntohl(receivedHeader);
    std::cout << std::to_string(receivedHeader) << '\n';
    if (receivedHeader == protocolHash) {
      std::cout << "Got a valid packet from client! " << '\n';
      timeout = 0;
    } else {
      bytesRead = 0; // not from the right header
    }
  }
  return bytesRead;
}

Address Server::getServerAddress() { return serverAddress; }
Address Server::getClientAddress() { return clientAddress; }

void Server::initialize(uint32_t protocolHash) {
  serverSocket.Open(serverAddress.GetPort());
  this->protocolHash = protocolHash;
  std::cout << "Initialized to listen for connection; hash is "
            << std::to_string(protocolHash) << std::endl;
}

void Server::Update() {
  isConnected = !timedOut();
  if (!isConnected)
    return;
  receive();
}

bool Server::timedOut() { return false; }

bool Server::getIsConnected() { return isConnected; }

int main() {
  Address serverAddress = Address(127, 0, 0, 1, 3000);
  Server server = Server(serverAddress);
  server.initialize(0x12345678);

  Client client = Client(Address(127, 0, 0, 1, 3001));
  client.initialize(serverAddress, 0x12345678);

  while (true) {
    std::this_thread::sleep_for(std::chrono::milliseconds(10));
    server.Update();
    client.Update();
  }
}