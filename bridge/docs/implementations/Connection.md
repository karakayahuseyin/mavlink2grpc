# Connection

## Purpose
Manages MAVLink protocol communication over a transport layer. Handles message parsing, framing, routing, and system/component ID management. Acts as the MAVLink protocol state machine.

## Design Principles
- Protocol-agnostic transport via ITransport dependency injection
- MAVLink v1 and v2 support with auto-detection
- System and component ID assignment for this node
- Message sequence number tracking
- CRC validation and signing support

## Member Variables

### transport
std::unique_ptr<ITransport> owning the underlying transport layer. Injected via constructor (dependency injection).

### system_id
uint8_t identifying this MAVLink system (1-255). Typically 1 for GCS, 255 for broadcast.

### component_id
uint8_t identifying this component (1-255). Use MAV_COMP_ID_ONBOARD_COMPUTER or similar.

### channel
mavlink_channel_t from MAVLink library. Manages per-connection parsing state.

### receive_buffer
std::array<uint8_t, MAVLINK_MAX_PACKET_LEN> for assembling incoming bytes into complete messages.

### buffer_position
size_t tracking current position in receive buffer.

### sequence_number
uint8_t auto-incrementing sequence for outgoing messages. Wraps at 255.

### message_callback
std::function or callback interface for delivering parsed messages to upper layers. Use std::function<void(const mavlink_message_t&)>.

### statistics
Struct tracking: messages received, messages sent, parse errors, CRC failures, sequence gaps. Useful for diagnostics.

### mavlink_version
Enum or uint8_t indicating v1 or v2. Auto-detected from first received message magic byte.

## Functions

### Constructor
Takes transport (unique_ptr), system_id, and component_id. Initializes MAVLink channel. Does not start I/O.

### start
Begins message receive loop. Either blocking in current thread or spawns worker thread. Calls transport->open().

### stop
Gracefully stops receive loop. Closes transport. Waits for worker thread if used.

### send_message
High-level function taking mavlink_message_t. Serializes to buffer using mavlink_msg_to_send_buffer(). Calls transport->write(). Increments sequence number.

### receive_loop
Internal function running continuous receive cycle. Reads bytes from transport. Feeds to mavlink_parse_char(). On complete message, invokes callback.

### register_message_callback
Sets the callback function for incoming messages. Use std::function for flexibility.

### get_statistics
Returns const reference to statistics struct. Thread-safe if using atomics.

### parse_byte
Helper function wrapping mavlink_parse_char(). Updates buffer and state machine. Returns true when message complete.

## Threading Considerations
- Receive loop typically runs in dedicated thread
- Send operations may be called from multiple threads: use mutex for sequence number and transport write
- Statistics should use std::atomic for lock-free updates
- Callback invoked in receive thread context: keep it fast or queue work

## MAVLink Protocol Details
- Message framing: 0xFE (v1) or 0xFD (v2) magic byte, length, sequence, sys/comp ID, msg ID, payload, CRC
- CRC-16/MCRF4XX checksum with CRC_EXTRA byte
- Sequence number wraps 0-255, used to detect packet loss
- Signing (v2): optional cryptographic signature for security

## Error Handling
- Parse errors: corrupt framing, invalid CRC, oversized payload
- Transport errors: disconnection, timeout, I/O failure
- Increment error counters, log details, continue operation
- Consider error callback for upper layer notification

## Performance Considerations
- Avoid memory allocation in receive loop
- Use zero-copy parsing where possible
- Batch statistics updates to reduce atomic contention
- Profile callback overhead: may need message queue for slow handlers
- Consider lock-free queue for send if multiple threads
