#pragma once
#include "Address.h"
#include "NetworkSimulator.h"

class Socket {
public:
  Socket();
  ~Socket();

  bool Open(unsigned short port);

  bool isOpen() const;

  void Close();

  bool Send(const Address &destination, const void *data, int size);

  int Receive(Address &sender, void *data, int size);

  // fake network conditions, applied to packets going both ways
  void SetConditions(const NetworkConditions &conditions);
  NetworkConditions GetConditions() const;

  // sends anything the simulator is done delaying. call once a tick
  void Update();

private:
  int handle;
  NetworkSimulator outgoing;
  NetworkSimulator incoming;

  bool SendNow(const Address &destination, const void *data, int size);
  int ReceiveNow(Address &sender, void *data, int size);
};