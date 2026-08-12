#include "../include/Server.h"
#include "Address.h"
#include "Connection.h"
#include <cstring>
#include <iostream>

Server::Server(Address address_c) { localAddress = address_c; }

void Server::initialize() {
  socket.Open(localAddress.GetPort());
  std::cout << "Server listening on port " << localAddress.GetPort() << '\n';
}

Address Server::GetAddress() { return localAddress; }
std::unordered_map<ClientID, Connection> Server::GetClientMap() {
  return clients;
}
bool Server::GetIsConnected() { return socket.isOpen(); }