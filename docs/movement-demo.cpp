#include "Client.h"
#include "ClientSimulation.h"
#include "ServerReplicationSystem.h"
#include "ServerWorld.h"
#include "SnapshotHistory.h"
#include <array>
#include <cassert>
#include <chrono>
#include <fstream>
#include <iostream>
#include <thread>

// Record the real UDP path without a window; render_demo.py draws these states.
int main(int argc, char **argv) {
  if (argc != 2)
    return 1;
  std::ofstream trace(argv[1]);
  if (!trace)
    return 1;
  Address address(127,0,0,1,40100);
  Server server(address);
  server.Initialize();
  ServerWorld world;
  ServerReplicationSystem replication(world);
  std::array<Client, 2> clients{Client(Address(127,0,0,1,40101), address),
                               Client(Address(127,0,0,1,40102), address)};
  std::array<ClientSimulation, 2> predictions;
  std::array<SnapshotHistory, 2> histories;
  std::array<PlayerID, 2> ids{};
  std::array<bool, 2> connected{};
  std::array<Vec3, 2> authoritative{};
  for (auto &client : clients) {
    client.Initialize();
    client.SetConditions({50, 15, 0.05f, 0.02f});
  }
  trace << "time,px,pz,pqx,pqy,pqz,pqw,ax,az,rx,rz,rqx,rqy,rqz,rqw,rawx,rawz,rtt,pending,rawbytes,packedbytes\n";
  using Clock = std::chrono::steady_clock;
  auto start = Clock::now();
  uint8_t data[MAX_PACKET_SIZE];
  for (int tick = 0; tick < 720; ++tick) {
    for (size_t i = 0; i < clients.size(); ++i) {
      auto &client = clients[i];
      client.UpdateConnection();
      histories[i].Advance(TICK_DT);
      Buffer buff{data,0,0,sizeof(data)};
      while (client.ReceiveFromServer(buff) > 0) {
        PacketType type = static_cast<PacketType>(packets::ReadChar(buff));
        if (type == PacketType::ConnectionAccepted && buff.length-buff.index >= 4) {
          ids[i] = packets::ReadInteger(buff);
          connected[i] = true;
        } else if (type == PacketType::WorldSnapshot) {
          auto snapshot = WorldSnapshot::deserialize(buff);
          if (histories[i].Push(snapshot, client.GetRTT()/2000.0)) {
            if (auto local = snapshot.players.find(ids[i]); local != snapshot.players.end()) {
              predictions[i].Reconcile(local->second);
              authoritative[i] = local->second.pos;
            }
          }
        }
      }
      if (!connected[i]) {
        if (tick % 6 == 0)
          client.SendPacket(ConnectionRequest{});
        continue;
      }
      // Move in arcs, then release the keys so friction is visible.
      float time = tick*TICK_DT;
      float yaw = (i == 0 ? 1.0f : -0.9f)*time;
      Vec3 direction = tick < 600 ? Vec3{0,0,1} : Vec3{};
      predictions[i].AddInput(direction, yaw);
      PlayerInputPacket packet;
      for (const auto &input : predictions[i].pending) {
        if (packet.inputs.size() == INPUTS_PER_PACKET)
          break;
        packet.inputs.push_back(input);
      }
      assert(client.SendPacket(packet));
    }
    server.UpdateConnections();
    Buffer buff{data,0,0,sizeof(data)};
    ClientID clientID;
    while (server.ReceiveFromClients(clientID, buff) > 0)
      replication.HandlePacket(clientID, buff);
    world.Tick();
    auto snapshot = world.Snapshot();
    for (const auto &[id, player] : world.players)
      server.SendPacket(player.owner, snapshot);
    if (predictions[0].ready && predictions[1].ready && tick % 2 == 0) {
      auto remote = histories[0].Sample(ids[1], histories[0].RenderTime());
      auto rawRemote = histories[0].Sample(ids[1], histories[0].LatestTime());
      if (remote && rawRemote) {
        uint8_t rawData[MAX_PACKET_SIZE], packedData[MAX_PACKET_SIZE];
        Buffer raw{rawData,0,0,sizeof(rawData)}, packed{packedData,0,0,sizeof(packedData)};
        snapshot.SerializeRaw(raw);
        snapshot.Serialize(packed);
        const auto &local = predictions[0].local;
        const auto &q = local.orientation;
        const auto &rq = remote->orientation;
        trace << tick*TICK_DT << ',' << local.pos.x << ',' << local.pos.z << ','
              << q.x << ',' << q.y << ',' << q.z << ',' << q.w << ','
              << authoritative[0].x << ',' << authoritative[0].z << ','
              << remote->pos.x << ',' << remote->pos.z << ','
              << rq.x << ',' << rq.y << ',' << rq.z << ',' << rq.w << ','
              << rawRemote->pos.x << ',' << rawRemote->pos.z << ','
              << clients[0].GetRTT() << ',' << predictions[0].pending.size() << ','
              << raw.length+16 << ',' << packed.length+16 << '\n';
      }
    }
    std::this_thread::sleep_until(start + std::chrono::duration_cast<Clock::duration>(
        std::chrono::duration<double>((tick+1)*static_cast<double>(TICK_DT))));
  }
  assert(predictions[0].ready && predictions[1].ready);
  assert(!predictions[0].pending.empty());
  assert(predictions[0].pending.size() < MAX_PENDING_INPUTS);
  assert(Dot(predictions[0].local.velocity, predictions[0].local.velocity) == 0);
  trace.close();
  std::cout << "Recorded two UDP clients with latency, jitter, loss + duplication\n";
}
