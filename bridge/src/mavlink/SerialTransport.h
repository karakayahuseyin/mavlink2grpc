/**
 * Copyright (C) 2025 Hüseyin Karakaya
 * This file is part of the mavlink2grpc project licensed under the MIT License.
 */

/**
 * @file SerialTransport.h
 * @brief Serial port transport implementation.
 */

#pragma once

#include "Transport.h"
#include <array>
#include <cstdint>
#include <span>
#include <string>
#include <termios.h>

namespace mav2grpc {

/**
 * @brief Serial port transport implementation.
 *
 * Implements serial port communication for MAVLink traffic.
 * Commonly used for direct connection to autopilot hardware
 * via UART/USB serial interfaces.
 *
 * @note Serial connections are reliable at the hardware level
 *       but may experience data loss if buffers overflow.
 *
 * @note Not thread-safe by default. External synchronization
 *       required if accessed from multiple threads.
 */
class SerialTransport : public Transport {
public:
  /**
   * @brief Default receive buffer size (512 bytes).
   *
   * Smaller than UDP since serial is typically slower.
   */
  static constexpr size_t DEFAULT_BUFFER_SIZE = 512;

  /**
   * @brief Constructs a serial transport.
   *
   * @param device Serial device path (e.g., "/dev/ttyUSB0", "/dev/ttyACM0")
   * @param baudrate Baud rate (e.g., 57600, 115200, 921600)
   */
  explicit SerialTransport(std::string device, uint32_t baudrate);

  /**
   * @brief Destructor - ensures port is closed.
   */
  ~SerialTransport() override;

  /**
   * @brief Opens the serial port.
   *
   * Configures port with 8N1 settings (8 data bits, no parity, 1 stop bit),
   * no flow control, and raw mode for binary data.
   *
   * @return true if port opened successfully, false otherwise.
   */
  bool open() override;

  /**
   * @brief Closes the serial port.
   *
   * Safe to call multiple times.
   */
  void close() override;

  /**
   * @brief Checks if the serial port is currently open.
   *
   * @return true if port is open and ready for I/O, false otherwise.
   */
  bool is_open() const override;

  /**
   * @brief Reads data from the serial port (non-blocking).
   *
   * @param buffer Destination buffer for received data
   * @return Number of bytes read (0 if no data available, -1 on error)
   */
  ssize_t read(std::span<std::byte> buffer) override;

  /**
   * @brief Writes data to the serial port.
   *
   * @param data Data to transmit
   * @return Number of bytes written (-1 on error)
   */
  ssize_t write(std::span<const std::byte> data) override;

private:
  std::string device_;        ///< Serial device path
  uint32_t baudrate_;         ///< Baud rate
  int fd_;                    ///< File descriptor for serial port
  struct termios old_tio_;    ///< Original terminal settings (for restoration)

  /**
   * @brief Configure serial port settings.
   *
   * @return true if configuration successful, false otherwise.
   */
  bool configure_port();

  /**
   * @brief Convert baudrate to termios speed constant.
   *
   * @param baudrate Numeric baud rate
   * @return Termios speed constant (e.g., B57600)
   */
  static speed_t baudrate_to_speed(uint32_t baudrate);
};

} // namespace mav2grpc