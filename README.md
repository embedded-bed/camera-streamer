# Camera Streamer

Small C++ application for processing an RTSP camera stream using GStreamer and OpenCV.

The application:

```text
RTSP input
    ↓
GStreamer
    ↓
H.264 decode
    ↓
OpenCV processing
    ↓
H.264 encode
    ↓
RTSP
    ↓
go2rtc
    ↓
RTSP / WebRTC / HTTP
```

The application currently adds an incremental frame counter to the center of each frame.

## Requirements

The development environment is provided through Docker.

Required host software:

* Docker
* Docker Compose
* just

## Build Development Container

Build the development image:

```bash
just docker-build
```

Open a development shell:

```bash
just docker-shell
```

Inside the container, build the application:

```bash
just build
```

## Build Locally in Docker

The build can also be run directly without opening a shell:

```bash
just docker just build
```

## Run Complete Streaming Stack

The complete application and go2rtc server can be started using Docker Compose.

```bash
USER_ID=$(id -u) \
GROUP_ID=$(id -g) \
DOCKER_USER=$(id -un) \
docker compose up --build
```

This starts:

```text
camera-streamer
    |
    | RTSP publish
    v
rtsp://127.0.0.1:8554/camera
    |
    v
go2rtc
```

The camera streamer reads its configured input RTSP stream, processes each decoded frame using OpenCV, encodes the result as H.264, and publishes it to go2rtc.

## go2rtc

go2rtc acts as the streaming gateway.

The processed stream is published by the application to:

```text
rtsp://127.0.0.1:8554/camera
```

The go2rtc web interface is available at:

```text
http://localhost:1984
```

The processed stream can be opened directly over RTSP:

```text
rtsp://localhost:8554/camera
```

The built-in browser viewer can be opened at:

```text
http://localhost:1984/stream.html?src=camera
```

The browser viewer can automatically use WebRTC and other browser-compatible streaming mechanisms supported by go2rtc.

When accessing the service from another computer, replace `localhost` with the IP address of the machine running the containers.

For example:

```text
http://192.168.1.50:1984/stream.html?src=camera
```

or:

```text
rtsp://192.168.1.50:8554/camera
```

## Stop

Stop both services with:

```bash
docker compose down
```

## Development

The source directory is mounted into the application container at:

```text
/workdir
```

The executable is built as:

```text
build/camera-streamer
```

The normal development loop is therefore:

```bash
just docker-shell
just build
./build/camera-streamer
```

or simply:

```bash
docker compose up --build
```

## Debugging GStreamer

Check that the required GStreamer elements are installed:

```bash
gst-inspect-1.0 rtspsrc
gst-inspect-1.0 rtph264depay
gst-inspect-1.0 h264parse
gst-inspect-1.0 avdec_h264
gst-inspect-1.0 appsink
gst-inspect-1.0 appsrc
gst-inspect-1.0 x264enc
gst-inspect-1.0 rtspclientsink
```

For additional GStreamer logging:

```bash
GST_DEBUG=3 ./build/camera-streamer
```

For more verbose output:

```bash
GST_DEBUG=4 ./build/camera-streamer
```
