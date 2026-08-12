#pragma once

#include "Connection.h"
class Client {
public:
  Client(Address clientAddress);
  Connection connection = Connection();
  void initialize(Address serverAddress);
  void Update();

private:
};