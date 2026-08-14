#include "../include/Server.h"
#include "Address.h"
#include "Connection.h"
#include "Protocol.h"
#include <cstring>
#include <iostream>

Server::Server(Address address_c) { localAddress = address_c; }

void Server::Initialize() {
  socket.Open(localAddress.GetPort());
  std::cout << "Server listening on port " << localAddress.GetPort() << '\n';
}

int Server::ReceiveFromClients(ClientID &clientID, Buffer &buff) {
  Address incoming;

  int bytesRead = socket.Receive(incoming, buff.data, buff.size);
  if (bytesRead <= 0) {
    return 0;
  }

  bool createdClient;
  Connection &conn = FindOrCreateConnection(clientID, incoming, createdClient);

  buff.index = 0;
  buff.size = bytesRead;

  int processedBytes = conn.ProcessReceived(buff);

  // erase because we created a tunnel regardless
  if (processedBytes <= 0) {
    if (createdClient) {
      clients.erase(clientID);
    }

    return 0;
  }

  PacketType packetType = static_cast<PacketType>(buff.data[buff.index]);

  // must be a connection req
  if (createdClient && packetType != PacketType::ConnectionRequest) {
    clients.erase(clientID);
    return 0;
  } else if (packetType == PacketType::ConnectionRequest) {
    std::cout << "connection accepted" << '\n';
    SendPacket(clientID, ConnectionAccepted{clientID});
  }

  return processedBytes;
}

Connection &Server::FindOrCreateConnection(ClientID &id, Address address,
                                           bool &flagCreatedClient) {
  // TODO: IMPORTANT TODO
  // i want to turn this into a lookup of address -> clientid. for now this is
  // fine; o(n) lookup for a small # of clients is okay. but whe i get to stress
  // testing 50k+ clients or 100k+ clients then this will be problematic.
  for (auto &[connId, connection] : clients) {
    if (connection.GetAddress() == address) {
      id = connId;
      flagCreatedClient = false;
      return connection;
    }
  }

  id = nextClientNumber;
  nextClientNumber++;
  // clients[id] = newConnection;  old; invoked connection default constructor
  // which doesn't exist
  auto [it, inserted] = clients.try_emplace(id, address);
  flagCreatedClient = inserted;
  return it->second;
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
