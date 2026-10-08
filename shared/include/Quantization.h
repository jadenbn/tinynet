#pragma once
#include "GameTypes.h"
#include <array>
#include <bit>
#include <stdexcept>

inline uint16_t PackPosition(float value) {
  if (!std::isfinite(value) || std::abs(value) > WORLD_BOUND)
    throw std::invalid_argument("position outside world");
  return std::bit_cast<uint16_t>(static_cast<int16_t>(std::lround(value*100)));
}
inline float UnpackPosition(uint16_t value) {
  float position = std::bit_cast<int16_t>(value) / 100.0f;
  if (std::abs(position) > WORLD_BOUND)
    throw std::invalid_argument("position outside world");
  return position;
}

inline uint32_t PackQuaternion(Quat q) {
  float norm = Dot(q, q);
  if (!std::isfinite(norm) || norm <= 0)
    throw std::invalid_argument("invalid rotation");
  q = Normalize(q);
  std::array<float, 4> values{q.x, q.y, q.z, q.w};
  uint32_t largest = 0;
  for (uint32_t i = 1; i < 4; ++i)
    if (std::abs(values[i]) > std::abs(values[largest]))
      largest = i;
  float sign = values[largest] < 0 ? -1.0f : 1.0f;
  constexpr float bound = 0.7071067811865475f;
  uint32_t bits = largest;
  for (uint32_t i = 0; i < 4; ++i) {
    if (i == largest)
      continue;
    float unit = (values[i]*sign + bound) / (2*bound);
    uint32_t quantized = static_cast<uint32_t>(std::lround(std::clamp(unit, 0.0f, 1.0f)*1023));
    bits = (bits << 10) | quantized;
  }
  return bits;
}

inline Quat UnpackQuaternion(uint32_t bits) {
  uint32_t largest = bits >> 30;
  std::array<float, 4> values{};
  constexpr float bound = 0.7071067811865475f;
  float sum = 0;
  int shift = 20;
  for (uint32_t i = 0; i < 4; ++i) {
    if (i == largest)
      continue;
    float value = ((bits >> shift) & 1023) / 1023.0f * (2*bound) - bound;
    values[i] = value;
    sum += value*value;
    shift -= 10;
  }
  if (sum > 1.003f)
    throw std::invalid_argument("invalid packed rotation");
  values[largest] = std::sqrt(std::max(0.0f, 1-sum));
  return Normalize(Quat{values[0], values[1], values[2], values[3]});
}
