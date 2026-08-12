#include "Connection.h"
#include "Address.h"
#include "Packets.h"
#include "Protocol.h"
#include "Socket.h"
#include <arpa/inet.h>
#include <cassert>
#include <cstring>

// TODO: congestion avoidance

Connection::Connection(Address address) {
  this->address = address;
  socket = Socket();
}

Connection::Connection() { socket = Socket(); }

Address Connection::GetAddress() { return address; }
Address Connection::GetRemoteAddress() { return remoteAddress; }
void Connection::SetAddress(Address address) { this->address = address; }

bool Connection::GetIsConnected() { return isConnected; }
float Connection::GetRTT() { return sentQueue.globalRtt; }

bool Connection::Open() {
  assert(address != Address()); // ensure address isn't default
  return socket.Open(address.GetPort());
};

int Connection::Receive(Buffer &outBuffer) {
  int bytesRead;

  bytesRead = socket.Receive(remoteAddress, outBuffer.data, MAX_PACKET_SIZE);

  if (bytesRead < 16) {
    return 0;
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

bool Connection::Send(const Buffer &buffer) {
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

  return socket.Send(remoteAddress, send.data, buffer.index + send.index);
}

bool Connection::timedOut() {
  if (!initialPacketReceived)
    return false;

  auto now = std::chrono::steady_clock::now();
  auto elapsed = now - lastReceivedTime;

  return elapsed > CONNECTION_TIMEOUT_MS;
}

void Connection::Connect(Address remoteAddress) {
  this->remoteAddress = remoteAddress; // for now just a SetRemoteAddress
  isConnected = true;
}

void Connection::Update() {
  // handnle watchdog timeout
  isConnected = !timedOut();
  if (!isConnected) {
    return;
  }
}