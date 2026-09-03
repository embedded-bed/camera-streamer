#include <opencv2/opencv.hpp>

#include <stdexcept>
#include <string>

int main()
{
    constexpr int width = 1920;
    constexpr int height = 1080;
    constexpr double fps = 30.0;

    const std::string capture_pipeline =
        "rtspsrc location=rtsp://192.168.1.100:8554/input "
        "latency=100 protocols=tcp ! "
        "rtph264depay ! "
        "h264parse ! "
        "avdec_h264 ! "
        "videoconvert ! "
        "video/x-raw,format=BGR ! "
        "appsink max-buffers=1 drop=true sync=false";

    cv::VideoCapture capture(
        capture_pipeline,
        cv::CAP_GSTREAMER
    );

    if (!capture.isOpened())
        throw std::runtime_error("Failed to open camera");

    const std::string output_pipeline =
        "appsrc ! "
        "videoconvert ! "
        "video/x-raw,format=I420 ! "
        "x264enc "
            "tune=zerolatency "
            "speed-preset=ultrafast "
            "bitrate=4000 "
            "key-int-max=30 ! "
        "h264parse ! "
        "rtspclientsink "
            "location=rtsp://127.0.0.1:8554/camera "
            "protocols=tcp";

    cv::VideoWriter writer(
        output_pipeline,
        cv::CAP_GSTREAMER,
        0,
        fps,
        cv::Size(width, height),
        true
    );

    if (!writer.isOpened())
        throw std::runtime_error("Failed to open RTSP output");

    cv::Mat frame;

    while (capture.read(frame)) {
        cv::putText(
            frame,
            "OpenCV",
            {50, 100},
            cv::FONT_HERSHEY_SIMPLEX,
            1.5,
            {0, 255, 0},
            2
        );

        writer.write(frame);
    }
}