#include "../include/Server.h"
#include "Address.h"
#include "Packets.h"
#include "Protocol.h"
#include <chrono>
#include <iostream>
#include <thread>

int main() {
  Server server(Address(127, 0, 0, 1, 3000));
  server.initialize();

  std::cout << "Starting server on 127.0.0.1:3000" << '\n';
  while (true) {
    server.UpdateConnection();

    uint8_t tmp[MAX_PACKET_SIZE];
    Buffer buff = {tmp, 0, sizeof(MAX_PACKET_SIZE)};
    while (server.Receive(buff) > 0) {
      // server received data
      PacketType packetType = static_cast<PacketType>(ReadChar(buff));
      if (packetType == PacketType::PlayerInput) {
        PlayerInputPacket packet = PlayerInputPacket::deserialize(buff);
        std::cout << "Received player move packet! x: " << packet.x
                  << "y: " << packet.y << '\n';
        server.SendPacket(PlayerInputPacket{69.0f, 420.0f});
      }
    }
    std::this_thread::sleep_for(std::chrono::milliseconds(16));
  }
}