#include "Server.h"
#include "Address.h"
#include "Packets.h"
#include "Socket.h"
#include <chrono>
#include <iostream>
#include <string>

Server::Server(Address serverAddress) {
  this->serverAddress = serverAddress;
  serverSocket = Socket();
}

bool Server::send(const Buffer &buffer) {
  if (!isConnected) {
    return false;
  }

  return serverSocket.Send(clientAddress, buffer.data, buffer.index);
}

int Server::receive(Buffer &outBuffer) {
  uint8_t data[MAX_PACKET_SIZE];
  int bytesRead;

  bytesRead = serverSocket.Receive(clientAddress, data, sizeof(data));

  if (bytesRead < 4) {
    return 0; // we need some space for our protocl hash
  }
  
  if (bytesRead > 0) {
    uint32_t receivedHeader = 0;
    std::memcpy(&receivedHeader, data, 4); // 4 is 4 bytes for protocol hash
    receivedHeader = ntohl(receivedHeader);
    if (receivedHeader == protocolHash) {
      initialPacketReceived = true;
      auto now = std::chrono::steady_clock::now();
      lastReceivedTime = now;
      outBuffer.data = data + 4; // ptr arithmetic cpsc213!! offset by 4 because
                                 // of the protocol hash
      outBuffer.index = 0;       // reading now
      outBuffer.size = bytesRead;
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