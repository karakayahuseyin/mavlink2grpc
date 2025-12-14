/**
 * Copyright (C) 2025 Hüseyin Karakaya
 * This file is part of the mavlink2grpc project licensed under the MIT License.
 */

#include "UdpTransport.h"

#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <netdb.h>
#include <fcntl.h>
#include <unistd.h>

#include <cstring>
#include <cerrno>
#include <algorithm>
#include <iostream>

namespace mav2grpc {
UdpTransport::UdpTransport(uint16_t local_port,
                            std::string bind_address,
                            bool broadcast_enabled)
  : socket_fd_(-1), local_port_(local_port), 
    bind_address_(std::move(bind_address)), 
    broadcast_enabled_(broadcast_enabled) {}

UdpTransport::~UdpTransport() {
  close();
}

bool UdpTransport::open() {
  if (is_open()) {
    return true; // Already open
  }

  socket_fd_ = socket(AF_INET, SOCK_DGRAM, 0);
  if (socket_fd_ < 0) {
    std::cerr << "Failed to create UDP socket: " << std::strerror(errno) << "\n";
    return false;
  }

  if (!configure_socket()) {
    ::close(socket_fd_);
    socket_fd_ = -1;
    return false;
  }

  sockaddr_in local_addr{};
  local_addr.sin_family = AF_INET;
  local_addr.sin_port = htons(local_port_);
  inet_pton(AF_INET, bind_address_.c_str(), &local_addr.sin_addr);

  if (bind(socket_fd_, reinterpret_cast<sockaddr*>(&local_addr), sizeof(local_addr)) < 0) {
    std::cerr << "Failed to bind UDP socket to port " << local_port_ 
              << ": " << std::strerror(errno) << "\n";
    ::close(socket_fd_);
    socket_fd_ = -1;
    return false;
  }

  return true;
}

void UdpTransport::close() {
  if (is_open()) {
    ::close(socket_fd_);
    socket_fd_ = -1;
    remote_endpoints_.clear();
  }
}

bool UdpTransport::is_open() const {
  return socket_fd_ >= 0;
}

ssize_t UdpTransport::read(std::span<std::byte> buffer) {
  if (!is_open()) {
    std::cerr << "UDP socket not open for reading.\n";
    return -1;
  }

  sockaddr_in sender_addr{};
  socklen_t addr_len = sizeof(sender_addr);
  ssize_t bytes_received = recvfrom(socket_fd_, buffer.data(), buffer.size(), 0,
                                    reinterpret_cast<sockaddr*>(&sender_addr), &addr_len);

  if (bytes_received < 0) {
    if (errno != EAGAIN && errno != EWOULDBLOCK) {
      std::cerr << "Error receiving UDP datagram: " << std::strerror(errno) << "\n";
      return -1;
    }
    return 0; // No data available
  }

  add_endpoint_if_new(sender_addr);
  return bytes_received;
}

ssize_t UdpTransport::write(std::span<const std::byte> data) {
  if (!is_open()) {
    std::cerr << "UDP socket not open for writing.\n";
    return -1;
  }

  if (remote_endpoints_.empty() && !broadcast_enabled_) {
    std::cerr << "No remote endpoints to send UDP datagram.\n";
    return -1;
  }

  ssize_t bytes_sent = -1;
  for (const auto& endpoint : remote_endpoints_) {
    bytes_sent = sendto(socket_fd_, data.data(), data.size(), 0,
                        reinterpret_cast<const sockaddr*>(&endpoint), sizeof(endpoint));
    if (bytes_sent < 0) {
      std::cerr << "Error sending UDP datagram: " << std::strerror(errno) << "\n";
      return -1;
    }
  }

  // If no endpoints and broadcast enabled, send to broadcast address
  if (remote_endpoints_.empty() && broadcast_enabled_) {
    sockaddr_in broadcast_addr{};
    broadcast_addr.sin_family = AF_INET;
    broadcast_addr.sin_port = htons(local_port_);
    inet_pton(AF_INET, "255.255.255.255", &broadcast_addr.sin_addr);

    bytes_sent = sendto(socket_fd_, data.data(), data.size(), 0,
                        reinterpret_cast<const sockaddr*>(&broadcast_addr), sizeof(broadcast_addr));
    if (bytes_sent < 0) {
      std::cerr << "Error sending UDP broadcast datagram: " << std::strerror(errno) << "\n";
      return -1;
    }
  }

  return bytes_sent;
}

bool UdpTransport::add_remote_endpoint(const std::string& host, uint16_t port) {
  addrinfo hints{}, *res = nullptr;
  hints.ai_family = AF_INET;
  hints.ai_socktype = SOCK_DGRAM;

  int status = getaddrinfo(host.c_str(), nullptr, &hints, &res);
  if (status != 0 || res == nullptr) {
    std::cerr << "Failed to resolve host " << host << ": " << gai_strerror(status) << "\n";
    return false;
  }

  sockaddr_in addr{};
  addr.sin_family = AF_INET;
  addr.sin_port = htons(port);
  addr.sin_addr = reinterpret_cast<sockaddr_in*>(res->ai_addr)->sin_addr;

  freeaddrinfo(res);
  add_endpoint_if_new(addr);
  return true;
}

void UdpTransport::clear_remote_endpoints() {
  remote_endpoints_.clear();
}

bool UdpTransport::configure_socket() {
  int optval = 1;

  // Enable SO_REUSEADDR
  if (setsockopt(socket_fd_, SOL_SOCKET, SO_REUSEADDR, &optval, sizeof(optval)) < 0) {
    std::cerr << "Failed to set SO_REUSEADDR: " << std::strerror(errno) << "\n";
    return false;
  }

  // Enable SO_BROADCAST if requested
  if (broadcast_enabled_) {
    if (setsockopt(socket_fd_, SOL_SOCKET, SO_BROADCAST, &optval, sizeof(optval)) < 0) {
      std::cerr << "Failed to set SO_BROADCAST: " << std::strerror(errno) << "\n";
      return false;
    }
  }

  // Set non-blocking mode
  int flags = fcntl(socket_fd_, F_GETFL, 0);
  if (flags < 0) {
    std::cerr << "Failed to get socket flags: " << std::strerror(errno) << "\n";
    return false;
  }
  if (fcntl(socket_fd_, F_SETFL, flags | O_NONBLOCK) < 0) {
    std::cerr << "Failed to set non-blocking mode: " << std::strerror(errno) << "\n";
    return false;
  }

  return true;
}

void UdpTransport::add_endpoint_if_new(const sockaddr_in& addr) {
  auto it = std::find_if(remote_endpoints_.begin(), remote_endpoints_.end(),
                         [&addr](const sockaddr_in& existing_addr) {
                           return existing_addr.sin_addr.s_addr == addr.sin_addr.s_addr &&
                                  existing_addr.sin_port == addr.sin_port;
                         });
  if (it == remote_endpoints_.end()) {
    remote_endpoints_.push_back(addr);
  }
}

} // namespace mav2grpc
