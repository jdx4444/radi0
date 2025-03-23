#!/bin/bash
# setup_pi.sh
# Script to set up the radi0 project on Raspberry Pi

set -e

echo "Starting setup for radi0 on Raspberry Pi..."

# Update package lists
echo "Updating package lists..."
sudo apt-get update

# Install essential build tools
echo "Installing build-essential, git, and cmake..."
sudo apt-get install -y build-essential git cmake

# Install SDL2 development libraries
echo "Installing SDL2 development libraries..."
sudo apt-get install -y libsdl2-dev

# Install SDL2_mixer development libraries
echo "Installing SDL2_mixer development libraries..."
sudo apt-get install -y libsdl2-mixer-dev

# Install D-Bus development libraries
echo "Installing D-Bus development libraries..."
sudo apt-get install -y libdbus-1-dev

# Install OpenGL development libraries
echo "Installing OpenGL development libraries..."
sudo apt-get install -y libgl1-mesa-dev

# Install additional dependencies if any (e.g., pkg-config)
echo "Installing additional dependencies..."
sudo apt-get install -y pkg-config

# Optional: Install libraries for image handling if not already included
# Uncomment the following lines if necessary
# echo "Installing libjpeg-dev and libpng-dev..."
# sudo apt-get install -y libjpeg-dev libpng-dev

# Verify installations
echo "Verifying installations..."
command -v g++ >/dev/null 2>&1 || { echo >&2 "g++ is not installed. Aborting."; exit 1; }
command -v make >/dev/null 2>&1 || { echo >&2 "make is not installed. Aborting."; exit 1; }
command -v sdl2-config >/dev/null 2>&1 || { echo >&2 "SDL2 is not installed correctly. Aborting."; exit 1; }

echo "All dependencies are installed successfully."

# Optional: Build the project to verify setup
# Uncomment the following lines to enable automatic building
# echo "Building the project..."
# make build_pi
# echo "Project built successfully."

echo "Setup completed. You can now build the project using 'make build_pi'."
