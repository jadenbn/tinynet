#include <iostream>

#define PLATFORM_WINDOWS 1
#define PLATFORM_MACOS 2
#define PLATFORM_UNIX 3

#if defined(_WIN32)
#define PLATFORM PLATFORM_WINDOWS

#elif defined(__APPLE__)
#define PLATFORM PLATFORM_MACOS

#else
#define PLATFORM PLATFORM_UNIX
#endif

int main()
{
    std::cout << "Hello, server" << std::endl;
}