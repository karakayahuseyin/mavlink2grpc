/**
 * Copyright (C) 2025 Hüseyin Karakaya
 * This file is part of the mavlink2grpc project licensed under the MIT License.
 */

#include "Service.h"
#include "Logger.h"
#include <sstream>
#include <chrono>
#include <thread>
#include <condition_variable>
#include <mutex>

namespace mav2grpc {

MavlinkBridgeServiceImpl::MavlinkBridgeServiceImpl(
    Router& router,
    SendMessageCallback send_callback)
  : router_(router),
    send_callback_(std::move(send_callback)) {
}

grpc::Status MavlinkBridgeServiceImpl::StreamMessages(
    grpc::ServerContext* context,
    const mavlink::StreamFilter* request,
    grpc::ServerWriter<mavlink::MavlinkMessage>* writer) {
  
  {
    std::ostringstream oss;
    oss << "Client connected - StreamMessages (sys:" << request->system_id()
        << ", comp:" << request->component_id()
        << ", msgs:" << request->message_ids_size() << ")";
    Logger::Info(oss.str());
  }

  // Subscribe to router with filter
  uint64_t sub_id = router_.subscribe(
    *request,
    [writer](const mavlink::MavlinkMessage& msg) -> bool {
      // Write to gRPC stream
      return writer->Write(msg);
    }
  );

  // Wait efficiently for client cancellation using condition variable
  std::mutex mtx;
  std::condition_variable cv;
  std::unique_lock<std::mutex> lock(mtx);
  
  // Wait until context is cancelled (no CPU busy-wait)
  cv.wait(lock, [context]() { return context->IsCancelled(); });

  // Cleanup subscription
  router_.unsubscribe(sub_id);

  {
    std::ostringstream oss;
    oss << "Client disconnected - StreamMessages (ID: " << sub_id << ")";
    Logger::Info(oss.str());
  }

  return grpc::Status::OK;
}

grpc::Status MavlinkBridgeServiceImpl::SendMessage(
    grpc::ServerContext* /* context */,
    const mavlink::MavlinkMessage* request,
    mavlink::SendResponse* response) {
  
  // Validate request has payload
  if (request->payload_case() == mavlink::MavlinkMessage::PAYLOAD_NOT_SET) {
    response->set_success(false);
    response->set_error("No payload in message");
    Logger::Warn("SendMessage RPC failed: No payload");
    return grpc::Status(
      grpc::StatusCode::INVALID_ARGUMENT,
      "Message has no payload"
    );
  }

  // Send via callback
  bool success = send_callback_(*request);

  if (success) {
    response->set_success(true);
    std::ostringstream oss;
    oss << "Sent message (ID: " << request->message_id()
        << ", sys: " << request->system_id()
        << ", comp: " << request->component_id() << ")";
    Logger::Info(oss.str());
    return grpc::Status::OK;
  } else {
    response->set_success(false);
    response->set_error("Failed to send via MAVLink connection");
    std::ostringstream oss;
    oss << "Failed to send message (ID: " << request->message_id() << ")";
    Logger::Error(oss.str());
    return grpc::Status(
      grpc::StatusCode::INTERNAL,
      "MAVLink send failed"
    );
  }
}

} // namespace mav2grpc