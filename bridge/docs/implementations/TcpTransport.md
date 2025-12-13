# TcpTransport

## Purpose
Implements ITransport for TCP socket communication. Provides reliable, ordered, connection-oriented MAVLink transport. Used when reliability is more important than latency.

## Design Principles
- Connection-oriented with explicit lifecycle
- Reliable delivery with flow control
- Support for both client and server modes
- Graceful connection recovery

## Member Variables

### socket_fd
Socket file descriptor for the connection. Use int on POSIX, SOCKET on Windows.

### mode
Enum indicating CLIENT or SERVER mode. Affects connection establishment logic.

### local_port
uint16_t for server mode: port to listen on. Not used in client mode.

### remote_host
std::string for client mode: hostname or IP to connect to. Not used in server mode.

### remote_port
uint16_t for client mode: destination port. Not used in server mode.

### listen_socket
Additional socket fd for server mode. Used for accept() operations. Should be separate from data socket.

### connection_state
Enum tracking current state: DISCONNECTED, CONNECTING, CONNECTED, ERROR. Thread-safe atomic or mutex-protected.

### reconnect_enabled
bool flag for automatic reconnection on disconnect. Useful for robust long-running connections.

### reconnect_interval
std::chrono::seconds delay between reconnection attempts. Exponential backoff recommended.

## Functions

### Constructor for Client Mode
Takes remote host and port. Stores connection parameters.

### Constructor for Server Mode
Takes local port to listen on. Prepares for incoming connections.

### open
Client mode: Creates socket and calls connect(). May block or timeout.
Server mode: Creates listen socket, binds to port, calls listen(). Then accept() for first connection.
Sets TCP_NODELAY to disable Nagle's algorithm for low latency.

### close
Graceful shutdown with shutdown(SHUT_RDWR) before close(). Closes both listen and data sockets in server mode.

### read
Uses recv() to read stream data. Handles partial reads. Returns bytes actually read. May return 0 on graceful disconnect (detect and set state).

### write
Uses send() to write stream data. Handles partial writes with loop. Returns total bytes written or -1 on error.

### accept_connection
Server mode only. Blocks on accept() to get incoming connection. Replaces current data socket. Consider moving to separate thread.

### reconnect
Client mode. Attempts to re-establish connection. Called automatically if reconnect_enabled. Uses exponential backoff.

### set_keepalive
Configures TCP keepalive options (SO_KEEPALIVE, TCP_KEEPIDLE, TCP_KEEPINTVL) to detect dead connections.

## Threading Considerations
- Separate thread for accept() in server mode recommended
- Read/write from different threads generally safe on TCP stream
- Connection state requires synchronization
- Consider one reader thread, one writer thread pattern

## Connection Management
- Detect disconnection: recv() returns 0
- Handle EPIPE/SIGPIPE on write to broken connection
- Implement heartbeat mechanism at application layer
- Track connection uptime and byte counters for monitoring

## Error Handling
- ECONNREFUSED: peer not listening
- ETIMEDOUT: connection attempt timeout
- EPIPE: broken pipe on write
- Distinguish transient vs permanent errors
- Log connection events for debugging

## Performance Considerations
- TCP_NODELAY critical for MAVLink (small messages, low latency)
- SO_SNDBUF/SO_RCVBUF tuning for throughput
- Avoid frequent connect/disconnect cycles
- Consider connection pooling for multiple vehicles
- Monitor send buffer fullness for backpressure
