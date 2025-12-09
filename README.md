# mavlink2grpc
A high-performance, native bridge and code generator that exposes the entire MAVLink protocol as type-safe gRPC services using modern C++20.

Designed as a robust, type-safe alternative to [mavlink2rest](https://github.com/mavlink/mavlink2rest) for professional UAV systems.

## Features
* **Schema-First:** Auto-generates Protobuf definitions (`.proto`) directly from MAVLink XMLs.
* **High Performance:** C++20 runtime bridge designed for zero-copy deserialization and low latency.
* **Polyglot:** Consume MAVLink telemetry in Python, Go, Rust, or Web clients via gRPC.
* **Streaming:** Real-time Pub/Sub support for high-frequency sensor data.