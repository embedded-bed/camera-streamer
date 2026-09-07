set positional-arguments := true
set dotenv-load := true

DOCKER_IMAGE := "camera-streamer-builder"
DOCKER_RUN := "docker run --rm -it -v /dev/:/dev/ -v ./:$(pwd) --privileged"

[private]
@default:
    just --list

# build binary
build:
    #!/bin/bash
    set -ex
    mkdir -p build
    cd build
    cmake ..
    make -j `nproc --ignore 2`
    cp compile_commands.json ..

# Clean build directory
clean:
    rm -rf build bin compile_commands.json

# Run binary
run:
    ./bin/camera-streamer

# Run docker compose
run-compose:
    docker compose up -d
    docker compose logs -f -n 100

# Stop docker compose
stop-compose:
    docker compose stop


# Get logs from docker compose
log:
    docker compose logs -f -n 100

# Build docker image
docker-build:
    docker build . \
        --build-arg USER_ID=$(id -u) \
        --build-arg GROUP_ID=$(id -g) \
        --build-arg DOCKER_USER=$(id -u -n) \
        --build-arg WORKDIR_PATH=$(pwd) \
        -t {{ DOCKER_IMAGE }}

# Mount the project the Docker container and open a shell
docker-shell:
    {{ DOCKER_RUN }} {{ DOCKER_IMAGE }} bash

# Run command in docker container, e.g: `just docker-run just build`
docker *args:
    #!/bin/bash
    set -ex
    {{ DOCKER_RUN }} {{ DOCKER_IMAGE }} just "$@"