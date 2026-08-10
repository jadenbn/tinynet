#include "Server.h"
#include "Address.h"
#include "Socket.h"
#include <chrono>
#include <iostream>
#include <string>

Server::Server(Address serverAddress) {
  this->serverAddress = serverAddress;
  serverSocket = Socket();
}

bool Server::send(char *packet) {
  if (!isConnected) {
    return false;
  }

  uint32_t protocolHashNetworked = htonl(protocolHash);

  char firstPacket[16];
  std::memcpy(firstPacket, &protocolHashNetworked,
              sizeof(protocolHashNetworked));

  return serverSocket.Send(clientAddress, firstPacket, sizeof(firstPacket));
}

int Server::receive() {
  char buffer[256]; // todo: turn into packet
  int bytesRead;

  bytesRead = serverSocket.Receive(clientAddress, buffer, sizeof(buffer));
  if (bytesRead > 0) {
    uint32_t receivedHeader = 0;
    std::memcpy(&receivedHeader, buffer, 4);
    receivedHeader = ntohl(receivedHeader);
    if (receivedHeader == protocolHash) {
      initialPacketReceived = true;
      auto now = std::chrono::steady_clock::now();
      lastReceivedTime = now;
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
  receive();
  isConnected = !timedOut();
  if (!isConnected) {
    clientAddress = Address();
  }
  return;
}

bool Server::timedOut() {
  if (!initialPacketReceived)
    return false;

  auto now = std::chrono::steady_clock::now();
  auto elapsed = now - lastReceivedTime;
  return elapsed > TIMEOUT_MS;
}

bool Server::getIsConnected() { return isConnected; }