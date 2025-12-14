#!/bin/bash
# This script sets up the environment for the project
# Tested on Ubuntu 22.04

set -e

echo "Setting up the environment..."

# Install system dependencies
echo "-- Installing system dependencies..."
sudo apt-get update
sudo apt-get install -y \
    build-essential \
    cmake \
    git \
    python3 \
    python3-pip \
    libgrpc++-dev \
    libprotobuf-dev \
    protobuf-compiler-grpc

# Clone MAVLink repository
if [ ! -d "mavlink" ]; then
    echo "-- Cloning mavlink repository..."
    git clone https://github.com/mavlink/mavlink.git --recursive
else
    echo "-- Mavlink repository already exists, skipping clone."
fi

cd mavlink
# Ensure submodules are initialized and updated
echo "-- Initializing and updating submodules..."
git submodule update --init --recursive

# Generate MAVLink headers
echo "-- Generating MAVLink headers..."
python3 -m pip install -r pymavlink/requirements.txt
python3 -m pymavlink.tools.mavgen --lang=C --wire-protocol=2.0 --output=generated/include/mavlink/v2.0 message_definitions/v1.0/common.xml

# run CMake for installing headers
echo "-- Running CMake to install headers..."
cmake -Bbuild -H. -DCMAKE_INSTALL_PREFIX=install -DMAVLINK_DIALECT=common -DMAVLINK_VERSION=2.0
cmake --build build --target install