# Start from the official Ubuntu 22.04 LTS image.
# Using a specific version is recommended for reproducibility.
FROM ubuntu:22.04

# Avoid interactive prompts during package installation
ENV DEBIAN_FRONTEND=noninteractive

# Install essential C++ development tools, curl, and pip
RUN apt-get update && \
    apt-get install -y --no-install-recommends \
    build-essential \
    gdb \
    cmake \
    software-properties-common \
    gcc-11 g++-11 \
    clang \
    clang-format \
    clang-tidy \
    make \
    ninja-build \
    git \
    curl \
    python3 \
    python3-pip \
    unzip \
    && rm -rf /var/lib/apt/lists/*

# Download and install nvm:
RUN curl -L -o "node-v22.18.0-linux-x64.tar.xz" "https://nodejs.org/dist/v22.18.0/node-v22.18.0-linux-x64.tar.xz"
RUN tar -xJf "node-v22.18.0-linux-x64.tar.xz" -C "/usr/local" --strip-components=1
RUN rm node-v22.18.0-linux-x64.tar.xz

RUN npm install -g @anthropic-ai/claude-code

# Install the latest version of CMake
ARG CMAKE_VERSION=3.27.7
RUN curl -sSL https://github.com/Kitware/CMake/releases/download/v${CMAKE_VERSION}/cmake-${CMAKE_VERSION}-linux-x86_64.sh -o /tmp/cmake.sh && \
    chmod +x /tmp/cmake.sh && \
    /tmp/cmake.sh --prefix=/usr/local --skip-license && \
    rm /tmp/cmake.sh

# Install Conan using pip
RUN pip3 install --no-cache-dir --upgrade pip
RUN pip3 install --no-cache-dir "conan>=2.0"

# Create a non-root user for development to improve security
# The user will be 'vscode', with UID/GID 1000, which is standard.
RUN groupadd --gid 1000 vscode && \
    useradd --uid 1000 --gid 1000 -m -s /bin/bash vscode

# Set up custom Conan profile for the new user
USER vscode
RUN conan profile detect --force
COPY conan-profile /home/vscode/.conan2/profiles/default

# Set back to root to allow VSCode to perform setup, it will switch to 'vscode' user later.
USER root

# Final command to keep the container running
CMD ["bash", "-c"]