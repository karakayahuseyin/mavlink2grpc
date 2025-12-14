/**
 * Copyright (C) 2025 Hüseyin Karakaya
 * This file is part of the mavlink2grpc project licensed under the MIT License.
 */

#include "Bridge.h"
#include "service/Logger.h"
#include <csignal>
#include <iostream>
#include <memory>
#include <sstream>

using namespace mav2grpc;

// Global bridge instance for signal handling
std::unique_ptr<Bridge> g_bridge;

void signal_handler(int signal) {
  if (signal == SIGINT || signal == SIGTERM) {
    Logger::Info("Received shutdown signal");
    if (g_bridge) {
      g_bridge->stop();
    }
  }
}

void print_usage(const char* program_name) {
  std::cout << "Usage: " << program_name << " [options]\n"
            << "\nOptions:\n"
            << "  -c, --connection <url>    MAVLink connection URL (default: udp://:14550)\n"
            << "  -g, --grpc <address>      gRPC server address (default: 0.0.0.0:50051)\n"
            << "  -s, --system-id <id>      MAVLink system ID (default: 1)\n"
            << "  -C, --component-id <id>   MAVLink component ID (default: 1)\n"
            << "  -h, --help                Show this help\n"
            << "\nConnection URL formats:\n"
            << "  udp://:14550              UDP server on port 14550\n"
            << "  udp://192.168.1.100:14550 UDP client to remote host\n"
            << "  serial:///dev/ttyUSB0:57600 Serial connection\n"
            << "\nExamples:\n"
            << "  " << program_name << " -c udp://:14550\n"
            << "  " << program_name << " -c serial:///dev/ttyUSB0:57600 -g localhost:50051\n";
}

int main(int argc, char* argv[]) {
  // Default configuration
  std::string connection_url = "udp://:14550";
  std::string grpc_address = "0.0.0.0:50051";
  uint8_t system_id = 1;
  uint8_t component_id = 1;

  // Parse command line arguments
  for (int i = 1; i < argc; ++i) {
    std::string arg = argv[i];

    if (arg == "-h" || arg == "--help") {
      print_usage(argv[0]);
      return 0;
    }
    else if ((arg == "-c" || arg == "--connection") && i + 1 < argc) {
      connection_url = argv[++i];
    }
    else if ((arg == "-g" || arg == "--grpc") && i + 1 < argc) {
      grpc_address = argv[++i];
    }
    else if ((arg == "-s" || arg == "--system-id") && i + 1 < argc) {
      system_id = std::stoi(argv[++i]);
    }
    else if ((arg == "-C" || arg == "--component-id") && i + 1 < argc) {
      component_id = std::stoi(argv[++i]);
    }
    else {
      std::cerr << "Unknown option: " << arg << "\n";
      print_usage(argv[0]);
      return 1;
    }
  }

  // Setup signal handlers
  std::signal(SIGINT, signal_handler);
  std::signal(SIGTERM, signal_handler);

  Logger::Info("MAVLink to gRPC Bridge starting...");

  try {
    Logger::Info("Configuration:");
    {
      std::ostringstream oss;
      oss << "  Connection: " << connection_url;
      Logger::Info(oss.str());
    }
    {
      std::ostringstream oss;
      oss << "  gRPC address: " << grpc_address;
      Logger::Info(oss.str());
    }
    {
      std::ostringstream oss;
      oss << "  System ID: " << static_cast<int>(system_id);
      Logger::Info(oss.str());
    }
    {
      std::ostringstream oss;
      oss << "  Component ID: " << static_cast<int>(component_id);
      Logger::Info(oss.str());
    }

    // Create and start bridge
    g_bridge = std::make_unique<Bridge>(
      connection_url,
      grpc_address,
      system_id,
      component_id
    );

    g_bridge->start();

    Logger::Info("Bridge running. Press Ctrl+C to stop.");

    // Wait for bridge to finish
    g_bridge->wait();

    Logger::Info("Bridge shutdown complete");
    return 0;

  } catch (const std::exception& e) {
    std::ostringstream oss;
    oss << "Fatal error: " << e.what();
    Logger::Error(oss.str());
    return 1;
  }
}