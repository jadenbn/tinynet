#include "../src/Socket.h" // change later, i don't know if i like this relative path schtick
#include <cassert>
#include <chrono>
#include <iostream>
#include <thread>

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

  assert(socket.Open(3000));
  assert(socket.isOpen());

  socket.Close();
  assert(!socket.isOpen());
}

void test_one_socket() {
  Socket socket;
  const unsigned short port = 3000;
  assert(socket.Open(port));

  Address sender;
  Address receiver(127, 0, 0, 1, port);

  char dataSent[] = "Testing socket!";
  auto dataSentSize = sizeof(dataSent);

  char receiveBuffer[256];

  assert(socket.Send(receiver, dataSent, dataSentSize));

  int bytesReceived = 0;
  int attempts = 0;

  while (bytesReceived == 0 && attempts < 100) {
    bytesReceived =
        socket.Receive(sender, receiveBuffer, sizeof(receiveBuffer));
    attempts++;
    std::this_thread::sleep_for(std::chrono::milliseconds(10));
  }

  assert(bytesReceived > 0);
}

void test_two_socket() {
  Socket sender;
  Socket receiver;
  assert(sender.Open(3000));
  assert(receiver.Open(3001));

  Address senderAddress;
  Address receiverAddress(127, 0, 0, 1, 3001);

  constexpr char data[] = "Ping";
  sender.Send(receiverAddress, data, sizeof(data));

  char receiveBuff[256];

  int bytesReceived = 0;
  int attempts = 0;

  while (bytesReceived == 0 && attempts < 100) {
    bytesReceived =
        receiver.Receive(senderAddress, receiveBuff, sizeof(receiveBuff));
    attempts++;
    std::this_thread::sleep_for(std::chrono::milliseconds(10));
  }

  assert(bytesReceived > 0);
  assert(senderAddress.GetPort() == 3000);
}

int main() {
  test_address();
  test_socket_lifecycle();
  test_one_socket();
  test_two_socket();
  std::cout << "All tests passed!" << std::endl;
  return 0;
}