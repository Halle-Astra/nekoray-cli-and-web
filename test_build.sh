#!/bin/bash

# NekoRay Headless Build Test Script

echo "=== NekoRay Headless Build Test ==="

# Check for required tools
echo "Checking build dependencies..."

if ! command -v cmake &> /dev/null; then
    echo "Error: CMake not found"
    exit 1
fi

if ! command -v make &> /dev/null; then
    echo "Error: Make not found"
    exit 1
fi

# Check for Qt6
echo "Checking Qt6..."
if ! pkg-config --exists Qt6Core; then
    echo "Warning: Qt6 not found via pkg-config, build may fail"
fi

# Create build directory
BUILD_DIR="build-headless-test"
echo "Creating build directory: $BUILD_DIR"
mkdir -p "$BUILD_DIR"
cd "$BUILD_DIR"

# Configure with CMake
echo "Configuring with CMake..."
cmake -DCMAKE_BUILD_TYPE=Release \
      -DNKR_NO_EXTERNAL=ON \
      -DNKR_HEADLESS_MODE=ON \
      -f ../nekoray/CMakeLists_headless.txt \
      ../nekoray

if [ $? -ne 0 ]; then
    echo "Error: CMake configuration failed"
    exit 1
fi

echo "Build configuration completed successfully!"
echo
echo "To build the project, run:"
echo "  cd $BUILD_DIR"
echo "  make -j\$(nproc)"
echo
echo "Executables will be:"
echo "  - nekoray-cli: Command line interface"
echo "  - nekoray-daemon: Web interface daemon"
echo
echo "=== Configuration Summary ==="
echo "Build type: Release"
echo "Headless mode: Enabled"
echo "External libs: Disabled (for testing)"
echo "Source directory: $(realpath ../nekoray)"
echo "Build directory: $(realpath .)"
echo
echo "Note: For full functionality, you'll need to:"
echo "1. Install nekobox_core binary in the same directory"
echo "2. Ensure proper permissions for TUN mode"
echo "3. Configure profiles before running"