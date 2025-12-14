/**
 * Copyright (C) 2025 Hüseyin Karakaya
 * This file is part of the mavlink2grpc project licensed under the MIT License.
 */

/**
 * @file UdpTransport.h
 * @brief UDP socket transport implementation.
 */

#pragma once

#include "Transport.h"
#include <array>
#include <cstdint>
#include <netinet/in.h>
#include <string>
#include <vector>

namespace mav2grpc {

/**
 * @brief UDP socket transport implementation.
 *
 * Implements connection-less UDP socket communication for MAVLink traffic.
 * Supports both unicast and broadcast modes. Commonly used for ground
 * control station communication on local networks.
 *
 * @note This transport is inherently unreliable - packets may be lost,
 *       duplicated, or reordered. The MAVLink protocol handles retransmission
 *       at the application layer.
 *
 * @note Not thread-safe by default. External synchronization required if
 *       accessed from multiple threads.
 */
class UdpTransport : public Transport {
public:
  /**
   * @brief Default receive buffer size (4KB).
   *
   * Large enough for multiple MAVLink packets per datagram while
   * staying under typical MTU limits.
   */
  static constexpr size_t DEFAULT_BUFFER_SIZE = 4096;

  /**
   * @brief Constructs a UDP transport.
   *
   * @param local_port Port to bind to for receiving datagrams.
   *                   Common MAVLink ports: 14550 (GCS), 14555 (companion).
   * @param bind_address Local interface to bind. Use "0.0.0.0" for all
   *                     interfaces, or specific IP for single interface.
   * @param broadcast_enabled Enable SO_BROADCAST socket option for
   *                          broadcast transmission capability.
   */
  explicit UdpTransport(uint16_t local_port,
                        std::string bind_address = "0.0.0.0",
                        bool broadcast_enabled = false);

  /**
   * @brief Destructor - ensures socket is closed.
   */
  ~UdpTransport() override;

  /**
   * @brief Opens the UDP socket and binds to the configured port.
   *
   * Creates a UDP socket, sets SO_REUSEADDR for port sharing, enables
   * SO_BROADCAST if requested, and binds to the local port. Configures
   * the socket for non-blocking operation.
   *
   * @return true if socket created and bound successfully, false otherwise.
   */
  bool open() override;

  /**
   * @brief Closes the UDP socket.
   *
   * Shuts down the socket and clears the list of known remote endpoints.
   * Safe to call multiple times.
   */
  void close() override;

  /**
   * @brief Checks if the UDP socket is currently open.
   *
   * @return true if socket is open and ready for I/O, false otherwise.
   */
  bool is_open() const override;

  /**
   * @brief Receives a datagram from the socket (non-blocking).
   *
   * Uses recvfrom() to receive a datagram. If data is available, copies
   * the payload to the provided buffer and stores the sender's address
   * in the remote endpoints list for future transmission.
   *
   * @param buffer Span to receive datagram payload. Size should be at
   *               least MTU (1500 bytes) or larger to avoid truncation.
   *
   * @return Number of bytes received (0 if no data available), or -1 on error.
   *
   * @note EAGAIN/EWOULDBLOCK is normal for non-blocking sockets with no data.
   * @note Datagrams larger than buffer size will be truncated.
   */
  ssize_t read(uint8_t* buffer, size_t buffer_size) override;

  /**
   * @brief Sends a datagram to all known remote endpoints.
   *
   * Uses sendto() to transmit data to each remote endpoint that has
   * previously sent data to this socket. If no endpoints are known and
   * broadcast is enabled, may broadcast the message.
   *
   * @param data Pointer to data to transmit.
   * @param data_size Number of bytes to transmit.
   *
   * @return Number of bytes sent to the first endpoint, or -1 on error.
   *
   * @note All-or-nothing semantics: entire datagram sent or error.
   * @note If data size exceeds MTU, fragmentation may occur at IP layer.
   */
  ssize_t write(const uint8_t* data, size_t data_size) override;

  /**
   * @brief Manually adds a remote endpoint for transmission.
   *
   * Registers a specific IP address and port as a target for outgoing
   * datagrams. Useful for establishing initial connection before any
   * data has been received.
   *
   * @param host Remote hostname or IP address.
   * @param port Remote UDP port number.
   *
   * @return true if endpoint added successfully, false on resolution failure.
   */
  bool add_remote_endpoint(const std::string& host, uint16_t port);

  /**
   * @brief Returns the number of known remote endpoints.
   *
   * @return Count of unique remote addresses that have sent data or
   *         been manually added.
   */
  size_t remote_endpoint_count() const { return remote_endpoints_.size(); }

  /**
   * @brief Clears all known remote endpoints.
   *
   * Removes all remote addresses from the endpoint list. Useful for
   * resetting connection state.
   */
  void clear_remote_endpoints();

private:
  /**
   * @brief Configures socket options (SO_REUSEADDR, SO_BROADCAST, non-blocking).
   *
   * @return true if all options set successfully, false otherwise.
   */
  bool configure_socket();

  /**
   * @brief Adds a sockaddr_in to the remote endpoints list if not present.
   *
   * @param addr Socket address structure to add.
   */
  void add_endpoint_if_new(const sockaddr_in& addr);

  int socket_fd_;                              ///< Socket file descriptor (-1 if closed)
  uint16_t local_port_;                        ///< Port to bind to
  std::string bind_address_;                   ///< Local interface address
  bool broadcast_enabled_;                     ///< SO_BROADCAST flag
  std::vector<sockaddr_in> remote_endpoints_;  ///< Known remote endpoints
  std::array<uint8_t, DEFAULT_BUFFER_SIZE> receive_buffer_;  ///< Datagram buffer
};

} // namespace mav2grpc
