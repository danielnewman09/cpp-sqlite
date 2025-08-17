# Start from the official Ubuntu 24.04 LTS image.
FROM ubuntu:24.04

# Avoid interactive prompts
ENV DEBIAN_FRONTEND=noninteractive

# ============================ THE FIX: PART 1 ============================
# Define arguments for user/group IDs. Default to 1000, a common standard.
ARG USER_UID=1000
ARG USER_GID=1000
# =======================================================================

# Install system dependencies
RUN apt-get update && \
    apt-get install -y --no-install-recommends \
    build-essential gdb cmake software-properties-common clang clang-format \
    clang-tidy make ninja-build pipx git curl python3-full python3-pip unzip \
    libgmp-dev libmpfr-dev libmpc-dev gcc-14 g++-14 \
    && rm -rf /var/lib/apt/lists/*

# Install Node.js
RUN curl -L -o "node-v22.18.0-linux-x64.tar.xz" "https://nodejs.org/dist/v22.18.0/node-v22.18.0-linux-x64.tar.xz" && \
    tar -xJf "node-v22.18.0-linux-x64.tar.xz" -C "/usr/local" --strip-components=1 && \
    rm node-v22.18.0-linux-x64.tar.xz

# Install global npm packages
RUN npm install -g @anthropic-ai/claude-code

# ============================ THE FIX: PART 2 ============================
# Create a non-root user with the UID/GID passed in as build arguments.
RUN groupadd --gid ${USER_GID} vscode && \
    useradd --uid ${USER_UID} --gid ${USER_GID} -m -s /bin/bash vscode
# =======================================================================

# Add the conan profile, ensuring correct ownership from the start.
# Note the use of build arguments in the --chown flag.
ADD --chown=${USER_UID}:${USER_GID} conan-profile /home/vscode/default-conan-profile

# Switch to the non-root user
USER vscode

# Set environment for the user
ENV PATH="/home/vscode/.local/bin:${PATH}"

# Install and configure Conan as the non-root user
RUN pipx install "conan>=2.0" && \
    conan profile detect --force && \
    mv /home/vscode/default-conan-profile /home/vscode/.conan2/profiles/default && \
    conan remote update conancenter --url="https://center2.conan.io"

# Switch back to root user.
USER root

# Keep the container running
CMD ["sleep", "infinity"]`