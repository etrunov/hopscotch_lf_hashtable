#!/bin/bash

# Build Script
# Usage: ./build.sh [clean]

set -e  # Exit immediately on error

BUILD_DIR="build"
MAIN_EXECUTABLE="hopscotch_ht_app"

# Clean if requested
if [[ "$1" == "clean" ]]; then
	rm -rf "$MAIN_EXECUTABLE"
	echo "Cleaning build directory..."
	rm -rf "${BUILD_DIR}"
	exit 0
fi

mkdir -p "${BUILD_DIR}"
cd "${BUILD_DIR}"

# Build
echo "Building project..."
cmake ..
make

# Copy executables to root for easy access
echo "Preparing binaries..."
cp "${MAIN_EXECUTABLE}" ..
cd ../

echo "Build successfully completed!"
echo "Execute ./${MAIN_EXECUTABLE}"
