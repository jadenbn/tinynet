#include "Client.h"
#include "Address.h"
#include "Connection.h"
#include "Packets.h"
#include <arpa/inet.h>
#include <cstring>
#include <iostream>

Client::Client(Address clientAddress, Address serverAddress)
    : localAddress(clientAddress), serverConnection(serverAddress) {};

void Client::Initialize() {
  socket.Open(localAddress.GetPort());
  std::cout << "Client listening on port " << localAddress.GetPort() << '\n';
}
// give me a commit :()
bool Client::GetIsConnected() { return serverConnection.connected; }
float Client::GetRTT() { return serverConnection.GetRTT(); }

int Client::ReceiveFromServer(Buffer &buff) {
  Address inc;
  int bytesRead = socket.Receive(inc, buff.data, buff.capacity);
  if (bytesRead <= 0) {
    return 0;
  }
  buff.index = 0;
  buff.length = bytesRead;
  return serverConnection.ProcessReceived(buff);
};

void Client::UpdateConnection() {
  serverConnection.connected = !serverConnection.timedOut();
}
