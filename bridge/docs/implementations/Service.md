# Service

## Purpose
Implements the gRPC MavlinkBridge service interface. Handles StreamMessages (server streaming) and SendMessage (unary) RPC methods. Integrates with Router for message delivery.

## Design Principles
- Implements generated service base class
- Manages active stream lifecycle
- Integrates filter parsing and validation
- Handles client disconnection gracefully
- Thread-safe stream management

## Member Variables

### router
Pointer or reference to Router instance. Used to register/unregister streams and send messages.

### converter
Pointer or reference to gRPCConverter for message wrapping/unwrapping.

### active_streams
std::map<StreamId, StreamContext> tracking all active StreamMessages calls. Thread-safe with mutex.

### stream_mutex
std::mutex protecting active_streams map.

### next_stream_id
std::atomic<uint64_t> for generating unique stream identifiers.

### server_statistics
Struct tracking: total streams created, active streams, messages sent, errors. For monitoring.

## Functions

### StreamMessages
gRPC server streaming RPC implementation. Takes StreamFilter from client. Validates filter. Generates stream ID. Registers with router including filter and delivery callback. Blocks until client disconnects or error. Unregisters on exit.

### SendMessage
gRPC unary RPC implementation. Takes MavlinkMessage from client. Unwraps using gRPCConverter. Validates. Routes to MAVLink via router. Returns SendResponse with success/failure.

### create_stream_context
Helper to build StreamContext object containing: stream ID, filter criteria, writer object, creation timestamp.

### validate_filter
Checks StreamFilter for valid message IDs, system IDs, rate limits. Returns validation errors if any.

### deliver_to_stream
Callback function given to router. Called when message matches stream filter. Writes message to gRPC stream. Handles write failures (client disconnect).

### cleanup_stream
Removes stream from active_streams and unregisters from router. Called on disconnect or error.

### handle_write_error
Strategy when stream write fails: log, update stats, trigger cleanup. Distinguish transient vs permanent errors.

## StreamFilter Processing

### message_ids
Repeated uint32 field. Empty means all messages. Non-empty means whitelist.

### system_ids
Repeated uint32 field. Empty means all systems. Non-empty means whitelist.

### component_ids
Repeated uint32 field. Empty means all components. Non-empty means whitelist.

### rate_limit
Optional double field. Maximum messages per second for this stream. Requires rate limiting logic.

## Stream Lifecycle
1. Client calls StreamMessages with filter
2. Service validates filter and creates stream context
3. Service registers with router
4. Router delivers matching messages via callback
5. Service writes to gRPC stream
6. On client disconnect or error: cleanup and unregister
7. Service returns from RPC call

## Threading Considerations
- gRPC server uses thread pool: multiple concurrent RPC calls
- StreamMessages blocks calling thread until disconnect
- deliver_to_stream called from router thread: needs thread-safe write
- active_streams accessed from multiple threads: requires mutex
- Consider one thread per stream vs shared thread pool

## Error Handling
- Invalid filter: return gRPC InvalidArgument status
- Stream write failure: detect client disconnect, cleanup
- Router errors: convert to gRPC status codes
- Message conversion errors: skip message, log, continue stream

## Backpressure
- gRPC provides flow control: Write() may block if client slow
- Consider timeout on Write() to detect stuck clients
- Router may drop messages if client too slow: notify client via error message or metadata

## Performance Considerations
- Minimize work in deliver_to_stream (router thread context)
- Consider queue per stream to decouple router from write blocking
- Profile Write() latency: slow clients affect router performance
- Monitor active stream count and resource usage
- Use gRPC compression for bandwidth efficiency

## Client Disconnection Detection
- Write() returns false on disconnect
- Context cancellation: check ServerContext::IsCancelled()
- Implement keepalive or deadline for detecting silent failures
- Cleanup resources promptly to avoid leaks
