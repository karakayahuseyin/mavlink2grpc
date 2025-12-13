# Router

## Purpose
Routes messages bidirectionally between MAVLink connection and gRPC service. Manages message queues, filtering, and fan-out to multiple gRPC clients. Acts as the central message hub.

## Design Principles
- Thread-safe message queuing
- Multiple subscriber support (fan-out)
- Message filtering based on client requirements
- Backpressure handling
- Lock-free or fine-grained locking for performance

## Member Variables

### mavlink_to_grpc_queue
Thread-safe queue for messages flowing from MAVLink to gRPC clients. Use std::queue with std::mutex or lock-free queue (boost::lockfree::spsc_queue).

### grpc_to_mavlink_queue
Thread-safe queue for messages from gRPC clients to MAVLink. Typically lower volume.

### subscribers
std::vector or std::map of active gRPC stream subscribers. Each has filter criteria and callback/queue.

### subscriber_mutex
std::shared_mutex for reader-writer lock on subscriber list. Many readers (message routing), few writers (client connect/disconnect).

### running
std::atomic<bool> flag for graceful shutdown.

### routing_thread
std::jthread (C++20) for background message routing. Processes queues and delivers to subscribers.

### queue_size_limit
size_t for maximum queue depth. Prevents unbounded memory growth under load.

### dropped_message_count
std::atomic<uint64_t> tracking messages dropped due to full queues or slow consumers.

## Functions

### Constructor
Initializes empty queues and subscriber list. Does not start routing thread.

### start
Spawns routing thread. Begins processing messages. Connects to MAVLink connection callback.

### stop
Sets running flag to false. Joins routing thread. Flushes queues.

### route_to_grpc
Called by MAVLink connection callback. Enqueues message to mavlink_to_grpc_queue. Applies backpressure if queue full.

### route_to_mavlink
Called by gRPC service when client sends message. Enqueues to grpc_to_mavlink_queue.

### add_subscriber
Registers new gRPC client stream. Takes filter criteria (message IDs, system IDs) and delivery callback. Thread-safe.

### remove_subscriber
Unregisters client on stream close. Thread-safe with subscriber_mutex.

### routing_loop
Main loop running in routing thread. Dequeues messages. Matches against subscriber filters. Calls delivery callbacks. Handles errors.

### apply_filter
Given message and filter criteria, returns bool whether message should be delivered. Checks message ID, system ID, component ID.

### handle_backpressure
Strategy when queue full: drop oldest, drop newest, block, or notify. Configurable policy.

## Message Filtering

### by_message_id
Only deliver specific message types (e.g., only HEARTBEAT and GPS_RAW_INT).

### by_system_id
Filter by source system (e.g., only messages from vehicle 1).

### by_component_id
Filter by source component (e.g., only autopilot messages, not camera).

### by_rate
Decimate high-frequency messages (e.g., ATTITUDE at 1 Hz instead of 50 Hz).

## Threading Model
- One routing thread for all message distribution
- MAVLink connection runs in separate thread
- gRPC service has thread pool from gRPC server
- Lock contention points: queue operations, subscriber list access
- Consider per-subscriber queues for isolation

## Performance Considerations
- Lock-free queues for hot path if possible
- Shared mutex for subscriber list (many readers)
- Avoid holding locks during message delivery callbacks
- Batch processing: dequeue multiple messages at once
- Consider priority queues for critical messages
- Profile lock contention and queue depth

## Error Handling
- Full queue: apply backpressure policy
- Slow subscriber: timeout and disconnect, or skip frames
- Invalid message: log and drop
- Subscriber callback exception: catch, log, continue

## Monitoring
Track metrics: queue depth, messages routed, messages dropped, subscriber count, routing latency. Expose via logging or metrics endpoint.
