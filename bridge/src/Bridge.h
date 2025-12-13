/**
 * Copyright (C) 2025 Hüseyin Karakaya
 * This file is part of the mavlink2grpc project licensed under the MIT License.
 */

#pragma once

#include "mavlink/Connection.h"
#include "service/Router.h"
#include "service/Service.h"
#include "service/Server.h"
#include <memory>
#include <string>

namespace mav2grpc {

/**
 * @brief Main bridge coordinator.
 *
 * Orchestrates all components:
 * - MAVLink Connection (receive MAVLink messages)
 * - Router (route messages to gRPC streams)
 * - Service (gRPC service implementation)
 * - Server (gRPC server lifecycle)
 *
 * Usage:
 * @code
 * auto transport = UdpTransport::create_server(14550);
 * Bridge bridge(transport);
 * bridge.start();
 * bridge.wait(); // blocks until shutdown
 * @endcode
 */
class Bridge {
public:
  /**
   * @brief Construct bridge with transport.
   *
   * @param transport MAVLink transport layer (ownership transferred)
   * @param grpc_address gRPC server address (default: 0.0.0.0:50051)
   * @param system_id MAVLink system ID (default: 1)
   * @param component_id MAVLink component ID (default: 1)
   */
  explicit Bridge(std::unique_ptr<Transport> transport,
                  const std::string& grpc_address = "0.0.0.0:50051",
                  uint8_t system_id = 1,
                  uint8_t component_id = 1);

  ~Bridge();

  // Non-copyable, non-movable
  Bridge(const Bridge&) = delete;
  Bridge& operator=(const Bridge&) = delete;
  Bridge(Bridge&&) = delete;
  Bridge& operator=(Bridge&&) = delete;

  /**
   * @brief Start the bridge (MAVLink connection + gRPC server).
   */
  void start();

  /**
   * @brief Stop the bridge gracefully.
   */
  void stop();

  /**
   * @brief Wait for bridge to finish (blocks until shutdown).
   */
  void wait();

  /**
   * @brief Check if bridge is running.
   */
  bool is_running() const;

private:
  /**
   * @brief MAVLink message callback - routes to gRPC clients.
   */
  void on_mavlink_message(const mavlink_message_t& msg);

  std::shared_ptr<Connection> connection_;
  std::shared_ptr<Router> router_;
  std::shared_ptr<MavlinkBridgeServiceImpl> service_;
  std::unique_ptr<Server> server_;
  uint8_t system_id_;
  uint8_t component_id_;
  bool running_;
};

} // namespace mav2grpc