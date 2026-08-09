#include "../src/Socket.h" // change later, i don't know if i like this relative path schtick
#include <cassert>
#include <iostream>

void test_address() {
  Address addr(127, 0, 0, 1, 3000);
  assert(addr.GetA() == 127);
  assert(addr.GetB() == 0);
  assert(addr.GetC() == 0);
  assert(addr.GetD() == 1);
  assert(addr.GetPort() == 3000);
}

void test_socket_lifecycle() {
  Socket socket;
  assert(!socket.isOpen());

  int handle = socket.Open(3000);
  assert(handle == 3000);
  assert(socket.isOpen());

  socket.Close();
  assert(!socket.isOpen());
}

void test_one_socket();

int main() {
  constexpr short port = 30000;

  Socket socket;
  if (!socket.Open(port)) {
    return 1;
  }

  Address sender;
  Address receiver(127, 0, 0, 1, port);

  char dataSent[] = "Hello, world!";
  const auto sizeSent = sizeof(dataSent);

  char dataReceived[256];
  const auto sizeReceived = sizeof(dataSent);

  for (int i = 0; i < 10; i++) {
    socket.Send(receiver, dataSent, sizeSent);
    int bytesReceived = socket.Receive(sender, dataReceived, sizeReceived);
    if (bytesReceived == 0) {
      std::cout << "No bytes found!\n";
      continue;
    }
    std::cout << "Receiving: ";
    for (int j = 0; j < bytesReceived; j++) {
      std::cout << dataReceived[j];
    }
    std::cout << std::endl;
  }

  return 0;
}