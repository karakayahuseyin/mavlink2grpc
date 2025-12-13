# SerialTransport

## Purpose
Implements ITransport for serial port communication. Handles platform-specific serial port operations (POSIX termios on Linux, COM ports on Windows).

## Design Principles
- RAII for file descriptor/handle management
- Platform abstraction using conditional compilation
- Zero-copy where possible
- Non-blocking I/O with configurable timeout

## Member Variables

### port_name
std::string storing the device path ("/dev/ttyUSB0" or "COM3"). Immutable after construction.

### baud_rate
Integer storing configured baud rate (9600, 57600, 115200, etc.). Common MAVLink rates: 57600 or 115200.

### file_descriptor
Platform-specific handle. Use int for POSIX, HANDLE for Windows. Wrap in optional or use -1/INVALID_HANDLE_VALUE as sentinel.

### termios_config
Linux only. Original terminal settings stored for restoration on close.

### read_timeout
std::chrono::milliseconds for read operation timeout. Affects blocking behavior.

## Functions

### Constructor
Takes port name and baud rate. Does not open connection (lazy initialization). Use explicit keyword to prevent implicit conversions.

### open
Platform-specific implementation:
- Linux: Use ::open(), configure with tcgetattr/tcsetattr
- Windows: CreateFile() with DCB configuration
Sets non-blocking mode, configures 8N1 (8 data bits, no parity, 1 stop bit).

### close
Restores original terminal settings (Linux). Closes file descriptor/handle. Safe to call multiple times.

### read
Uses ::read() on POSIX or ReadFile() on Windows. Implements timeout using select/poll or OVERLAPPED I/O. Returns actual bytes read.

### write
Uses ::write() on POSIX or WriteFile() on Windows. May need to loop for partial writes. Returns total bytes written.

### configure_port
Private helper to set baud rate, parity, stop bits. Uses cfsetispeed/cfsetospeed on POSIX.

## Threading Considerations
- Not thread-safe by default
- Concurrent read/write may work on full-duplex hardware
- Consider std::mutex if shared across threads
- Better approach: dedicate one thread per serial connection

## Platform Abstraction
Use preprocessor directives for platform-specific code:
- #ifdef __linux__ for POSIX
- #ifdef _WIN32 for Windows
- Consider std::filesystem for path handling

## Error Handling
- Check errno/GetLastError() for detailed error information
- Map platform errors to common enum
- Log errors with context (port name, operation)

## Performance Considerations
- Avoid frequent open/close cycles
- Use larger buffers (4KB-8KB) for bulk reads
- Consider memory-mapped I/O for high-throughput scenarios
