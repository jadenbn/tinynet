#include "NetworkSimulator.h"
#include <algorithm>
#include <cstring>

bool NetworkSimulator::IsActive() const {
  return conditions.latency > 0.0f || conditions.jitter > 0.0f ||
         conditions.loss > 0.0f || conditions.duplicate > 0.0f ||
         !queue.empty();
}

float NetworkSimulator::Roll() {
  return std::uniform_real_distribution<float>(0.0f, 1.0f)(rng);
}

void NetworkSimulator::Push(const Address &address, const void *data,
                            int size) {
  if (Roll() < conditions.loss) {
    return;
  }

  Schedule(address, data, size);

  if (Roll() < conditions.duplicate) {
    Schedule(address, data, size);
  }
}

void NetworkSimulator::Schedule(const Address &address, const void *data,
                                int size) {
  // every copy gets its own jitter, so packets can overtake each other like
  // they would on a real network
  float delay = conditions.latency + (Roll() * 2.0f - 1.0f) * conditions.jitter;
  delay = std::max(delay, 0.0f);

  auto deliverAt =
      std::chrono::steady_clock::now() +
      std::chrono::duration_cast<std::chrono::steady_clock::duration>(
          std::chrono::duration<float, std::milli>(delay));

  const uint8_t *bytes = static_cast<const uint8_t *>(data);
  queue.push_back(
      {deliverAt, address, std::vector<uint8_t>(bytes, bytes + size)});
}

int NetworkSimulator::Pop(Address &address, void *data, int size) {
  auto now = std::chrono::steady_clock::now();

  // the earliest packet that's due. with jitter, that isn't always the one
  // that was pushed first
  auto next = queue.end();
  for (auto it = queue.begin(); it != queue.end(); it++) {
    if (it->deliverAt <= now &&
        (next == queue.end() || it->deliverAt < next->deliverAt)) {
      next = it;
    }
  }

  if (next == queue.end()) {
    return 0;
  }

  int bytes = std::min(static_cast<int>(next->data.size()), size);
  std::memcpy(data, next->data.data(), bytes);
  address = next->address;
  queue.erase(next);
  return bytes;
}
