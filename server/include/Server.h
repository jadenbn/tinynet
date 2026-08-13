#pragma once
#include "Connection.h"
#include "Packets.h"
#include <unordered_map>

class Address;

class Server {
public:
  Server(Address serverAddress);
  void Initialize();
  void UpdateConnections();
  Address GetAddress();
  std::unordered_map<ClientID, Connection> &GetClientMap();
  bool GetIsConnected();
  int ReceiveFromClients(ClientID &client, Buffer &buff);
  Connection *FindOrCreateConnection(ClientID &id, Address address);

  template <typename Packet>
  bool SendPacket(const ClientID client, const Packet &packet) {
    auto conn = clients.find(client);
    if (conn == clients.end())
      return false;

    uint8_t scratch[MAX_PACKET_SIZE];
    Buffer buff = Buffer(scratch, 0, sizeof(scratch));
    packet.Serialize(buff);
    return conn->second.Send(socket, buff);
  }

private:
  Socket socket;
  Address localAddress;
  std::unordered_map<ClientID, Connection> clients;
  ClientID nextClientNumber = 0;
};
