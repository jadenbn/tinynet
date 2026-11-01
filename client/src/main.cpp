#include "ClientGame.h"
#include <iostream>
#include <string>

int main(int argc, char *argv[]) {
  if (argc < 2) {
    std::cerr << "Error: Please provide a port for this client.\n";
    std::cerr << "Usage: " << argv[0] << " <number> [--bot] [--preset 1-4]\n";
    return 1;
  }
  bool bot = false;
  int preset = 1;
  for (int i = 2; i < argc; i++) {
    std::string arg = argv[i];
    if (arg == "--bot")
      bot = true;
    else if (arg == "--preset" && i + 1 < argc)
      preset = std::clamp(std::stoi(argv[++i]), 1, 4);
  }

  Address serverAddress(127, 0, 0, 1,
                        3000); // def unsecure but fine for now i thiknks
  Address clientAddress(127, 0, 0, 1, std::stoi(argv[1]));
  ClientGame game(clientAddress, serverAddress, bot);
  std::signal(SIGINT, [](int) { ClientGame::quitRequested = 1; });
  std::signal(SIGTERM, [](int) { ClientGame::quitRequested = 1; });

  // game loop
  game.NetworkInit();
  game.SetConditions(NetDebugOverlay::PRESETS[preset - 1]);
  game.GameInit();
  game.GameLoop();
  return 0;
}
