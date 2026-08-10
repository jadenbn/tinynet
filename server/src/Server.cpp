#include "Server.h"
#include "Address.h"
#include "Packets.h"
#include "Protocol.h"
#include "Socket.h"
#include <chrono>
#include <cstring>
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

  uint8_t scratch[MAX_PACKET_SIZE + 4];
  Buffer send = {scratch, 0, sizeof(scratch)};
  WriteInteger(send, PROTOCOL_HASH);

  std::memcpy(send.data + 4, buffer.data, buffer.index);

  return serverSocket.Send(clientAddress, send.data, buffer.index + 4);
}

int Server::receive(Buffer &outBuffer) {
  int bytesRead;

  bytesRead =
      serverSocket.Receive(clientAddress, outBuffer.data, MAX_PACKET_SIZE);

  if (bytesRead < 4) {
    return 0; // we need some space for our protocl hash
  }

  outBuffer.size = bytesRead;
  outBuffer.index = 0;

  uint32_t receivedHeader = ReadInteger(outBuffer);
  if (receivedHeader != PROTOCOL_HASH) {
    return 0;
  }

  initialPacketReceived = true;
  auto now = std::chrono::steady_clock::now();
  lastReceivedTime = now;

  return bytesRead - 4;
}

Address Server::getServerAddress() { return serverAddress; }
Address Server::getClientAddress() { return clientAddress; }

void Server::initialize() {
  serverSocket.Open(serverAddress.GetPort());
  std::cout << "Initialized to listen for connection; hash is "
            << std::to_string(PROTOCOL_HASH) << std::endl;
}

void Server::Update() {
  uint8_t raw[MAX_PACKET_SIZE];
  Buffer buff = {raw, 0, sizeof(raw)};

  while (receive(buff) > 0) {
    // process
    std::cout << "First char in packet: " << ReadChar(buff);
  }

  isConnected = !timedOut();
  if (!isConnected) {
    clientAddress = Address();
  }
}

bool Server::timedOut() {
  if (!initialPacketReceived)
    return false;

  auto now = std::chrono::steady_clock::now();
  auto elapsed = now - lastReceivedTime;
  return elapsed > TIMEOUT_MS;
}

bool Server::getIsConnected() { return isConnected; }