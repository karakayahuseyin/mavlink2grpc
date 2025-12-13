/**
 * Copyright (C) 2025 Hüseyin Karakaya
 * This file is part of the mavlink2grpc project licensed under the MIT License.
 */

#include "Logger.h"

#include <chrono>
#include <iostream>

namespace mav2grpc {

Logger& Logger::Instance() {
  static Logger instance;
  return instance;
}

Logger::Logger() {
  // Start background logging thread
  log_thread_ = std::thread([this]() {
    while (true) {
      std::string message;
      
      {
        std::unique_lock<std::mutex> lock(queue_mutex_);

        // Wait for log message or stop signal
        log_condition_.wait(lock, [this] {
          return stop_logging_.load() || !log_queue_.empty();
        });

        // If stopping and no messages left, exit
        if (stop_logging_.load() && log_queue_.empty()) {
          break;
        }

        // Get message from queue
        message = std::move(log_queue_.front());
        log_queue_.pop();
      }

      // Output to console (outside lock)
      std::cout << message << std::endl;
    }
  });
}

Logger::~Logger() {
  // Signal thread to stop
  {
    std::lock_guard<std::mutex> lock(queue_mutex_);
    stop_logging_.store(true);
  }
  log_condition_.notify_all();

  // Wait for thread to finish
  if (log_thread_.joinable()) {
    log_thread_.join();
  }
}

void Logger::Log(Level level, const std::string& message) {
  // Format: [Timestamp] [Level] Message
  auto now = std::chrono::system_clock::now();
  auto now_ms = std::chrono::floor<std::chrono::milliseconds>(now);
  std::string timestamp = std::format("{:%Y-%m-%d %H:%M:%S}", now_ms);

  // Color codes for terminal output
  const char* color_reset = "\033[0m";
  const char* color_code = "";
  std::string level_str;

  switch (level) {
    case Level::INFO:
      color_code = "\033[32m";  // Green
      level_str = "INFO";
      break;
    case Level::WARNING:
      color_code = "\033[33m";  // Yellow
      level_str = "WARN";
      break;
    case Level::ERROR:
      color_code = "\033[31m";  // Red
      level_str = "ERR ";
      break;
  }

  std::string log_entry = std::format(
    "{}[{}] [{}]{} {}",
    color_code,
    timestamp,
    level_str,
    color_reset,
    message
  );

  // Add to queue
  {
    std::lock_guard<std::mutex> lock(queue_mutex_);
    log_queue_.push(std::move(log_entry));
  }
  log_condition_.notify_one();
}

} // namespace mav2grpc