# ITransport

## Purpose
Abstract interface defining the contract for all transport layer implementations. Provides a unified API for reading and writing bytes regardless of the underlying physical medium (serial, UDP, TCP).

## Design Principles
- Pure virtual interface (no implementation)
- Use C++20 concepts for type safety
- Non-copyable but movable
- RAII-compliant lifecycle management

## Member Variables
None. This is a pure interface.

## Virtual Functions

### open
Opens the connection to the physical transport medium. Returns boolean indicating success or failure. Should be idempotent (safe to call multiple times).

### close
Closes the connection and releases all resources. Must be safe to call even if connection is not open. Should cleanup any pending buffers or state.

### read
Non-blocking read operation. Takes a buffer pointer and maximum size. Returns number of bytes actually read (0 if no data available, -1 on error). Should use std::span<std::byte> for buffer safety.

### write
Writes data to the transport. Takes const buffer pointer and size. Returns number of bytes written (-1 on error). May block if internal buffers are full.

### is_open
Query function to check if connection is currently active. Returns boolean.

## Destructor
Virtual destructor to ensure proper cleanup of derived classes. Should be defaulted unless specific cleanup logic needed.

## Threading Considerations
- Interface does not mandate thread-safety
- Each implementation decides its own threading model
- Document thread-safety guarantees in derived classes

## Error Handling Strategy
- Use return codes for synchronous errors
- Consider std::expected<size_t, ErrorCode> for C++23-style error handling
- Avoid exceptions in hot path (read/write loops)

## Memory Management
- No dynamic allocation in interface
- Derived classes should use RAII for resource management
- Consider std::unique_ptr for platform-specific handles
