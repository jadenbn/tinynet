#pragma once
#include "Connection.h"

class Address;

class Server {
public:
  Server(Address serverAddress);
  Connection connection = Connection();
  void initialize();
  void Update();

private:
};
