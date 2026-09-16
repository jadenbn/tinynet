#include "ClientGame.h"
#include <iostream>

int main(int argc, char *argv[]) {
  if (argc < 2) {
    std::cerr << "Error: Please provide a port for this client.\n";
    std::cerr << "Usage: " << argv[0] << " <number>\n";
    return 1;
  }

  Address serverAddress(127, 0, 0, 1,
                        3000); // def unsecure but fine for now i thiknks
  Address clientAddress(127, 0, 0, 1, std::stoi(argv[1]));
  ClientGame game(clientAddress, serverAddress);

  // game loop
  game.NetworkInit();
  game.GameInit();
  game.GameLoop();
  return 0;
}
