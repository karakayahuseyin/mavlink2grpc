# Examples

Python examples demonstrating how to use the mavlink2grpc bridge.

## Prerequisites

```bash
# Install Python gRPC dependencies
pip install grpcio grpcio-tools

# Create python files from generated protos
python3 -m grpc_tools.protoc -I./proto --python_out=./generated --grpc_python_out=./generated ./proto/mavlink_bridge.proto ./proto/mavlink/common.proto
```

## Available Examples

### takeoff.py

Sends a takeoff command to a MAVLink vehicle and monitors the acknowledgment.

**Usage:**
```bash
# Takeoff to 10 meters (default)
python3 takeoff.py

# Custom altitude
python3 takeoff.py --altitude 20.0

# Custom gRPC server address
python3 takeoff.py --host 192.168.1.100:50051 --altitude 15.0

# Custom target system/component
python3 takeoff.py --system 1 --component 1 --altitude 10.0
```

**Arguments:**
- `--host`: gRPC bridge address (default: `localhost:50051`)
- `--altitude`: Takeoff altitude in meters (default: `10.0`)
- `--system`: Target system ID (default: `1`)
- `--component`: Target component ID (default: `1`)

**What it does:**
1. Connects to the mavlink2grpc bridge
2. Sends a `COMMAND_LONG` message with `MAV_CMD_NAV_TAKEOFF`
3. Monitors for `COMMAND_ACK` response from the vehicle
4. Displays the result (accepted/rejected)

## Running the Examples

1. Make sure the bridge is running:
```bash
cd ../bridge/build
./mav2grpc_bridge -c udp://:14550 -g 0.0.0.0:50051
```

2. Connect your MAVLink vehicle (e.g., SITL, real drone, or simulator)

3. Run the example:
```bash
cd ../examples/python
python3 takeoff.py --altitude 10.0
```

## Creating Your Own Client

You can use these examples as a starting point for your own MAVLink client. The general pattern is:

1. Import generated proto files from `../generated/`
2. Create a gRPC channel and stub
3. Create MAVLink messages using the proto message types
4. Send messages using `stub.SendMessage()`
5. Stream messages using `stub.StreamMessages()`

See `takeoff.py` for a complete working example.
