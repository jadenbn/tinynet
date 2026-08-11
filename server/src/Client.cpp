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

  if (bytesRead < 16) {
    return 0;
  }

  outBuffer.size = bytesRead;
  outBuffer.index = 0;

  uint32_t receivedHeader = ReadInteger(outBuffer);
  if (receivedHeader != PROTOCOL_HASH) {
    return 0;
  }

  uint32_t sequenceNumber = ReadInteger(outBuffer);
  uint32_t remoteSequenceNumber = ReadInteger(outBuffer);
  uint32_t ackBitfield = ReadInteger(outBuffer);

  sentQueue.ackPacket(remoteSequenceNumber, ackBitfield);
  receivedQueue.insert(sequenceNumber);

  initialPacketReceived = true;
  auto now = std::chrono::steady_clock::now();
  lastReceivedTime = now;

  return bytesRead - outBuffer.index;
}

bool Client::send(const Buffer &buffer) {
  if (!isConnected)
    return false;

  uint8_t scratch[MAX_PACKET_SIZE];
  Buffer send = {scratch, 0, sizeof(scratch)};
  WriteInteger(send, PROTOCOL_HASH);
  WriteInteger(send, sequenceNumber);
  WriteInteger(send, remoteSequenceNumber);

  // todo: turn this into helper later
  uint32_t ackBitfield = 0;
  for (int i = 1; i < 33; i++) {
    auto prevSequenceNumber = remoteSequenceNumber - i;
    if (receivedQueue.exists(prevSequenceNumber)) {
      ackBitfield |= (1U << (i - 1));
    }
  }
  WriteInteger(send, ackBitfield);
  sentQueue.insert(sequenceNumber, std::chrono::steady_clock::now());
  sequenceNumber++;

  std::memcpy(send.data + send.index, buffer.data, buffer.index);

  return clientSocket.Send(serverAddress, send.data, buffer.index + send.index);
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