#include "../src/Socket.h"
#include <iostream>

int main() {
  unsigned short port = 3000;
  Socket socket;

  if (!socket.Open(port)) {
    std::cout << "Failed to open socket" << std::endl;
    return 1;
  }

  const char data[] = "Hello world!";
  socket.Send(Address(127, 0, 0, 1, port), data, sizeof(data));

  while (true) {
    Address sender;
    unsigned char buff[256];
    int bytes_read = socket.Receive(sender, buff, sizeof(buff));

    if (!bytes_read) {
      break;
    }

    // packet reaceived
  }
}