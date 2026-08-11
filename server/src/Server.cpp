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

  uint8_t scratch[MAX_PACKET_SIZE];
  Buffer send = {scratch, 0, sizeof(scratch)};
  WriteInteger(send, PROTOCOL_HASH);  // write protocol hash
  WriteInteger(send, sequenceNumber); // write sequence num
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

  // copy payload of buffer
  std::memcpy(send.data + send.index, buffer.data, buffer.index);

  return serverSocket.Send(clientAddress, send.data, buffer.index + send.index);
}

int Server::receive(Buffer &outBuffer) {
  int bytesRead;

  bytesRead =
      serverSocket.Receive(clientAddress, outBuffer.data, MAX_PACKET_SIZE);

  if (bytesRead < 16) {
    return 0; // we need some space for our protocl hash, sequence num, ack,
              // etc.
  }

  outBuffer.size = bytesRead;
  outBuffer.index = 0;

  uint32_t receivedHeader = ReadInteger(outBuffer);
  if (receivedHeader != PROTOCOL_HASH) {
    return 0;
  }
  uint32_t incomingSequenceNumber = ReadInteger(outBuffer);

  if (incomingSequenceNumber > remoteSequenceNumber) {
    remoteSequenceNumber = incomingSequenceNumber;
  }

  receivedQueue.insert(incomingSequenceNumber);

  uint32_t incomingAck = ReadInteger(outBuffer);
  uint32_t incomingAckBitfield = ReadInteger(outBuffer);

  sentQueue.ackPacket(incomingAck, incomingAckBitfield);

  initialPacketReceived = true;
  auto now = std::chrono::steady_clock::now();
  lastReceivedTime = now;

  return bytesRead - outBuffer.index;
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
    // server received data
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