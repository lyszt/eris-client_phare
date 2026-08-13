#!/bin/bash
# ErisCLI Dependency Installation Script

set -e

echo "Installing dependencies for ErisCLI..."

# Detect OS
if [ -f /etc/os-release ]; then
    . /etc/os-release
    OS=$ID
else
    echo "Unable to detect OS"
    exit 1
fi

# Install based on OS
case "$OS" in
    ubuntu|debian)
        echo "Detected Debian/Ubuntu"
        sudo apt update
        sudo apt install -y \
            build-essential \
            cmake \
            pkg-config \
            libcurl4-openssl-dev \
            libssl-dev \
            libgit2-dev \
            libreadline-dev \
            swi-prolog
        ;;

    fedora|rhel|centos)
        echo "Detected Fedora/RHEL/CentOS"
        sudo dnf install -y \
            gcc \
            make \
            cmake \
            pkgconfig \
            libcurl-devel \
            openssl-devel \
            libgit2-devel \
            readline-devel \
            swi-prolog
        ;;

    arch|manjaro)
        echo "Detected Arch Linux"
        sudo pacman -S --needed \
            base-devel \
            cmake \
            pkg-config \
            curl \
            openssl \
            libgit2 \
            readline \
            swi-prolog
        ;;

    *)
        echo "Unsupported OS: $OS"
        echo "Please install manually: build-essential, cmake, libcurl-dev"
        exit 1
        ;;
esac

echo ""
echo "✓ Dependencies installed successfully!"
echo ""
echo "Next steps:"
echo "  make        # Build the project"
echo "  make run    # Build and run"
