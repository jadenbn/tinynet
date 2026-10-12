#include "../server/include/Server.h"
#include "../shared/include/Protocol.h"
#include "Client.h"
#include "Packets.h"
#include <cassert>
#include <chrono>
#include <iostream>
#include <thread>

void test_server_socket() {
  Server s(Address(127, 0, 0, 1, 3000));
  assert(s.GetAddress().GetPort() == 3000);
  assert(s.GetAddress().GetA() == 127);
}

void test_server_listens_and_receives_packets_from_client() {
  Address serverAddress = Address(127, 0, 0, 1, 3000);
  Address clientOneAddress = Address(127, 0, 0, 1, 3001);

  Server server = Server(serverAddress);
  server.Initialize();

  Client client = Client(clientOneAddress, serverAddress);
  client.Initialize();

  // run for 2 * timeout duration
  for (int i = 0; i < ((CONNECTION_TIMEOUT_MS.count() * 2) / 10); i++) {
    client.UpdateConnection();
    server.UpdateConnections();

    uint8_t tmp1[MAX_PACKET_SIZE];
    Buffer buff1 = {tmp1, 0, 0, sizeof(tmp1)};
    client.ReceiveFromServer(buff1);

    uint8_t tmp2[MAX_PACKET_SIZE];
    ClientID clientID;
    Buffer buff2 = {tmp2, 0, 0, sizeof(tmp1)};
    server.ReceiveFromClients(clientID, buff2);

    // establish connection
    if (i < 200) {
      assert(client.SendPacket(ConnectionRequest{}));
      if (server.GetClientMap().size() > 0) {
        assert(server.SendPacket(clientID, ConnectionAccepted{clientID}));
      }
    }

    // confirm connection between 100ms and 1s
    if (i > 10 && i < 100) {
      assert(client.GetIsConnected());
      assert(server.GetClientMap().size() > 0);
    }

    // confirm both timeout
    if (i > (CONNECTION_TIMEOUT_MS.count() + 100)) {
      assert(!client.GetIsConnected());
      assert(!server.GetIsConnected());
    }

    std::this_thread::sleep_for(std::chrono::milliseconds(10));
  }
}

// pumps both sides for a few ms at a time until `done` or a second passes
template <typename Done>
bool PumpUntil(Server &server, Client &client, Done done) {
  for (int i = 0; i < 100 && !done(); i++) {
    client.UpdateConnection();
    server.UpdateConnections();
    uint8_t data[MAX_PACKET_SIZE];
    Buffer buff = {data, 0, 0, sizeof(data)};
    while (client.ReceiveFromServer(buff) > 0) {
    }
    ClientID id;
    while (server.ReceiveFromClients(id, buff) > 0) {
    }
    std::this_thread::sleep_for(std::chrono::milliseconds(10));
  }
  return done();
}

void test_disconnects_are_immediate() {
  Address serverAddress(127, 0, 0, 1, 3010);
  Server server(serverAddress);
  server.Initialize();
  Client leaver(Address(127, 0, 0, 1, 3011), serverAddress);
  Client stayer(Address(127, 0, 0, 1, 3012), serverAddress);
  leaver.Initialize();
  stayer.Initialize();

  leaver.SendPacket(ConnectionRequest{});
  stayer.SendPacket(ConnectionRequest{});
  assert(PumpUntil(server, stayer,
                   [&] { return server.GetClientMap().size() == 2; }));

  // the client leaving: gone well before the 10s timeout
  auto start = std::chrono::steady_clock::now();
  leaver.Disconnect();
  assert(!leaver.GetIsConnected());
  assert(PumpUntil(server, stayer,
                   [&] { return server.GetClientMap().size() == 1; }));
  assert(std::chrono::steady_clock::now() - start <
         std::chrono::milliseconds(500));

  // the server kicking the other one
  ClientID stayerID = server.GetClientMap().begin()->first;
  assert(PumpUntil(server, stayer, [&] { return stayer.GetIsConnected(); }));
  server.Disconnect(stayerID);
  assert(server.GetClientMap().empty());
  assert(PumpUntil(server, stayer, [&] { return stayer.WasDisconnected(); }));
  stayer.UpdateConnection();
  assert(!stayer.GetIsConnected());
}

int main() {
  test_server_socket();
  test_disconnects_are_immediate();
  test_server_listens_and_receives_packets_from_client();
  std::cout << "All tests passed!" << std::endl;
}
