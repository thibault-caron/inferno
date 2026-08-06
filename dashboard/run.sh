#!/bin/bash


echo "========================================"
echo "  Running Inferno Dashboard"
echo "========================================"
echo ""

# Navigate to script's directory
cd "$(dirname "$0")"

# Clear Snap environment variables that can interfere with Qt
unset GTK_PATH
unset LD_LIBRARY_PATH

# Find and run the executable
if [ -f "build/dashboard" ]; then
    echo "[OK] Starting dashboard..."
    echo ""
    cd build
    ./dashboard
elif [ -f "build/Release/dashboard" ]; then
    echo "[OK] Starting dashboard (Release)..."
    echo ""
    cd build/Release
    ./dashboard
else
    echo "[ERROR] Client executable not found!"
    echo ""
    echo "Please build the dashboard first using:"
    echo "  ./build.sh"
    echo ""
    exit 1
fi