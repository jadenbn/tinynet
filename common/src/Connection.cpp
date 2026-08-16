#include "Connection.h"
#include "../shared/include/Protocol.h"
#include "Address.h"
#include "Packets.h"
#include "Socket.h"
#include <arpa/inet.h>
#include <cassert>
#include <cstring>
#include <iostream>

// TODO: congestion avoidance

Connection::Connection(Address remoteAddress) {
  this->remoteAddress = remoteAddress;
}

Address Connection::GetAddress() { return remoteAddress; }
float Connection::GetRTT() { return sentQueue.globalRtt; }

int Connection::ProcessReceived(Buffer &outBuffer) {
  if (outBuffer.length < 16) {
    return 0;
  }

  if (!ValidateHeader(outBuffer)) {
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
  connected = true;
  auto now = std::chrono::steady_clock::now();
  lastReceivedTime = now;

  return outBuffer.length - outBuffer.index;
}

bool Connection::Send(Socket &sock, const Buffer &buffer) {
  uint8_t scratch[MAX_PACKET_SIZE];
  Buffer buff = {scratch, 0, 0, sizeof(scratch)};
  WriteInteger(buff, PROTOCOL_HASH);
  WriteInteger(buff, sequenceNumber);
  WriteInteger(buff, remoteSequenceNumber);

  // todo: turn this into helper later
  uint32_t ackBitfield = 0;
  for (int i = 1; i < 33; i++) {
    auto prevSequenceNumber = remoteSequenceNumber - i;
    if (receivedQueue.exists(prevSequenceNumber)) {
      ackBitfield |= (1U << (i - 1));
    }
  }
  WriteInteger(buff, ackBitfield);
  sentQueue.insert(sequenceNumber, std::chrono::steady_clock::now());
  sequenceNumber++;

  std::memcpy(buff.data + buff.length, buffer.data, buffer.index);

  return sock.Send(remoteAddress, buff.data, buffer.index + buff.index);
}

bool Connection::timedOut() {
  if (!initialPacketReceived)
    return false;

  auto now = std::chrono::steady_clock::now();
  auto elapsed = now - lastReceivedTime;

  return elapsed > CONNECTION_TIMEOUT_MS;
}