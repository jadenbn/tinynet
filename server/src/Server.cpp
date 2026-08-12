#include "../include/Server.h"
#include "Address.h"
#include "Connection.h"
#include <cstring>
#include <iostream>

Server::Server(Address address) { connection.SetAddress(address); }

void Server::initialize() {
  connection.Open();
  std::cout << "Server listening on port " << connection.GetAddress().GetPort()
            << '\n';
}

Address Server::GetAddress() { return connection.GetAddress(); }
Address Server::GetRemoteAddress() { return connection.GetRemoteAddress(); }
bool Server::GetIsConnected() { return connection.GetIsConnected(); }
float Server::GetRTT() { return connection.GetRTT(); }

int Server::Receive(Buffer &buff) { return connection.Receive(buff); }

void Server::UpdateConnection() { connection.Update(); }