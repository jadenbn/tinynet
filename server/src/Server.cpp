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

  int bytesRead = socket.Receive(incoming, buff.data, buff.size);
  if (bytesRead <= 0) {
    return 0;
  }

  Connection *conn = FindOrCreateConnection(clientID, incoming);

  buff.index = 0;
  buff.size = bytesRead;

  return conn->ProcessReceived(buff);
}

Connection *Server::FindOrCreateConnection(ClientID &id, Address address) {
  // TODO: IMPORTANT TODO
  // i want to turn this into a lookup of address -> clientid. for now this is
  // fine; o(n) lookup for a small # of clients is okay. but whe i get to stress
  // testing 50k+ clients or 100k+ clients then this will be problematic.
  for (auto &[connId, connection] : clients) {
    if (connection.GetAddress() == address) {
      id = connId;
      return &connection;
    }
  }

  Connection newConnection = Connection(address);
  id = nextClientNumber;
  nextClientNumber++;
  clients[id] = newConnection;

  return &clients.find(id)->second;
}

Address Server::GetAddress() { return localAddress; }
std::unordered_map<ClientID, Connection> &Server::GetClientMap() {
  return clients;
}
bool Server::GetIsConnected() { return socket.isOpen(); }

void Server::UpdateConnections() {
  for (auto &[id, connection] : clients) {
    connection.connected = !connection.timedOut();
  }
}