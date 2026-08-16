#include "Socket.h"
#include "fcntl.h"
#include <cstdlib>
#include <iostream>
#include <netinet/in.h>
#include <string>
#include <sys/socket.h>
#include <unistd.h>

Socket::Socket() { handle = -1; };
Socket::~Socket() { Close(); };

bool Socket::Open(unsigned short port) {
  handle = socket(AF_INET, SOCK_DGRAM, IPPROTO_UDP);

  if (handle <= 0) {
    std::cout << "Failed to create socket on port " << std::to_string(port)
              << std::endl;
    exit(EXIT_FAILURE);
  }

  sockaddr_in address;
  address.sin_family = AF_INET;
  address.sin_addr.s_addr = INADDR_ANY;
  address.sin_port = htons((unsigned short)port);

  if (bind(handle, (const sockaddr *)&address, sizeof(sockaddr_in)) < 0) {
    std::cout << "Failed to bind socket!" << '\n';
    Close();
    exit(EXIT_FAILURE);
  }

#pragma region Set_Packets_Non_Blocking
  // set packets to be non blocking, otherwise a function like recvfrom will not
  // return until a packet is available to read
#if defined(_WIN32)
  DWORD nonBlocking = 1;
  if (ioctlsocket(handle, FIONBIO, &nonBlocking) != 0) {
    Close();
    std::cout << "Failed to set non-blocking socket!" << std::endl;
    exit(EXIT_FAILURE);
    return false;
  }
#else
  if (fcntl(handle, F_SETFL, O_NONBLOCK) == -1) {
    std::cout << "Failed to set non-blocking socket!" << std::endl;
    Close();
    exit(EXIT_FAILURE);
  }
#endif

#pragma endregion

  return true;
}

bool Socket::isOpen() const { return handle > 0; }

void Socket::Close() {
#if defined(_WIN32)
  closesocket(handle);
#else
  close(handle);
#endif

  handle = -1;
}

bool Socket::Send(const Address &destination, const void *data, int size) {
  // turns our address from 192, 168, 0, ... to one unsigned integer with each
  // byte corresponding to input vals
  sockaddr_in addr;
  addr.sin_family = AF_INET;
  addr.sin_addr.s_addr = htonl(destination.GetAddress());
  addr.sin_port = htons(destination.GetPort());

  int sent_bytes = sendto(handle, (const char *)data, size, 0,
                          (sockaddr *)&addr, sizeof(sockaddr_in));

  if (sent_bytes != size) {
    std::cout << "Failed to send packet; is this port busy?" << std::endl;
    return false;
  }

  return true;
}

int Socket::Receive(Address &sender, void *data, int size) {
  sockaddr_in from;
  socklen_t fromLength = sizeof(from);

  // we need to make sure our largest packet < packet_data - if a 400 byte
  // packet comes in, we do NOT get the first 256 bytes; instead, our packet
  // will be skipped
  int read_bytes =
      recvfrom(handle, (char *)data, size, 0, (sockaddr *)&from, &fromLength);

  if (read_bytes <= 0) {
    return 0;
  }

  unsigned int address = ntohl(from.sin_addr.s_addr);
  unsigned short port = ntohs(from.sin_port);
  sender = Address(address, port);

  return read_bytes;
}