#include "Client.h"
#include "Packets.h"
#include "Protocol.h"
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

void Client::initialize(Address serverAddress) {
  clientSocket.Open(clientAddress.GetPort());
  this->serverAddress = serverAddress;
  isConnected = true;
}

int Client::receive(Buffer &outBuffer) {
  int bytesRead;

  bytesRead =
      clientSocket.Receive(serverAddress, outBuffer.data, MAX_PACKET_SIZE);

  if (bytesRead < 4) {
    return 0;
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

bool Client::send(const Buffer &buffer) {
  if (!isConnected)
    return false;

  uint8_t scratch[MAX_PACKET_SIZE + 4];
  Buffer send = {scratch, 0, sizeof(scratch)};
  WriteInteger(send, PROTOCOL_HASH);

  std::memcpy(send.data + 4, buffer.data, buffer.index);

  return clientSocket.Send(serverAddress, send.data, buffer.index + 4);
}

bool Client::timedOut() {
  if (!initialPacketReceived)
    return false;

  auto now = std::chrono::steady_clock::now();
  auto elapsed = now - lastReceivedTime;

  return elapsed > TIMEOUT_MS;
}

void Client::Update() {
  uint8_t rawData[MAX_PACKET_SIZE];
  Buffer buff = {rawData, 0, sizeof(rawData)};

  while (receive(buff) > 0) {
    std::cout << "Client received data! First char: "
              << std::to_string(ReadChar(buff)) << '\n';
  }

  isConnected = !timedOut();
  if (!isConnected) {
    return;
  }
}