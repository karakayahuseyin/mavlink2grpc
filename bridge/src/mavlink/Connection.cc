/**
 * Copyright (C) 2025 Hüseyin Karakaya
 * This file is part of the mavlink2grpc project licensed under the MIT License.
 */

#include "Connection.h"

#include <cstring>
#include <iostream>

namespace mav2grpc {

Connection::Connection(std::unique_ptr<Transport> transport,
                       uint8_t system_id,
                       uint8_t component_id)
  : transport_(std::move(transport)),
    system_id_(system_id),
    component_id_(component_id),
    channel_(MAVLINK_COMM_0),  // Use first available channel
    sequence_(0),
    running_(false),
    rx_status_(mavlink_get_channel_status(channel_)) {

  // Initialize receive message buffer
  std::memset(&rx_message_, 0, sizeof(rx_message_));
}

Connection::~Connection() {
  stop();
}

bool Connection::start() {
  if (running_.load()) {
    std::cerr << "Connection already started\n";
    return false;
  }

  // Open transport
  if (!transport_->open()) {
    std::cerr << "Failed to open transport\n";
    return false;
  }

  // Start receive thread
  running_.store(true);
  receive_thread_ = std::thread(&Connection::receive_loop, this);

  return true;
}

void Connection::stop() {
  if (!running_.load()) {
    return;  // Already stopped
  }

  // Signal thread to stop
  running_.store(false);

  // Wait for receive thread to finish
  if (receive_thread_.joinable()) {
    receive_thread_.join();
  }

  // Close transport
  transport_->close();
}

bool Connection::send_message(mavlink_message_t& msg) {
  if (!transport_->is_open()) {
    std::cerr << "Transport not open for sending\n";
    return false;
  }

  std::lock_guard<std::mutex> lock(tx_mutex_);

  // Update sequence number
  msg.seq = sequence_.fetch_add(1, std::memory_order_relaxed);

  // Serialize message to buffer
  uint16_t len = mavlink_msg_to_send_buffer(tx_buffer_.data(), &msg);

  // Send via transport
  auto buffer_span = std::span<const std::byte>(
    reinterpret_cast<const std::byte*>(tx_buffer_.data()), len);

  ssize_t sent = transport_->write(buffer_span);

  if (sent < 0 || static_cast<size_t>(sent) != len) {
    std::cerr << "Failed to send MAVLink message\n";
    return false;
  }

  stats_.messages_sent.fetch_add(1, std::memory_order_relaxed);
  return true;
}

void Connection::set_message_callback(MessageCallback callback) {
  std::lock_guard<std::mutex> lock(callback_mutex_);
  message_callback_ = std::move(callback);
}

void Connection::receive_loop() {
  std::array<std::byte, MAX_PACKET_LEN> read_buffer;

  while (running_.load()) {
    // Read bytes from transport
    ssize_t bytes_read = transport_->read(read_buffer);

    if (bytes_read < 0) {
      std::cerr << "Transport read error, stopping connection\n";
      running_.store(false);
      break;
    }

    if (bytes_read == 0) {
      // No data available - non-blocking read, just continue
      continue;
    }

    // Parse each byte
    for (ssize_t i = 0; i < bytes_read; ++i) {
      uint8_t byte = static_cast<uint8_t>(read_buffer[i]);

      if (parse_byte(byte)) {
        // Complete message received
        stats_.messages_received.fetch_add(1, std::memory_order_relaxed);

        // Check for sequence gaps
        uint8_t expected_seq = static_cast<uint8_t>(last_sequence_ + 1);
        if (rx_message_.seq != expected_seq && last_sequence_ != 0) {
          stats_.sequence_gaps.fetch_add(1, std::memory_order_relaxed);
        }
        last_sequence_ = rx_message_.seq;

        // Copy callback to avoid holding lock during execution
        MessageCallback callback_copy;
        {
          std::lock_guard<std::mutex> lock(callback_mutex_);
          callback_copy = message_callback_;
        }

        // Execute callback without holding lock
        if (callback_copy) {
          callback_copy(rx_message_);
        }
      }
    }
  }
}

bool Connection::parse_byte(uint8_t byte) {
  // Parse byte through MAVLink state machine
  uint8_t msg_received = mavlink_parse_char(channel_, byte, &rx_message_, rx_status_);

  if (msg_received == MAVLINK_FRAMING_OK) {
    // Complete message parsed successfully
    return true;
  }

  if (msg_received == MAVLINK_FRAMING_BAD_CRC) {
    stats_.crc_errors.fetch_add(1, std::memory_order_relaxed);
  } else if (msg_received == MAVLINK_FRAMING_INCOMPLETE) {
    // Still parsing, normal
  } else {
    // Other parse error
    stats_.parse_errors.fetch_add(1, std::memory_order_relaxed);
  }

  return false;
}

} // namespace mav2grpc
