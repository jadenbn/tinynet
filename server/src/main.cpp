#include "GameServer.h"
#include <chrono>
#include <csignal>
#include <iostream>
#include <thread>

namespace {
volatile std::sig_atomic_t running = 1;
void Stop(int) { running = 0; }
} // namespace

int main() {
  std::signal(SIGINT, Stop);
  std::signal(SIGTERM, Stop);
  GameServer game(Address(127, 0, 0, 1, 3000));
  game.Initialize();
  using Clock = std::chrono::steady_clock;
  auto previous = Clock::now();
  double accumulator = 0;
  std::cout << "Starting server on 127.0.0.1:3000\n";
  while (running) {
    auto now = Clock::now();
    accumulator += std::min(std::chrono::duration<double>(now-previous).count(), 0.25);
    previous = now;
    game.ReceivePackets();
    while (accumulator >= TICK_DT) {
      game.Step();
      accumulator -= TICK_DT;
    }
    std::this_thread::sleep_for(std::chrono::milliseconds(1));
  }

  // ctrl+c: let everyone know instead of leaving them to time out
  int count = game.Shutdown();
  std::cout << "\nServer stopped, disconnected " << count << " clients\n";
}
