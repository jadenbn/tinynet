#include <iostream>

// os setup
#define PLATFORM_WINDOWS 1
#define PLATFORM_MAC 2
#define PLATFORM_UNIX 3

#if defined(_WIN32)
#define PLATFORM PLATFORM_WINDOWS

#elif defined(__APPLE__)
#define PLATFORM PLATFORM_MAC

#else
#define PLATFORM PLATFORM_UNIX
#endif

#if PLATFORM == PLATFORM_WINDOWS
#include <winsock2.h>

#else
#include <sys/socket.h>
#include <netinet/in.h>
#include <fcntl.h>
#include <unistd.h>
#endif

#if PLATFORM == PLATFORM_WINDOWS
// link winsock library on windows; we don't need to do this on unix platforms as they (sockets) are built in to the standard system libraries
#pragma comment(lib, "wsock32.lib")
#endif

// todo: probbaly move these to be RAII friendly once ready
bool InitializeSockets()
{
#if PLATFORM == PLATFORM_WINDOWS
    WSADATA WsaData;
    return WSAStartup(MAKEWORD(2, 2),
                      &WsaData) == NO_ERROR;
#else
    return true;
#endif
}

void ShutdownSockets()
{
#if PLATFORM == PLATFORM_WINDOWS
    WSACleanup();
#endif
}

int main()
{

    // 207.45.186.98:30000 example destination address
    // note that 0 <= a, b, c, d <= 255
    unsigned int a = 207;
    unsigned int b = 45;
    unsigned int c = 186;
    unsigned int d = 98;
    unsigned short port = 30000;

    int handle = socket(AF_INET, SOCK_DGRAM, IPPROTO_UDP);

    if (handle <= 0)
    {
        std::cout << "Failed to create socket!" << '\n';
        return 1;
    }

    sockaddr_in address;
    address.sin_family = AF_INET;
    address.sin_addr.s_addr = INADDR_ANY;
    address.sin_port = htons((unsigned short)port);

    if (bind(handle, (const sockaddr *)&address, sizeof(sockaddr_in)) < 0)
    {
        std::cout << "Failed to bind socket!" << '\n';
        return 1;
    }
#pragma region Set_Packets_Non_Blocking
    // set packets to be non blocking, otherwise a function like recvfrom will not return until a packet is available to read
#if PLATFORM == PLATFORM_MAC || PLATFORM == PLATFORM_UNIX
    int nonBlocking = 1;

    if (fcntl(handle, F_SETFL, O_NONBLOCK, nonBlocking) == -1)
    {
        std::cout << "Failed to set non-blocking socket!" << std::endl;
        return false;
    }

#elif PLATFORM == PLATFORM_WINDOWS

    DWORD nonBlocking = 1;
    if (ioctlsocket(handle, FIONBIO, &nonBlocking) != 0)
    {
        std::cout << "Failed to set non-blocking socket!" << std::endl;
        return false;
    }

#endif
#pragma endregion

#pragma region Sending_Packets
    // turns our address from 207, 45, 186, ... to one unsigned integer with each byte corresponding to input vals
    // uint32_t address = ((a << 24) ||
    //                     (b << 16) ||
    //                     (c << 8) ||
    //                     d);

    // sockaddr_in addr;
    // addr.sin_family = AF_INET;
    // addr.sin_addr.s_addr = htonl(address);
    // addr.sin_port = htons(port);

    // int sent_bytes = sendto(
    //     handle,
    //     (const char *)packet_data,
    //     packet_size,
    //     0,
    //     (sockaddr *)&address,
    //     sizeof(sockaddr_in));

    // if (sent_bytes != packet_size)
    // {
    //     std::cout << "Failed to send packet" << std::endl;
    //     return 1;
    // }
#pragma endregion

    // receive packets
    while (true)
    {
        unsigned char packet_data[256];
        unsigned int max_packet_size = sizeof(packet_data);

#if PLATFORM == PLATFORM_WINDOWS
        typedef int socklen_t;
#endif

        sockaddr_in from;
        socklen_t fromLength = sizeof(from);

        int bytes = recvfrom(handle,
                             (char *)packet_data,
                             max_packet_size,
                             0,
                             (sockaddr *)&from,
                             &fromLength);

        if (bytes <= 0)
            break;

        unsigned int from_address = ntohl(from.sin_addr.s_addr);
        unsigned int from_port = ntohs(from.sin_port);

#if PLATFORM == PLATFORM_MAC || PLATFORM == PLATFORM_UNIX
        close(handle);
#elif PLATFORM == PLATFORM_WINDOWS
        closesocket(socket);
#endif
    }
}