#include "Client.h"
#include "Address.h"
#include "Connection.h"
#include "Packets.h"
#include "Protocol.h"
#include <arpa/inet.h>
#include <cstring>
#include <iostream>

Client::Client(Address clientAddress) { connection.SetAddress(clientAddress); }

void Client::initialize(Address serverAddress) {
  connection.Connect(serverAddress);
  connection.Open();
  std::cout << "Client listening on port " << connection.GetAddress().GetPort()
            << '\n';
}

Address Client::GetAddress() { return connection.GetAddress(); }
Address Client::GetRemoteAddress() { return connection.GetRemoteAddress(); }
bool Client::GetIsConnected() { return connection.GetIsConnected(); }
float Client::GetRTT() { return connection.GetRTT(); }

int Client::Receive(Buffer &buff) { return connection.Receive(buff); }

void Client::UpdateConnection() {
  connection.Update(); // handle timeout
}
