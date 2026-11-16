#pragma once
#include <cstddef>
#include <stdexcept>
#ifdef _WIN32
#ifndef NOMINMAX
#define NOMINMAX
#endif
#ifndef WIN32_LEAN_AND_MEAN
#define WIN32_LEAN_AND_MEAN
#endif
#include <windows.h>
#include <winsock2.h>
#include <ws2tcpip.h>
#else
#include <netdb.h>
#include <sys/socket.h>
#include <unistd.h>
#endif

namespace ghosts {
#ifdef _WIN32
using Socket = SOCKET;
using SocketLength = int;
constexpr Socket invalid_socket = INVALID_SOCKET;
inline void close_socket(Socket fd) { closesocket(fd); }
inline void init_sockets() {
  struct Runtime {
    Runtime() {
      WSADATA data{};
      if (WSAStartup(MAKEWORD(2, 2), &data))
        throw std::runtime_error("Cannot initialize Windows networking");
    }
    ~Runtime() { WSACleanup(); }
  };
  static Runtime runtime;
}
#else
using Socket = int;
using SocketLength = socklen_t;
constexpr Socket invalid_socket = -1;
inline void close_socket(Socket fd) { close(fd); }
inline void init_sockets() {}
#endif
inline Socket udp_socket() {
  init_sockets();
#ifdef _WIN32
  auto fd = socket(AF_INET, SOCK_DGRAM, IPPROTO_UDP);
  if (fd == invalid_socket)
    return fd;
  u_long nonblocking = 1;
  if (ioctlsocket(fd, FIONBIO, &nonblocking)) {
    close_socket(fd);
    return invalid_socket;
  }
  SetHandleInformation(reinterpret_cast<HANDLE>(fd), HANDLE_FLAG_INHERIT, 0);
  return fd;
#else
  return socket(AF_INET, SOCK_DGRAM | SOCK_NONBLOCK | SOCK_CLOEXEC, 0);
#endif
}
inline int send_packet(Socket fd, const void *bytes, size_t size,
                       const sockaddr_in *address = nullptr) {
  auto data = static_cast<const char *>(bytes);
  return address ? int(sendto(fd, data, int(size), 0,
                              reinterpret_cast<const sockaddr *>(address),
                              sizeof(*address)))
                 : int(send(fd, data, int(size), 0));
}
// -1: no data/socket error; -2: oversized Windows datagram was discarded.
// POSIX MSG_TRUNC returns the original size so the caller rejects it.
inline int receive_packet(Socket fd, void *bytes, size_t size,
                          sockaddr_in *address = nullptr) {
  SocketLength length = sizeof(sockaddr_in);
#ifdef _WIN32
  constexpr int flags = 0;
#else
  constexpr int flags = MSG_TRUNC;
#endif
  auto data = static_cast<char *>(bytes);
  int n = address
              ? int(recvfrom(fd, data, int(size), flags,
                             reinterpret_cast<sockaddr *>(address), &length))
              : int(recv(fd, data, int(size), flags));
#ifdef _WIN32
  if (n == SOCKET_ERROR && WSAGetLastError() == WSAEMSGSIZE)
    return -2;
#endif
  return n;
}
} // namespace ghosts
