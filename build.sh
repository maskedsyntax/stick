#!/bin/bash
set -e
echo "Building Stick..."
make -j$(nproc)
echo "Build complete. Run ./build/stick to start."