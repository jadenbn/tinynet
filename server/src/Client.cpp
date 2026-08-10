#include "Client.h"
#include "Packet.h"
#include "Server.h" // for timeoutms constant
#include "Socket.h"
#include <arpa/inet.h>
#include <cstring>
#include <iostream>

Client::Client(Address clientAddress) {
  this->clientAddress = clientAddress;
  clientSocket = Socket();
}

Address Client::getServerAddress() { return serverAddress; }
Address Client::getClientAddress() { return clientAddress; }

bool Client::getIsConnected() { return isConnected; }

void Client::initialize(Address serverAddress, uint32_t protocolHash) {
  clientSocket.Open(clientAddress.GetPort());
  this->serverAddress = serverAddress;
  this->protocolHash = protocolHash;
  isConnected = true;
}

int Client::receive() {
  char buffer[256]; // todo: turn into packet
  int bytesRead;

  bytesRead = clientSocket.Receive(serverAddress, buffer, sizeof(buffer));
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

bool Client::send(char *packet) {
  if (!isConnected)
    return false;

  uint32_t protocolHashNetworked = htonl(protocolHash);

  char firstPacket[16];
  std::memcpy(firstPacket, &protocolHashNetworked,
              sizeof(protocolHashNetworked));

  return clientSocket.Send(serverAddress, firstPacket, sizeof(firstPacket));
}

bool Client::timedOut() {
  if (!initialPacketReceived)
    return false;

  auto now = std::chrono::steady_clock::now();
  auto elapsed = now - lastReceivedTime;

  return elapsed > TIMEOUT_MS;
}

void Client::Update() {
  receive();
  isConnected = !timedOut();
  if (!isConnected) {
    return;
  }
}