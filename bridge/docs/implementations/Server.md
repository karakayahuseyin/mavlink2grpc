# Server

## Purpose
Main orchestrator class that initializes and coordinates all components. Manages application lifecycle, configuration loading, and graceful shutdown. Entry point for the bridge application.

## Design Principles
- Single responsibility: coordinate, not implement
- Dependency injection for all components
- RAII for resource management
- Graceful shutdown on signals
- Configuration-driven initialization

## Member Variables

### config
Configuration object or struct containing: transport type and parameters, gRPC port, MAVLink system/component IDs, logging level, etc. Loaded from file or command line.

### transport
std::unique_ptr<ITransport> owning the transport layer instance. Created based on config.

### mavlink_connection
std::unique_ptr<Connection> managing MAVLink protocol. Owns transport.

### message_converter
std::unique_ptr<MessageConverter> for MAVLink/proto conversion. Shared among components.

### grpc_converter
std::unique_ptr<gRPCConverter> for wrapping messages. Uses message_converter.

### router
std::unique_ptr<Router> for message routing. Central hub.

### grpc_service
std::unique_ptr<Service> implementing gRPC interface. Uses router and converter.

### grpc_server
std::unique_ptr<grpc::Server> from gRPC library. Hosts the service.

### signal_handler
Function or object for catching SIGINT/SIGTERM. Triggers graceful shutdown.

### running
std::atomic<bool> flag for main loop and shutdown coordination.

## Functions

### Constructor
Takes configuration (path or object). Stores config. Does not initialize components yet.

### initialize
Creates all components in correct order:
1. Transport (based on config type: serial/UDP/TCP)
2. MessageConverter (registers all message types)
3. Connection (injects transport)
4. gRPCConverter (wraps MessageConverter)
5. Router (creates queues)
6. Service (injects router and converter)
7. gRPC Server (registers service, binds port)
Validates configuration before creation.

### start
Starts all components in order:
1. Transport open
2. Connection start (begins receive loop)
3. Router start (begins routing loop)
4. gRPC server Start()
Registers signal handlers. Enters wait state or main loop.

### stop
Gracefully shuts down in reverse order:
1. gRPC server Shutdown()
2. Router stop (flush queues)
3. Connection stop (close transport)
Joins all threads. Logs final statistics.

### run
Combined initialize + start + wait. Main entry point. Blocks until shutdown signal.

### wait_for_shutdown
Blocks main thread. Waits on condition variable or signal. Returns when shutdown requested.

### load_configuration
Reads config from file (YAML/JSON) or command line args. Validates. Returns config object. Uses C++20 std::filesystem for path handling.

### setup_logging
Initializes logging framework (spdlog, etc.). Configures log level, format, output (console/file). Based on config.

## Configuration Parameters

### transport.type
Enum or string: "serial", "udp", "tcp".

### transport.serial.port
String: "/dev/ttyUSB0", baud_rate: 57600.

### transport.udp.local_port
uint16_t: 14550, broadcast: true.

### transport.tcp.mode
"client" or "server", host/port as appropriate.

### mavlink.system_id
uint8_t: 1 (default GCS).

### mavlink.component_id
uint8_t: 191 (MAV_COMP_ID_ONBOARD_COMPUTER).

### grpc.port
uint16_t: 50051 (default gRPC port).

### grpc.max_connections
uint32_t: limit concurrent streams.

### logging.level
String: "debug", "info", "warn", "error".

## Initialization Order
Critical: dependencies must be initialized before dependents. Transport before Connection. Converters before Service. All components before gRPC server start.

## Shutdown Order
Reverse of initialization. Stop accepting new requests first (gRPC). Then stop processing (Router). Finally close connections (Transport).

## Signal Handling
Register handlers for SIGINT (Ctrl+C) and SIGTERM (kill). Set running flag to false. Wake up wait condition. Allow graceful shutdown. Use std::signal or platform-specific APIs.

## Error Handling
Initialization failure: log error, cleanup partial initialization, exit with error code. Runtime error: attempt recovery or graceful degradation. Fatal error: log and exit cleanly.

## Threading Model
Main thread: initialization, configuration, signal handling. Worker threads: created by components (Connection receive, Router routing, gRPC thread pool). Coordination via running flag and condition variables.

## Logging and Monitoring
Log component lifecycle events: start, stop, errors. Periodic statistics logging: message counts, queue depths, active streams. Consider metrics export (Prometheus) for production.

## Resource Management
RAII: use smart pointers for all components. Ensure proper destruction order. Avoid manual delete. Check for resource leaks (valgrind, sanitizers).

## Testing Considerations
Design for testability: accept config via constructor. Allow mock injection for unit tests. Separate initialization from construction. Provide test configuration presets.
