/**
 * Copyright (C) 2025 Hüseyin Karakaya
 * This file is part of the mavlink2grpc project licensed under the MIT License.
 */

#include "SerialTransport.h"
#include <fcntl.h>
#include <unistd.h>
#include <cstring>
#include <system_error>

namespace mav2grpc {

SerialTransport::SerialTransport(std::string device, uint32_t baudrate)
    : device_(std::move(device)),
      baudrate_(baudrate),
      fd_(-1) {
}

SerialTransport::~SerialTransport() {
  close();
}

bool SerialTransport::open() {
  if (is_open()) {
    return true;
  }

  // Open serial port in non-blocking mode
  fd_ = ::open(device_.c_str(), O_RDWR | O_NOCTTY | O_NONBLOCK);
  if (fd_ < 0) {
    return false;
  }

  // Save current settings to restore later
  if (tcgetattr(fd_, &old_tio_) != 0) {
    ::close(fd_);
    fd_ = -1;
    return false;
  }

  // Configure port
  if (!configure_port()) {
    ::close(fd_);
    fd_ = -1;
    return false;
  }

  return true;
}

void SerialTransport::close() {
  if (!is_open()) {
    return;
  }

  // Restore original settings
  tcsetattr(fd_, TCSANOW, &old_tio_);

  ::close(fd_);
  fd_ = -1;
}

bool SerialTransport::is_open() const {
  return fd_ >= 0;
}

ssize_t SerialTransport::read(uint8_t* buffer, size_t buffer_size) {
  if (!is_open()) {
    return -1;
  }

  ssize_t n = ::read(fd_, buffer, buffer_size);
  
  // Non-blocking read returns -1 with EAGAIN/EWOULDBLOCK if no data
  if (n < 0 && (errno == EAGAIN || errno == EWOULDBLOCK)) {
    return 0;
  }

  return n;
}

ssize_t SerialTransport::write(const uint8_t* data, size_t data_size) {
  if (!is_open()) {
    return -1;
  }

  return ::write(fd_, data, data_size);
}

bool SerialTransport::configure_port() {
  struct termios tio;
  std::memset(&tio, 0, sizeof(tio));

  // Get baud rate
  speed_t speed = baudrate_to_speed(baudrate_);
  if (speed == B0) {
    return false; // Invalid baud rate
  }

  // Set input/output baud rate
  cfsetispeed(&tio, speed);
  cfsetospeed(&tio, speed);

  // Control modes:
  // - CS8: 8 data bits
  // - CLOCAL: Ignore modem control lines
  // - CREAD: Enable receiver
  tio.c_cflag = CS8 | CLOCAL | CREAD;

  // Input modes:
  // - Disable all input processing (raw mode)
  tio.c_iflag = 0;

  // Output modes:
  // - Disable all output processing (raw mode)
  tio.c_oflag = 0;

  // Local modes:
  // - Disable canonical mode (line buffering)
  // - Disable echo
  // - Disable signal generation
  tio.c_lflag = 0;

  // Control characters:
  // - VMIN: Minimum characters to read (0 = non-blocking)
  // - VTIME: Timeout in deciseconds (0 = no timeout)
  tio.c_cc[VMIN] = 0;
  tio.c_cc[VTIME] = 0;

  // Apply settings immediately
  if (tcsetattr(fd_, TCSANOW, &tio) != 0) {
    return false;
  }

  // Flush any pending data
  tcflush(fd_, TCIOFLUSH);

  return true;
}

speed_t SerialTransport::baudrate_to_speed(uint32_t baudrate) {
  switch (baudrate) {
    case 9600:    return B9600;
    case 19200:   return B19200;
    case 38400:   return B38400;
    case 57600:   return B57600;
    case 115200:  return B115200;
    case 230400:  return B230400;
    case 460800:  return B460800;
    case 500000:  return B500000;
    case 576000:  return B576000;
    case 921600:  return B921600;
    case 1000000: return B1000000;
    case 1152000: return B1152000;
    case 1500000: return B1500000;
    case 2000000: return B2000000;
    case 2500000: return B2500000;
    case 3000000: return B3000000;
    case 3500000: return B3500000;
    case 4000000: return B4000000;
    default:      return B0; // Invalid
  }
}

} // namespace mav2grpc