#!/bin/bash
set -e
echo "Building Stick..."
make -j$(nproc)
echo "Build complete. Run ./stick to start."