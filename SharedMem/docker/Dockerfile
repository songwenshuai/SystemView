#==============================================================================
# SharedMem Kernel Module Docker Image
# Platform: Linux container for macOS or Linux Docker hosts
# Purpose: Provide kbuild user-space dependencies for SharedMem.ko builds
#==============================================================================

FROM debian:bookworm-slim

#==============================================================================
# Build Arguments
#==============================================================================

ARG DEBIAN_FRONTEND=noninteractive
ARG INSTALL_DEBIAN_CROSS_TOOLCHAINS=0

#==============================================================================
# Build Dependencies
#==============================================================================

# The target kernel build directory is intentionally not installed here.
# It must be mounted by the host wrapper because kernel modules must match the
# exact target kernel source, configuration, symbol versions, and architecture.
RUN set -eux; \
    packages="\
        bash \
        bc \
        binutils \
        bison \
        build-essential \
        ca-certificates \
        dwarves \
        flex \
        kmod \
        libelf-dev \
        libssl-dev \
        make \
        pkg-config \
    "; \
    if [ "${INSTALL_DEBIAN_CROSS_TOOLCHAINS}" = "1" ]; then \
        packages="${packages} gcc-aarch64-linux-gnu gcc-arm-linux-gnueabihf"; \
    fi; \
    apt-get update; \
    apt-get install -y --no-install-recommends ${packages}; \
    rm -rf /var/lib/apt/lists/*

#==============================================================================
# Container Entrypoint
#==============================================================================

COPY build-kernel-module.sh /usr/local/bin/build-sharedmem-kernel
RUN chmod 0755 /usr/local/bin/build-sharedmem-kernel

WORKDIR /work/SystemView
ENTRYPOINT ["/usr/local/bin/build-sharedmem-kernel"]
