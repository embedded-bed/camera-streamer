FROM debian:13.6-slim

# Permission setup
ARG DOCKER_USER=builder
ARG USER_ID=1000
ARG GROUP_ID=1000
ARG WORKDIR_PATH="/workdir"

ENV DEBIAN_FRONTEND=noninteractive

RUN apt-get update; \
    apt-get upgrade -y; \
    apt-get install -y \
        build-essential \
        cmake \
        curl \
        ca-certificates \
        git \
        pkg-config \
        libopencv-dev \
        libspdlog-dev \
        libgstreamer1.0-dev \
        libgstreamer-plugins-base1.0-dev \
        gstreamer1.0-plugins-base \
        gstreamer1.0-plugins-good \
        gstreamer1.0-plugins-bad \
        gstreamer1.0-plugins-ugly \
        gstreamer1.0-libav \
        gstreamer1.0-rtsp \
    && rm -rf /var/lib/apt/lists/*

# Install just command runner
RUN curl --proto '=https' --tlsv1.2 -sSf https://just.systems/install.sh | bash -s -- --to /usr/bin

RUN groupadd -o -g ${GROUP_ID} ${DOCKER_USER} \
    && useradd -o -m -u ${USER_ID} -g ${GROUP_ID} -s /bin/bash ${DOCKER_USER}

USER ${DOCKER_USER}

WORKDIR ${WORKDIR_PATH}

ENV IN_DOCKER=1