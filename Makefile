# Makefile for http-sniffer using CMake
# Merged functionality from build.sh

.PHONY: all clean debug release install uninstall help nfm clean-build

# Default values
BUILD_TYPE ?= Release
ENABLE_NFM ?= OFF
CLEAN_BUILD ?= false

# Default target
all: release

# Build in release mode
release:
	@echo "Building in release mode..."
	@mkdir -p build
	@cd build && cmake -DCMAKE_BUILD_TYPE=Release -DENABLE_NFM=$(ENABLE_NFM) .. && make

# Build in debug mode
debug:
	@echo "Building in debug mode..."
	@mkdir -p build
	@cd build && cmake -DCMAKE_BUILD_TYPE=Debug -DENABLE_NFM=$(ENABLE_NFM) .. && make

# Build with NFM support
nfm:
	@echo "Building with NFM support..."
	@mkdir -p build
	@cd build && cmake -DCMAKE_BUILD_TYPE=Release -DENABLE_NFM=ON .. && make

# Clean build directory
clean:
	@echo "Cleaning build directory..."
	@rm -rf build
	@rm -rf bin/*

# Clean build (clean then build)
clean-build: clean
	@echo "Performing clean build..."
	@$(MAKE) release

# Debug clean build
debug-clean: clean
	@echo "Performing debug clean build..."
	@$(MAKE) debug

# NFM clean build
nfm-clean: clean
	@echo "Performing NFM clean build..."
	@$(MAKE) nfm

# Install (if supported by CMake)
install:
	@echo "Installing..."
	@cd build && make install

# Uninstall (if supported by CMake)
uninstall:
	@echo "Uninstalling..."
	@cd build && make uninstall

# Show help
help:
	@echo "http-sniffer Makefile - Unified build interface"
	@echo ""
	@echo "Available targets:"
	@echo "  all          - Build in release mode (default)"
	@echo "  release      - Build in release mode"
	@echo "  debug        - Build in debug mode"
	@echo "  nfm          - Build with NFM support"
	@echo "  clean        - Clean build directory"
	@echo "  clean-build  - Clean then build in release mode"
	@echo "  debug-clean  - Clean then build in debug mode"
	@echo "  nfm-clean    - Clean then build with NFM support"
	@echo "  install      - Install the application"
	@echo "  uninstall    - Uninstall the application"
	@echo "  help         - Show this help message"
	@echo ""
	@echo "Environment variables:"
	@echo "  BUILD_TYPE   - Set to 'Debug' or 'Release' (default: Release)"
	@echo "  ENABLE_NFM   - Set to 'ON' or 'OFF' (default: OFF)"
	@echo "  CLEAN_BUILD  - Set to 'true' to clean before building"
	@echo ""
	@echo "Note: This Makefile is a wrapper around CMake."
	@echo "Build artifacts are created in the 'bin/' directory." 