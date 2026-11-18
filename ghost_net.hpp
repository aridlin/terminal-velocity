#pragma once
// Versioned, fixed-size UDP snapshots. No remote state enters the physics
// engine.
#include "net_platform.hpp"
#include <algorithm>
#include <array>
#include <chrono>
#include <cmath>
#include <cstdint>
#include <cstring>
#include <map>
#include <stdexcept>
#include <string>
#include <vector>

namespace ghosts {
using Clock = std::chrono::steady_clock;
inline double seconds() {
  return std::chrono::duration<double>(Clock::now().time_since_epoch()).count();
}
inline bool valid_room(const std::string &s) {
  return !s.empty() && s.size() < 24 &&
         s.find_first_not_of("abcdefghijklmnopqrstuvwxyzABCDEFGHIJKLMNOPQRSTUVW"
                             "XYZ0123456789-_") == std::string::npos;
}
struct Snapshot {
  std::string room = "default";
  uint32_t id = 0, sequence = 0, level = 0, seed = 0, flags = 0, epoch = 0;
  double x = 0, y = 0;
};
using Packet = std::array<unsigned char, 60>;
inline void put(Packet &p, size_t at, uint32_t n) {
  for (int i = 3; i >= 0; --i) {
    p[at + i] = n & 255;
    n >>= 8;
  }
}
inline uint32_t get(const Packet &p, size_t at) {
  uint32_t n = 0;
  for (int i = 0; i < 4; ++i)
    n = (n << 8) | p[at + i];
  return n;
}
inline Packet encode(const Snapshot &s) {
  Packet p{};
  std::memcpy(p.data(), "TVG1", 4);
  std::memcpy(p.data() + 4, s.room.data(), std::min(size_t(23), s.room.size()));
  put(p, 28, s.id);
  put(p, 32, s.sequence);
  put(p, 36, s.level);
  put(p, 40, s.seed);
  put(p, 44, uint32_t(int32_t(std::lround(s.x * 1000))));
  put(p, 48, uint32_t(int32_t(std::lround(s.y * 1000))));
  put(p, 52, s.flags);
  put(p, 56, s.epoch);
  return p;
}
// The high bit separates adventure from all historical course IDs. The
// remaining fields compare content version, district, mode and template
// exactly; the packet's seed still identifies the generated layout.
inline uint32_t adventure_level(unsigned version,int district,int mode,int room_template) {
  return 0x80000000u|((version&0x7fffu)<<16)|(unsigned(district)<<12)|
    (unsigned(mode)<<10)|unsigned(room_template);
}
inline bool valid_level(uint32_t level) {
  return level<=5 || ((level&0x80000000u)&&((level>>16)&0x7fff)>0&&
    !(level&0x8000u)&&((level>>12)&7)<6&&((level>>10)&3)<3);
}
inline bool decode(const Packet &p, Snapshot &s) {
  if (std::memcmp(p.data(), "TVG1", 4) || p[27] != 0)
    return false;
  s.room =
      std::string(reinterpret_cast<const char *>(p.data() + 4),
                  strnlen(reinterpret_cast<const char *>(p.data() + 4), 24));
  if (!valid_room(s.room))
    return false;
  // Reject hidden data after a room terminator as well.
  for (size_t i = 4 + s.room.size(); i < 28; ++i)
    if (p[i])
      return false;
  s.id = get(p, 28);
  s.sequence = get(p, 32);
  s.level = get(p, 36);
  s.seed = get(p, 40);
  s.x = int32_t(get(p, 44)) / 1000.;
  s.y = int32_t(get(p, 48)) / 1000.;
  s.flags = get(p, 52);
  s.epoch = get(p, 56);
  return valid_level(s.level) && s.flags <= 7 && s.x >= -64 && s.x <= 4096 &&
         s.y >= -64 && s.y <= 512;
}
inline bool newer(uint32_t a, uint32_t b) {
  return a != b && uint32_t(a - b) < 0x80000000u;
}
inline bool same_course(const Snapshot &a, const Snapshot &b) {
  return a.room == b.room && a.level == b.level && a.seed == b.seed;
}
struct Ghost {
  Snapshot state;
  double x = 0, y = 0, seen = 0;
};
class Client {
  Socket fd_ = invalid_socket;
  uint32_t sequence_ = 0;
  double send_at_ = 0;
  Snapshot course_;
  std::map<uint32_t, Ghost> peers_;

public:
  Client() = default;
  Client(const Client &) = delete;
  Client &operator=(const Client &) = delete;
  ~Client() {
    if (fd_ != invalid_socket)
      close_socket(fd_);
  }
  void connect_to(const std::string &host, const std::string &port) {
    init_sockets();
    addrinfo hints{}, *addresses = nullptr;
    hints.ai_family = AF_INET;
    hints.ai_socktype = SOCK_DGRAM;
    int error = getaddrinfo(host.c_str(), port.c_str(), &hints, &addresses);
    if (error)
      throw std::runtime_error(std::string("Ghost relay: ") +
                               gai_strerror(error));
    for (auto a = addresses; a; a = a->ai_next) {
      Socket fd = udp_socket();
      if (fd != invalid_socket &&
          ::connect(fd, a->ai_addr, a->ai_addrlen) == 0) {
        fd_ = fd;
        break;
      }
      if (fd != invalid_socket)
        close_socket(fd);
    }
    freeaddrinfo(addresses);
    if (fd_ == invalid_socket)
      throw std::runtime_error("Cannot open ghost relay socket");
  }
  bool enabled() const { return fd_ != invalid_socket; }
  void tick(Snapshot local, double dt, double now = seconds()) {
    if (!enabled())
      return;
    if (!same_course(local, course_)) {
      peers_.clear();
      send_at_ = 0;
    }
    course_ = local;
    if (now >= send_at_) {
      local.sequence = ++sequence_;
      auto p = encode(local);
      send_packet(fd_, p.data(), p.size());
      send_at_ = now + 1. / 30;
    }
    for (int i = 0; i < 64; ++i) {
      Packet p{};
      // MSG_TRUNC ensures oversized packets are rejected, not silently clipped.
      auto n = receive_packet(fd_, p.data(), p.size());
      if (n == -2)
        continue;
      if (n < 0)
        break;
      Snapshot s;
      if (n != int(p.size()) || !decode(p, s) || !s.id ||
          !same_course(local, s))
        continue;
      auto it = peers_.find(s.id);
      if (it == peers_.end()) {
        if (peers_.size() >= 32)
          continue;
        peers_[s.id] = {s, s.x, s.y, now};
      } else if (newer(s.sequence, it->second.state.sequence)) {
        auto &g = it->second;
        if (g.state.epoch != s.epoch || std::hypot(g.x - s.x, g.y - s.y) > 5) {
          g.x = s.x;
          g.y = s.y;
        }
        g.state = s;
        g.seen = now;
      }
    }
    for (auto it = peers_.begin(); it != peers_.end();) {
      auto &g = it->second;
      if (now - g.seen > 3) {
        it = peers_.erase(it);
        continue;
      }
      double blend = 1 - std::exp(-30 * std::clamp(dt, 0., .1));
      g.x += (g.state.x - g.x) * blend;
      g.y += (g.state.y - g.y) * blend;
      ++it;
    }
  }
  std::vector<Ghost> visible() const {
    std::vector<Ghost> result;
    for (const auto &[id, g] : peers_)
      if (g.state.flags & 2)
        result.push_back(g);
    return result;
  }
};

// A bounded relay for friends/LAN games. Endpoint ownership is assigned here;
// clients cannot choose another player's ID. Rooms are filters, not passwords.
class Relay {
  struct Peer {
    sockaddr_in address{};
    Snapshot state;
    double seen = 0, forwarded = -1;
  };
  Socket fd_ = invalid_socket;
  uint32_t next_id_ = 0;
  std::map<std::pair<uint32_t, uint16_t>, Peer> peers_;

public:
  explicit Relay(uint16_t port) {
    fd_ = udp_socket();
    sockaddr_in address{};
    address.sin_family = AF_INET;
    address.sin_addr.s_addr = htonl(INADDR_ANY);
    address.sin_port = htons(port);
    if (fd_ == invalid_socket ||
        bind(fd_, reinterpret_cast<sockaddr *>(&address), sizeof(address)) <
            0) {
      if (fd_ != invalid_socket)
        close_socket(fd_);
      throw std::runtime_error("Cannot bind ghost relay UDP port");
    }
  }
  Relay(const Relay &) = delete;
  Relay &operator=(const Relay &) = delete;
  ~Relay() { close_socket(fd_); }
  uint16_t port() const {
    sockaddr_in a{};
    SocketLength n = sizeof(a);
    getsockname(fd_, reinterpret_cast<sockaddr *>(&a), &n);
    return ntohs(a.sin_port);
  }
  void poll(double now = seconds()) {
    for (auto it = peers_.begin(); it != peers_.end();)
      if (now - it->second.seen > 3)
        it = peers_.erase(it);
      else
        ++it;
    for (int i = 0; i < 128; ++i) {
      Packet p{};
      sockaddr_in address{};
      auto n = receive_packet(fd_, p.data(), p.size(), &address);
      if (n == -2)
        continue;
      if (n < 0)
        break;
      Snapshot s;
      if (n != int(p.size()) || !decode(p, s))
        continue;
      auto key = std::make_pair(address.sin_addr.s_addr, address.sin_port);
      auto it = peers_.find(key);
      if (it == peers_.end()) {
        if (peers_.size() >= 32)
          continue;
        s.id = ++next_id_;
        it = peers_.emplace(key, Peer{address, s, now, -1}).first;
      } else {
        if (!newer(s.sequence, it->second.state.sequence))
          continue;
        s.id = it->second.state.id;
        it->second.state = s;
        it->second.seen = now;
      }
      auto &peer = it->second;
      if (now - peer.forwarded < 1. / 35)
        continue;
      peer.forwarded = now;
      p = encode(s);
      for (const auto &[other, recipient] : peers_)
        if (other != key && same_course(s, recipient.state))
          send_packet(fd_, p.data(), p.size(), &recipient.address);
    }
  }
};
} // namespace ghosts
