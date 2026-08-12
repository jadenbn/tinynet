#include "../include/Server.h"
#include "Address.h"
#include "Connection.h"
#include "Protocol.h"
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

void Server::Update() {
  connection.Update();
  uint8_t rawData[MAX_PACKET_SIZE];
  Buffer buff = {rawData, 0, sizeof(rawData)};

  while (connection.Receive(buff) > 0) {
    // server received data
    PacketType packetType = static_cast<PacketType>(ReadChar(buff));
    if (packetType == PacketType::PlayerInput) {
      PlayerInputPacket packet = PlayerInputPacket::deserialize(buff);
      std::cout << "Received player move packet! x: " << packet.x
                << "y: " << packet.y << '\n';
    }
  }
}