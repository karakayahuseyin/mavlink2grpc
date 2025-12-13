# gRPCConverter

## Purpose
Wraps and unwraps MAVLink proto messages in the gRPC service message envelope. Handles the MavlinkMessage wrapper that contains metadata (timestamp, system_id, etc.) and the specific message as oneof.

## Design Principles
- Lightweight wrapper around MessageConverter
- Adds gRPC-specific metadata (server timestamp, reception time)
- Manages proto oneof for different message types
- Stateless operation

## Member Variables

### message_converter
Reference or pointer to MessageConverter instance. Used for actual MAVLink <-> proto conversion.

### use_server_timestamp
bool flag to add server-side timestamp to messages. Useful for latency measurement.

## Functions

### wrap_for_grpc
Takes converted proto message (e.g., Heartbeat) and wraps in MavlinkMessage envelope. Sets system_id, component_id, message_id fields. Adds server timestamp if enabled. Sets appropriate oneof field. Returns MavlinkMessage ready for gRPC streaming.

### unwrap_from_grpc
Takes MavlinkMessage from gRPC client. Extracts oneof field. Validates presence. Returns inner message for conversion to MAVLink.

### add_metadata
Helper to populate common metadata fields: reception timestamp, sequence number, origin info.

### validate_message
Checks MavlinkMessage has required fields populated. Ensures oneof is set. Returns validation result.

## Metadata Fields

### timestamp
Server-side timestamp when message received. Use std::chrono::system_clock. Convert to proto Timestamp.

### system_id
Extracted from MAVLink header. Identifies source system.

### component_id
Extracted from MAVLink header. Identifies source component.

### message_id
MAVLink message type ID. Redundant with oneof but useful for filtering.

### sequence
Original MAVLink sequence number. Useful for detecting loss.

## gRPC Message Structure
MavlinkMessage contains:
- Common metadata fields
- oneof payload containing specific message (heartbeat, attitude, gps_raw_int, etc.)
- Extensions for future metadata

## Error Handling
- Missing oneof: return error status
- Invalid timestamp: use current time as fallback
- Unknown message type: reject with error

## Performance Considerations
- Wrapping is lightweight: just field assignment
- Timestamp overhead: microseconds per message
- Consider batch wrapping for multiple messages
- Move semantics to avoid copies of large messages

## Threading
- Stateless: safe to call from multiple threads
- MessageConverter reference must be thread-safe
- No internal synchronization needed
