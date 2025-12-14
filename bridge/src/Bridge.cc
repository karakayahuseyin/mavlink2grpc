/**
 * Copyright (C) 2025 Hüseyin Karakaya
 * This file is part of the mavlink2grpc project licensed under the MIT License.
 */

#include "Bridge.h"
#include "mavlink/MessageConverter.h"
#include "mavlink/UdpTransport.h"
#include "mavlink/SerialTransport.h"
#include "service/Logger.h"
#include <format>
#include <regex>
#include <stdexcept>

namespace mav2grpc {

std::unique_ptr<Transport> Bridge::parse_connection_url(const std::string& url) {
  // URL format: protocol://[host]:port or protocol://device:baudrate
  // Examples:
  //   udp://:14550 - UDP server on port 14550
  //   udp://192.168.1.100:14550 - UDP client to 192.168.1.100:14550
  //   serial:///dev/ttyUSB0:57600 - Serial on /dev/ttyUSB0 at 57600 baud

  std::regex udp_server_regex(R"(udp://:(\d+))");
  std::regex udp_client_regex(R"(udp://([^:]+):(\d+))");
  std::regex serial_regex(R"(serial://([^:]+):(\d+))");
  
  std::smatch match;

  // UDP server (e.g., udp://:14550)
  if (std::regex_match(url, match, udp_server_regex)) {
    uint16_t port = std::stoi(match[1]);
    Logger::Info(std::format("Connecting to MAVLink via UDP server on port {}", port));
    return std::make_unique<UdpTransport>(port);
  }

  // UDP client (e.g., udp://192.168.1.100:14550)
  if (std::regex_match(url, match, udp_client_regex)) {
    std::string host = match[1];
    uint16_t port = std::stoi(match[2]);
    Logger::Info(std::format("Connecting to MAVLink via UDP client {}:{}", host, port));
    // UDP client support would need UdpTransport enhancement
    throw std::runtime_error("UDP client mode not yet implemented");
  }

  // Serial (e.g., serial:///dev/ttyUSB0:57600)
  if (std::regex_match(url, match, serial_regex)) {
    std::string device = match[1];
    uint32_t baudrate = std::stoi(match[2]);
    Logger::Info(std::format("Connecting to MAVLink via serial {} @ {} baud", device, baudrate));
    return std::make_unique<SerialTransport>(device, baudrate);
  }

  throw std::runtime_error(std::format("Invalid connection URL: {}", url));
}

Bridge::Bridge(const std::string& connection_url,
               const std::string& grpc_address,
               uint8_t system_id,
               uint8_t component_id)
    : Bridge(parse_connection_url(connection_url), grpc_address, system_id, component_id) {
}

Bridge::Bridge(std::unique_ptr<Transport> transport,
               const std::string& grpc_address,
               uint8_t system_id,
               uint8_t component_id)
    : system_id_(system_id), component_id_(component_id), running_(false) {
  
  if (!transport) {
    throw std::invalid_argument("Transport cannot be null");
  }

  // Create router for message routing
  router_ = std::make_shared<Router>();

  // Create MAVLink connection (takes ownership of transport)
  connection_ = std::make_shared<Connection>(
    std::move(transport), system_id, component_id
  );

  // Register callback for MAVLink messages
  connection_->set_message_callback(
    [this](const mavlink_message_t& msg) {
      on_mavlink_message(msg);
    }
  );

  // Create gRPC service with router and send callback
  service_ = std::make_shared<MavlinkBridgeServiceImpl>(
    *router_,
    [this](const mavlink::MavlinkMessage& proto_msg) -> bool {
      // Convert proto to MAVLink message
      auto mav_msg = MessageConverter::from_proto(
        proto_msg, system_id_, component_id_
      );
      if (!mav_msg) {
        Logger::Warn("Failed to convert proto message to MAVLink");
        return false;
      }
      
      // Send via connection
      return connection_->send_message(*mav_msg);
    }
  );

  // Create gRPC server
  server_ = std::make_unique<Server>(service_, grpc_address);

  Logger::Info("Bridge initialized");
}

Bridge::~Bridge() {
  if (running_) {
    stop();
  }
}

void Bridge::start() {
  if (running_) {
    Logger::Warn("Bridge already running");
    return;
  }

  Logger::Info("Starting bridge...");

  // Start gRPC server
  server_->start();

  // Start MAVLink connection
  connection_->start();

  running_ = true;
  Logger::Info("Bridge started successfully");
}

void Bridge::stop() {
  if (!running_) {
    Logger::Warn("Bridge not running");
    return;
  }

  Logger::Info("Stopping bridge...");

  // Stop MAVLink connection first
  connection_->stop();

  // Stop gRPC server
  server_->stop();

  running_ = false;
  Logger::Info("Bridge stopped");
}

void Bridge::wait() {
  if (!running_) {
    Logger::Warn("Bridge not running");
    return;
  }

  // Wait for gRPC server to finish
  server_->wait();
}

bool Bridge::is_running() const {
  return running_;
}

void Bridge::on_mavlink_message(const mavlink_message_t& msg) {
  // Convert MAVLink message to proto
  auto proto_msg = MessageConverter::to_proto(msg);
  if (!proto_msg) {
    // Silently ignore unsupported messages to avoid log spam
    // Only HEARTBEAT (msgid=0) is supported in minimal dialect
    return;
  }
  
  // Log received message
  Logger::Info(std::format(
    "MAVLink message received: msgid={} from sys={} comp={}",
    msg.msgid, msg.sysid, msg.compid
  ));
  
  // Route the proto message to all subscribed gRPC clients
  size_t delivered = router_->route_message(*proto_msg);
  
  if (delivered > 0) {
    Logger::Info(std::format("  → Routed to {} gRPC client(s)", delivered));
  }
}

} // namespace mav2grpc