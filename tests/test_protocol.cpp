#include "Protocol.h"
#include <cassert>
#include <iostream>
#include <stdexcept>

int main() {
  uint8_t data[MAX_PACKET_SIZE];
  Buffer buff{data, 0, 0, sizeof(data)};
  WorldSnapshot snapshot;
  snapshot.tick = 42;
  snapshot.players.emplace(7, PlayerState{7, {1,2,3}, {0,0,1}, Yaw(0.5f), 19});
  snapshot.Serialize(buff);
  assert(buff.length == 39);
  buff.index = 1;
  auto decoded = WorldSnapshot::deserialize(buff);
  assert(decoded.tick == 42 && decoded.players.at(7).lastInput == 19);
  assert(decoded.players.at(7).pos.x == 1);
  assert(decoded.players.at(7).velocity.z == 1);
  assert(std::abs(Dot(decoded.players.at(7).orientation,
                      snapshot.players.at(7).orientation)) > 0.99999f);
  int fullLength = buff.length;
  for (int length = 1; length < fullLength; ++length) {
    buff.index = 1;
    buff.length = length;
    bool rejected = false;
    try { WorldSnapshot::deserialize(buff); }
    catch (const std::invalid_argument &) { rejected = true; }
    assert(rejected);
  }
  buff = {data, 0, 0, sizeof(data)};
  PlayerInputPacket inputs{{{12, {0,0,1}, 0.5f},
                            {13, {}, 0.5f, {KickType::Chip, 200, -90}}}};
  inputs.Serialize(buff);
  assert(buff.length == 48);
  buff.index = 1;
  auto read = PlayerInputPacket::deserialize(buff);
  assert(read.inputs.size() == 2 && read.inputs.at(1).sequence == 13);
  assert(read.inputs.at(0).kick.type == KickType::None);
  assert(read.inputs.at(1).kick.type == KickType::Chip);
  assert(read.inputs.at(1).kick.power == 200 && read.inputs.at(1).kick.curve == -90);
  fullLength = buff.length;
  for (int length = 1; length < fullLength; ++length) {
    buff.index = 1;
    buff.length = length;
    bool rejected = false;
    try { PlayerInputPacket::deserialize(buff); }
    catch (const std::invalid_argument &) { rejected = true; }
    assert(rejected);
  }
  buff = {data, 0, 0, sizeof(data)};
  bool rejected = false;
  try { PlayerInputPacket{{{1, {NAN,0,0}, 0}}}.Serialize(buff); }
  catch (const std::invalid_argument &) { rejected = true; }
  assert(rejected);
  // a kick type that doesn't exist
  buff = {data, 0, 0, sizeof(data)};
  PlayerInputPacket{{{1, {}, 0}}}.Serialize(buff);
  data[buff.length - 3] = 7;
  buff.index = 1;
  rejected = false;
  try { PlayerInputPacket::deserialize(buff); }
  catch (const std::invalid_argument &) { rejected = true; }
  assert(rejected);
  std::cout << "All protocol tests passed!\n";
}
