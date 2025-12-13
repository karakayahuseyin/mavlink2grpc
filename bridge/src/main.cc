/**
 * Copyright (C) 2025 Hüseyin Karakaya
 * This file is part of the mavlink2grpc project licensed under the MIT License.
 */

#include "Bridge.h"
#include "mavlink/UdpTransport.h"
#include "service/Logger.h"
#include <csignal>
#include <memory>

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

int main(int argc, char* argv[]) {
  // Setup signal handlers
  std::signal(SIGINT, signal_handler);
  std::signal(SIGTERM, signal_handler);

  Logger::Info("MAVLink to gRPC Bridge starting...");

  try {
    // Parse command line arguments
    uint16_t mavlink_port = 14550;
    std::string grpc_address = "0.0.0.0:50051";
    uint8_t system_id = 1;
    uint8_t component_id = 1;

    if (argc > 1) {
      mavlink_port = std::stoi(argv[1]);
    }
    if (argc > 2) {
      grpc_address = argv[2];
    }
    if (argc > 3) {
      system_id = std::stoi(argv[3]);
    }
    if (argc > 4) {
      component_id = std::stoi(argv[4]);
    }

    Logger::Info(std::format("Configuration:"));
    Logger::Info(std::format("  MAVLink UDP port: {}", mavlink_port));
    Logger::Info(std::format("  gRPC address: {}", grpc_address));
    Logger::Info(std::format("  System ID: {}", system_id));
    Logger::Info(std::format("  Component ID: {}", component_id));

    // Create UDP transport
    auto transport = std::make_unique<UdpTransport>(mavlink_port);

    // Create and start bridge
    g_bridge = std::make_unique<Bridge>(
      std::move(transport),
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
    Logger::Error(std::format("Fatal error: {}", e.what()));
    return 1;
  }
}