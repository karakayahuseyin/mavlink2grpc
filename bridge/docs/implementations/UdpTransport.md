# UdpTransport

## Purpose
Implements ITransport for UDP socket communication. Supports both unicast and broadcast MAVLink traffic. Commonly used for ground control station communication.

## Design Principles
- Connection-less protocol handling
- Support for multiple remote endpoints
- Broadcast capability for MAVLink discovery
- Non-blocking socket operations

## Member Variables

### socket_fd
Socket file descriptor. Use int on POSIX, SOCKET on Windows. Wrap in std::optional or use -1 as invalid sentinel.

### local_port
uint16_t for the port to bind to. Common MAVLink ports: 14550 (GCS), 14555 (companion computer).

### remote_endpoints
std::vector of sockaddr_in structures for known remote endpoints. Messages received from new endpoints can be added dynamically.

### bind_address
std::string for local interface to bind ("0.0.0.0" for all interfaces, or specific IP).

### broadcast_enabled
bool flag indicating if socket should support broadcast. Requires SO_BROADCAST socket option.

### receive_buffer
std::array or std::vector for incoming datagram assembly. Size should be at least MTU (1500 bytes) or larger (4KB recommended).

## Functions

### Constructor
Takes local port and optional bind address. Store configuration but don't create socket yet.

### open
Creates UDP socket using socket(AF_INET, SOCK_DGRAM). Binds to local port. Sets SO_REUSEADDR for port sharing. Enables SO_BROADCAST if requested. Sets non-blocking mode with fcntl/ioctlsocket.

### close
Shuts down socket gracefully. Clears endpoint list. Safe to call multiple times.

### read
Uses recvfrom() to get datagram and source address. Stores sender in remote_endpoints if new. Copies payload to provided buffer. Returns bytes received or 0 if no data available.

### write
Uses sendto() to transmit to all known remote endpoints. If no endpoints known, may broadcast. Returns bytes sent to first endpoint (all or nothing semantics).

### add_remote_endpoint
Public method to manually register a remote endpoint (IP:port pair). Useful for establishing initial connection.

### enable_broadcast
Configures socket for broadcast transmission. Sets SO_BROADCAST option.

## Threading Considerations
- UDP sockets can be shared across threads with care
- Read/write on same socket from different threads generally safe
- Endpoint list needs mutex protection if modified from multiple threads

## Network Considerations
- UDP is unreliable: packets may be lost, duplicated, or reordered
- No flow control: can overwhelm receiver
- MTU limits packet size (typically 1500 bytes - IP/UDP headers)
- Consider packet fragmentation for large messages

## Error Handling
- EAGAIN/EWOULDBLOCK: no data available (normal for non-blocking)
- EMSGSIZE: message too large for buffer
- Network unreachable errors: log but continue
- Track packet loss statistics for diagnostics

## Performance Considerations
- Use larger receive buffer to reduce system call overhead
- Consider SO_RCVBUF/SO_SNDBUF socket options for kernel buffer sizing
- Batch multiple small messages into single datagram when possible
- Monitor for congestion and implement backpressure
