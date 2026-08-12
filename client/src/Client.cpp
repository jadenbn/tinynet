#include "Client.h"
#include "Connection.h"
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

void Client::Update() {
  connection.Update();
  uint8_t rawData[MAX_PACKET_SIZE];
  Buffer buff = {rawData, 0, sizeof(rawData)};

  while (connection.Receive(buff) > 0) {
    // received data
  }
}