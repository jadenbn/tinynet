#include "Protocol.h"
#include "Quantization.h"
#include <cassert>
#include <iomanip>
#include <iostream>
#include <random>

int main() {
  for (float value : {-100.0f, -12.345f, 0.0f, 12.345f, 100.0f})
    assert(std::abs(UnpackPosition(PackPosition(value))-value) <= 0.00501f);
  assert(UnpackPosition(PackPosition(0)) == 0);
  for (float value : {NAN, INFINITY, 100.01f}) {
    bool rejected = false;
    try { PackPosition(value); }
    catch (const std::invalid_argument &) { rejected = true; }
    assert(rejected);
  }
  bool rejected = false;
  try { UnpackQuaternion(0); } // three components at -1/sqrt(2)
  catch (const std::invalid_argument &) { rejected = true; }
  assert(rejected);
  rejected = false;
  try { UnpackPosition(std::bit_cast<uint16_t>(int16_t{10001})); }
  catch (const std::invalid_argument &) { rejected = true; }
  assert(rejected);
  std::mt19937 rng(42);
  std::normal_distribution<float> normal;
  double maxAngle = 0;
  for (int i = 0; i < 10000; ++i) {
    Quat q = Normalize(Quat{normal(rng),normal(rng),normal(rng),normal(rng)});
    auto decoded = UnpackQuaternion(PackQuaternion(q));
    assert(std::abs(Dot(decoded, decoded)-1) < 0.000001f);
    double dot = std::abs(static_cast<double>(q.x)*decoded.x +
                         static_cast<double>(q.y)*decoded.y +
                         static_cast<double>(q.z)*decoded.z +
                         static_cast<double>(q.w)*decoded.w);
    double angle = 2*std::acos(std::clamp(dot, 0.0, 1.0));
    assert(angle < 0.006);
    maxAngle = std::max(maxAngle, angle);
    assert(PackQuaternion(q) == PackQuaternion({-q.x,-q.y,-q.z,-q.w}));
  }
  for (Quat q : {Quat{1,0,0,0}, Quat{0,1,0,0}, Quat{0,0,1,0}, Quat{0,0,0,1}})
    assert(std::abs(Dot(q, UnpackQuaternion(PackQuaternion(q)))) > 0.99999f);
  std::cout << "players  raw bytes  packed bytes  reduction (includes 16-byte header)\n";
  for (uint32_t count : {1u,10u,24u}) {
    WorldSnapshot snapshot;
    for (uint32_t id = 0; id < count; ++id)
      snapshot.players.emplace(id, PlayerState{id});
    uint8_t rawData[MAX_PACKET_SIZE], packedData[MAX_PACKET_SIZE];
    Buffer raw{rawData,0,0,sizeof(rawData)}, packed{packedData,0,0,sizeof(packedData)};
    snapshot.SerializeRaw(raw);
    snapshot.Serialize(packed);
    assert(raw.length == 9+count*48);
    assert(packed.length == 9+count*30);
    packed.index = 1;
    assert(WorldSnapshot::deserialize(packed).players.size() == count);
    double reduction = 100.0*(raw.length-packed.length)/(raw.length+16);
    std::cout << count << "        " << raw.length+16 << "         "
              << packed.length+16 << "          " << std::fixed
              << std::setprecision(1) << reduction << "%\n";
  }
  std::cout << "Maximum angular error in 10,000 seeded rotations: "
            << maxAngle*180/3.141592653589793 << " degrees\n";
  std::cout << "All quantization tests passed!\n";
}
