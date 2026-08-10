#include "../src/Server.h"
#include <cassert>

void test_server_socket() {
  Server s(Address(127, 0, 0, 1, 3000));
  assert(s.getServerAddress().GetPort() == 3000);
  assert(s.getServerAddress().GetA() == 127);
}

void test_server_listenForClient() { Server s(Address(127, 0, 0, 1, 3000)); }

int main() {}