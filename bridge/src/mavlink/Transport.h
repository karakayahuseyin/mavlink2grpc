/**
 * Copyright (C) 2025 Hüseyin Karakaya
 * This file is part of the mavlink2grpc project licensed under the MIT License.
 */

/**
 * @file Transport.h
 * @brief Abstract transport interface for various physical mediums.
 */

#pragma once

#include <unistd.h>
#include <cstddef>
#include <cstdint>

namespace mav2grpc {

/**
 * @brief Abstract interface for transport layer implementations.
 *
 * Provides a unified API for reading and writing bytes regardless of the
 * underlying physical medium (serial port, UDP socket, TCP connection).
 */
class Transport {
public:
  /**
   * @brief Virtual destructor for proper cleanup of derived classes.
   */
  virtual ~Transport() = default;

  /**
   * @brief Opens the connection to the physical transport medium.
   *
   * This method establishes the connection and prepares the transport
   * for read/write operations. It should be idempotent - safe to call
   * multiple times without adverse effects.
   *
   * @return true if connection opened successfully, false otherwise.
   *
   * @note Calling read() or write() before open() results in undefined behavior.
   */
  virtual bool open() = 0;

  /**
   * @brief Closes the connection and releases all resources.
   *
   * This method performs graceful shutdown of the transport. It must be
   * safe to call even if the connection is not currently open. After
   * calling close(), the transport can be reopened with open().
   *
   * @note Any pending data in internal buffers may be discarded.
   */
  virtual void close() = 0;

  /**
   * @brief Checks if the transport connection is currently active.
   *
   * @return true if connection is open and ready for I/O, false otherwise.
   */
  virtual bool is_open() const = 0;

  /**
   * @brief Performs a non-blocking read operation.
   *
   * Attempts to read up to buffer_size bytes from the transport into
   * the provided buffer. This is a non-blocking operation - it returns
   * immediately even if no data is available.
   *
   * @param buffer Pointer to buffer to read into.
   * @param buffer_size Maximum number of bytes to read.
   *
   * @return Number of bytes actually read (may be 0 if no data available),
   *         or -1 on error.
   *
   * @note Return value of 0 indicates no data available, not end-of-stream.
   * @note Buffer contents are undefined if return value is -1 (error).
   */
  virtual ssize_t read(uint8_t* buffer, size_t buffer_size) = 0;

  /**
   * @brief Writes data to the transport.
   *
   * Attempts to write all bytes from the provided buffer to the transport.
   * This operation may block if internal buffers are full, depending on
   * implementation.
   *
   * @param data Pointer to data to write.
   * @param data_size Number of bytes to write.
   *
   * @return Number of bytes actually written, or -1 on error.
   *
   * @note Partial writes are possible. Caller must check return value
   *       and retry if necessary.
   */
  virtual ssize_t write(const uint8_t* data, size_t data_size) = 0;

  // Non-copyable
  Transport(const Transport&) = delete;
  Transport& operator=(const Transport&) = delete;

  // Movable
  Transport(Transport&&) noexcept = default;
  Transport& operator=(Transport&&) noexcept = default;

protected:
	/**
	 * @brief Protected default constructor.
	 *
	 * Only derived classes can construct Transport objects.
	 */
	Transport() = default;
};

} // namespace mav2grpc