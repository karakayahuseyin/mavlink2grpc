# MessageConverter

## Purpose
Converts between MAVLink C structs (mavlink_message_t) and generated Protobuf messages. Handles type mapping, field extraction, and message ID routing.

## Design Principles
- Stateless conversion (no internal state between calls)
- Type-safe mapping using generated proto definitions
- Message ID-based routing to correct proto message type
- Efficient field copying with minimal allocation

## Member Variables

### message_registry
std::unordered_map<uint32_t, ConversionFunction> mapping MAVLink message ID to conversion function. Built at initialization.

### reverse_registry
std::unordered_map<std::string, BackConversionFunction> mapping proto type name to MAVLink conversion. For proto-to-MAVLink direction.

## Functions

### to_proto
Takes const mavlink_message_t reference. Extracts message ID. Looks up in registry. Calls appropriate conversion function. Returns proto message wrapped in variant or Any.

### from_proto
Takes proto message (likely mavlink::MavlinkMessage wrapper). Extracts oneof field or type URL. Looks up in reverse registry. Calls conversion function. Returns mavlink_message_t.

### register_converter
Template function to register converters for each message type. Called during initialization for all supported messages. Takes message ID and lambda/function pointer.

### convert_heartbeat
Example specific converter: extracts HEARTBEAT fields (type, autopilot, base_mode, etc.) from mavlink_heartbeat_t. Populates proto Heartbeat message. Returns it.

### extract_common_fields
Helper to extract system_id, component_id, timestamp from mavlink_message_t header. Common to all messages.

### handle_arrays
Helper for converting fixed-size arrays (char[16], uint8_t[3], etc.) between MAVLink and proto repeated fields.

### handle_enums
Helper for enum conversion. MAVLink uses uint8_t/uint16_t, proto uses enum types. Validate enum values are in range.

## Type Mapping Strategy

### Scalars
MAVLink uint8_t/int8_t -> proto int32 (proto3 has no smaller types)
MAVLink float -> proto float
MAVLink double -> proto double

### Arrays
MAVLink char[N] -> proto string (if text) or bytes (if binary)
MAVLink uint8_t[N] -> proto repeated uint32 or bytes
MAVLink float[N] -> proto repeated float

### Enums
MAVLink enum value -> proto enum by value
Handle MAVLink enum extensions: may not exist in proto

### Optional Fields
MAVLink uses magic values (NaN, INT32_MAX) -> proto optional or default value
Consider using proto3 optional keyword (C++20)

## Registration Pattern
Use static initialization to populate registries. Template metaprogramming or macro generation. One register call per message type. Consider code generation from XML definitions.

## Error Handling
- Unknown message ID: log warning, return empty optional or error type
- Invalid enum value: use UNKNOWN/UNSPECIFIED sentinel
- Array size mismatch: truncate or pad as appropriate
- NaN/infinity in floats: preserve if proto allows

## Performance Considerations
- Conversion is on hot path: avoid allocations
- Use move semantics for proto messages (unique_ptr or std::optional)
- Registry lookups should be O(1): std::unordered_map
- Consider perfect forwarding for zero-copy where possible
- Profile per-message conversion time
- Potentially generate conversion code for compile-time optimization

## Code Generation
Ideal implementation: generate converters from MAVLink XML definitions. Parse XML to get message structure. Generate C++ conversion functions. Include in build. Avoids manual maintenance for 300+ messages.
