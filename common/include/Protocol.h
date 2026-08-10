#pragma once

#include <cstdint>
constexpr uint32_t PROTOCOL_HASH = 0x12345678;

enum class PacketType : uint8_t {
  ConnectionRequest = 1,
  Heartbeat,
};