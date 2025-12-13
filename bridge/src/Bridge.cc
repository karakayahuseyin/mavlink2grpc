/**
 * Copyright (C) 2025 Hüseyin Karakaya
 * This file is part of the mavlink2grpc project licensed under the MIT License.
 */

#include "Bridge.h"
#include "mavlink/MessageConverter.h"
#include "service/Logger.h"
#include <format>

namespace mav2grpc {

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
    Logger::Warn(std::format("Failed to convert MAVLink message (msgid={})", msg.msgid));
    return;
  }
  
  // Route the proto message to all subscribed gRPC clients
  router_->route_message(*proto_msg);
}

} // namespace mav2grpc