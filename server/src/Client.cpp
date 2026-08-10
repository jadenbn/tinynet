#include "Client.h"
#include "Packet.h"
#include "Socket.h"
#include <arpa/inet.h>
#include <cstring>

Client::Client(Address clientAddress) {
  this->clientAddress = clientAddress;
  clientSocket = Socket();
}

void Client::initialize(Address serverAddress, uint32_t protocolHash) {
  clientSocket.Open(clientAddress.GetPort());
  this->serverAddress = serverAddress;
  this->protocolHash = protocolHash;
  isConnected = true;
}

int Client::send(char *packet) {
  if (!isConnected)
    return 0;

  uint32_t protocolHashNetworked = htonl(protocolHash);

  char firstPacket[16];
  std::memcpy(firstPacket, &protocolHashNetworked,
              sizeof(protocolHashNetworked));

  return clientSocket.Send(serverAddress, firstPacket, sizeof(firstPacket));
}

bool Client::timedOut() { return false; }

void Client::Update() {
  isConnected = !timedOut();
  if (!isConnected)
    return;

  send(0);
}