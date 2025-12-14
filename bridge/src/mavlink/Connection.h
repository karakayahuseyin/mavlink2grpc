/**
 * Copyright (C) 2025 Hüseyin Karakaya
 * This file is part of the mavlink2grpc project licensed under the MIT License.
 */

/**
 * @file Connection.h
 * @brief MAVLink protocol connection manager.
 */

#pragma once

#include "Transport.h"

#include <array>
#include <atomic>
#include <cstdint>
#include <functional>
#include <memory>
#include <mutex>
#include <thread>

// MAVLink C library
#include <mavlink/common/mavlink.h>

namespace mav2grpc {

/**
 * @brief Statistics for MAVLink connection monitoring.
 */
struct ConnectionStats {
  std::atomic<uint64_t> messages_received{0};  ///< Total messages received
  std::atomic<uint64_t> messages_sent{0};      ///< Total messages sent
  std::atomic<uint64_t> parse_errors{0};       ///< Parse/framing errors
  std::atomic<uint64_t> crc_errors{0};         ///< CRC validation failures
  std::atomic<uint64_t> sequence_gaps{0};      ///< Detected sequence number gaps
};

/**
 * @brief Manages MAVLink protocol communication over a transport layer.
 *
 * This class handles MAVLink message parsing, framing, routing, and
 * system/component ID management. Acts as the MAVLink protocol state machine.
 *
 * Features:
 * - MAVLink v1 and v2 auto-detection and support
 * - CRC validation and signing support
 * - Message sequence tracking
 * - Callback-based message delivery
 * - Connection statistics monitoring
 *
 * @note This class spawns a receive thread. Call stop() before destruction.
 * @note Not thread-safe for send operations - external synchronization required
 *       if calling send_message() from multiple threads.
 */
class Connection {
public:
  /**
   * @brief Message callback function type.
   *
   * Called when a complete MAVLink message is received and validated.
   *
   * @param msg The parsed MAVLink message
   */
  using MessageCallback = std::function<void(const mavlink_message_t& msg)>;

  /**
   * @brief Maximum MAVLink packet length (v2 extended).
   */
  static constexpr size_t MAX_PACKET_LEN = 280;

  /**
   * @brief Constructs a MAVLink connection.
   *
   * @param transport Transport layer (ownership transferred)
   * @param system_id This system's MAVLink system ID (1-255)
   * @param component_id This component's MAVLink component ID (1-255)
   *
   * @note Does not start I/O operations. Call start() to begin.
   */
  Connection(std::unique_ptr<Transport> transport,
             uint8_t system_id,
             uint8_t component_id);

  /**
   * @brief Destructor - ensures connection is stopped.
   */
  ~Connection();

  /**
   * @brief Starts the MAVLink connection and receive loop.
   *
   * Opens the underlying transport and spawns a background thread
   * to continuously receive and parse MAVLink messages.
   *
   * @return true if transport opened successfully, false otherwise
   *
   * @note This is not idempotent - calling start() on an already
   *       started connection will fail.
   */
  bool start();

  /**
   * @brief Stops the MAVLink connection.
   *
   * Gracefully stops the receive loop, joins the receive thread,
   * and closes the transport. Safe to call multiple times.
   */
  void stop();

  /**
   * @brief Checks if connection is currently active.
   *
   * @return true if started and transport is open, false otherwise
   */
  bool is_running() const { return running_.load(); }

  /**
   * @brief Sends a MAVLink message.
   *
   * Serializes the MAVLink message to binary format and transmits
   * via the transport layer. Automatically increments sequence number.
   *
   * @param msg MAVLink message to send (modified to set sequence)
   *
   * @return true if message sent successfully, false otherwise
   *
   * @note This method is thread-safe for concurrent calls.
   */
  bool send_message(mavlink_message_t& msg);

  /**
   * @brief Registers a callback for received messages.
   *
   * The callback is invoked from the receive thread context when
   * a complete, validated MAVLink message is received.
   *
   * @param callback Function to call on message reception
   *
   * @note Callback should execute quickly to avoid blocking receive loop.
   *       For slow processing, enqueue to a separate worker thread.
   */
  void set_message_callback(MessageCallback callback);

  /**
   * @brief Gets connection statistics.
   *
   * @return Const reference to statistics structure
   */
  const ConnectionStats& get_stats() const { return stats_; }

  /**
   * @brief Gets this connection's system ID.
   *
   * @return MAVLink system ID (1-255)
   */
  uint8_t get_system_id() const { return system_id_; }

  /**
   * @brief Gets this connection's component ID.
   *
   * @return MAVLink component ID (1-255)
   */
  uint8_t get_component_id() const { return component_id_; }

private:
  /**
   * @brief Receive loop running in background thread.
   *
   * Continuously reads from transport, parses bytes, and delivers
   * complete messages via callback.
   */
  void receive_loop();

  /**
   * @brief Processes a single byte through MAVLink parser.
   *
   * @param byte Byte to parse
   * @return true if complete message parsed, false otherwise
   */
  bool parse_byte(uint8_t byte);

  std::unique_ptr<Transport> transport_;     ///< Underlying transport layer
  uint8_t system_id_;                        ///< This system's ID
  uint8_t component_id_;                     ///< This component's ID
  uint8_t channel_;                          ///< MAVLink channel (0-15)
  std::atomic<uint8_t> sequence_;            ///< Outgoing message sequence

  MessageCallback message_callback_;         ///< Message delivery callback
  std::mutex callback_mutex_;                ///< Protects callback access

  std::atomic<bool> running_;                ///< Receive loop running flag
  std::thread receive_thread_;               ///< Background receive thread

  ConnectionStats stats_;                    ///< Connection statistics

  mavlink_message_t rx_message_;             ///< Receive message buffer
  mavlink_status_t* rx_status_;              ///< MAVLink parser state
  uint8_t last_sequence_{0};                 ///< Last received sequence number

  std::array<uint8_t, MAX_PACKET_LEN> tx_buffer_;  ///< Transmit buffer
  std::mutex tx_mutex_;                            ///< Protects send operations
};

} // namespace mav2grpc