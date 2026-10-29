#include "ClientGame.h"
#include <iostream>
#include <string>

int main(int argc, char *argv[]) {
  if (argc < 2) {
    std::cerr << "Error: Please provide a port for this client.\n";
    std::cerr << "Usage: " << argv[0] << " <number> [--bot]\n";
    return 1;
  }
  bool bot = argc > 2 && std::string(argv[2]) == "--bot";

  Address serverAddress(127, 0, 0, 1,
                        3000); // def unsecure but fine for now i thiknks
  Address clientAddress(127, 0, 0, 1, std::stoi(argv[1]));
  ClientGame game(clientAddress, serverAddress, bot);
  std::signal(SIGINT, [](int) { ClientGame::quitRequested = 1; });
  std::signal(SIGTERM, [](int) { ClientGame::quitRequested = 1; });

  // game loop
  game.NetworkInit();
  game.GameInit();
  game.GameLoop();
  return 0;
}
