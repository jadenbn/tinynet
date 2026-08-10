#include "Server.h"
#include "Address.h"
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

Address Server::getServerAddress() { return serverAddress; }
Address Server::getClientAddress() { return clientAddress; }

void Server::listenForClient(uint32_t protocolHash) {
  serverSocket.Open(serverAddress.GetPort());

  char buffer[256]; // todo: turn into packet
  int bytesRead;

  std::cout << "Listening for connection; hash is "
            << std::to_string(protocolHash) << std::endl;
  while (!connectionEstablished) {
    bytesRead = serverSocket.Receive(clientAddress, buffer, sizeof(buffer));
    std::this_thread::sleep_for(std::chrono::milliseconds(10));
    if (bytesRead > 0) {
      uint32_t receivedHeader = 0;
      std::memcpy(&receivedHeader, buffer, 4);
      receivedHeader = ntohl(receivedHeader);
      std::cout << std::to_string(receivedHeader) << '\n';
      if (receivedHeader == protocolHash) {
        std::cout << "Connection established with address "
                  << std::to_string(clientAddress.GetAddress()) << " and port "
                  << std::to_string(clientAddress.GetPort());
        connectionEstablished = true;
      }
    }
  }
}

bool Server::isConnected() { return connectionEstablished; }