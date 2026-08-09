#pragma once
#include "Address.h"

class Socket {
public:
  Socket();
  ~Socket();

  bool Open(unsigned short port);

  bool isOpen() const;

  void Close();

  bool Send(const Address &destination, const void *data, int size);

  int Receive(Address &sender, void *data, int size);

private:
  int handle;
};