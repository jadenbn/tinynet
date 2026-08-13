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

int Server::ReceiveFromClients(ClientID &clientID, Buffer &buff) {
  Address incoming;

  int bytesRead = socket.Receive(incoming, buff.data, buff.index);
  if (bytesRead <= 0) {
    return 0;
  }

  Connection *conn = FindOrCreateConnection(client, incoming);

  buff.index = 0;
  buff.size = bytesRead;

  return conn->ProcessReceived(buff);
}

Address Server::GetAddress() { return localAddress; }
std::unordered_map<ClientID, Connection> Server::GetClientMap() {
  return clients;
}
bool Server::GetIsConnected() { return socket.isOpen(); }

void Server::UpdateConnections() {
  for (auto &[id, connection] : clients) {
    connection.connected = !connection.timedOut();
  }
}