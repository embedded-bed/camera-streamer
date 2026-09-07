#include <gst/app/gstappsink.h>
#include <gst/app/gstappsrc.h>
#include <gst/gst.h>
#include <gst/video/video.h>

#include <opencv2/core.hpp>
#include <opencv2/imgproc.hpp>

#include <spdlog/spdlog.h>

#include <cstdint>
#include <cstring>
#include <string>

int main(int argc, char* argv[])
{
    constexpr int width = 1920;
    constexpr int height = 1080;
    constexpr int fps = 30;

    gst_init(&argc, &argv);

    /*
     * RTSP -> H264 -> raw BGR -> appsink
     */
    /*
    const std::string input_pipeline_description =
        "rtspsrc "
        "location=rtsp://192.168.1.100:8554/input "
        "latency=100 "
        "protocols=tcp ! "
        "rtph264depay ! "
        "h264parse ! "
        "avdec_h264 ! "
        "videoconvert ! "
        "video/x-raw,"
            "format=BGR,"
            "width=1920,"
            "height=1080 ! "
        "appsink "
            "name=input_sink "
            "max-buffers=1 "
            "drop=true "
            "sync=false";
    */

    // videotestsrc pattern=snow ! video/x-raw,width=1280,height=720 ! autovideosink
    /*
     * Video Test Source -> appsink
     */
    const std::string input_pipeline_description =
        "videotestsrc is-live=true pattern=snow ! "
        "video/x-raw,format=BGR,width=1920,height=1080,framerate=30/1 ! "
        "appsink "
            "name=input_sink "
            "max-buffers=1 "
            "drop=true "
            "sync=false";

    /*
     * appsrc -> raw BGR -> H264 -> RTSP
     */
    const std::string output_pipeline_description =
        "appsrc "
            "name=output_source "
            "is-live=true "
            "format=time "
            "do-timestamp=true "
            "block=true ! "
        "videoconvert ! "
        "video/x-raw,format=I420 ! "
        "x264enc "
            "tune=zerolatency "
            "speed-preset=ultrafast "
            "bitrate=4000 "
            "key-int-max=30 ! "
        "h264parse config-interval=-1 ! "
        "rtspclientsink "
            "location=rtsp://127.0.0.1:8554/camera "
            "protocols=tcp";

    GError* error = nullptr;

    GstElement* input_pipeline =
        gst_parse_launch(input_pipeline_description.c_str(), &error);

    if (!input_pipeline) {
        spdlog::error(
            "Failed to create input pipeline: {}",
            error ? error->message : "unknown error");

        if (error)
            g_error_free(error);

        return 1;
    }

    error = nullptr;

    GstElement* output_pipeline =
        gst_parse_launch(output_pipeline_description.c_str(), &error);

    if (!output_pipeline) {
        spdlog::error(
            "Failed to create output pipeline: {}",
            error ? error->message : "unknown error");

        if (error)
            g_error_free(error);

        gst_object_unref(input_pipeline);
        return 1;
    }

    auto* input_sink = GST_APP_SINK(
        gst_bin_get_by_name(GST_BIN(input_pipeline), "input_sink"));

    auto* output_source = GST_APP_SRC(
        gst_bin_get_by_name(GST_BIN(output_pipeline), "output_source"));

    if (!input_sink || !output_source) {
        spdlog::error("Could not find appsink or appsrc");

        if (input_sink)
            gst_object_unref(input_sink);

        if (output_source)
            gst_object_unref(output_source);

        gst_object_unref(input_pipeline);
        gst_object_unref(output_pipeline);

        return 1;
    }

    /*
     * Tell appsrc exactly what we will push into it.
     */
    GstCaps* output_caps = gst_caps_new_simple(
        "video/x-raw",
        "format", G_TYPE_STRING, "BGR",
        "width", G_TYPE_INT, width,
        "height", G_TYPE_INT, height,
        "framerate", GST_TYPE_FRACTION, fps, 1,
        nullptr);

    gst_app_src_set_caps(output_source, output_caps);
    gst_caps_unref(output_caps);

    /*
     * Start both pipelines.
     */
    gst_element_set_state(output_pipeline, GST_STATE_PLAYING);
    gst_element_set_state(input_pipeline, GST_STATE_PLAYING);

    spdlog::info("Streaming started");

    uint64_t counter = 0;

    while (true) {
        /*
         * Wait for next decoded frame.
         */
        GstSample* sample = gst_app_sink_pull_sample(input_sink);

        if (!sample) {
            spdlog::error("Input stream ended");
            break;
        }

        GstBuffer* buffer = gst_sample_get_buffer(sample);
        GstCaps* caps = gst_sample_get_caps(sample);

        if (!buffer || !caps) {
            gst_sample_unref(sample);
            continue;
        }

        /*
         * Determine frame dimensions and stride from GStreamer.
         */
        GstVideoInfo video_info{};

        if (!gst_video_info_from_caps(&video_info, caps)) {
            spdlog::error("Failed to read video information");
            gst_sample_unref(sample);
            break;
        }

        const int frame_width = GST_VIDEO_INFO_WIDTH(&video_info);
        const int frame_height = GST_VIDEO_INFO_HEIGHT(&video_info);
        const int stride = GST_VIDEO_INFO_PLANE_STRIDE(&video_info, 0);

        /*
         * Map the GStreamer buffer into our address space.
         */
        GstMapInfo map{};

        if (!gst_buffer_map(buffer, &map, GST_MAP_READ)) {
            spdlog::error("Failed to map input buffer");
            gst_sample_unref(sample);
            continue;
        }

        /*
         * Create an OpenCV view onto the GStreamer buffer.
         *
         * We clone it because the input buffer belongs to GStreamer and is
         * mapped read-only.
         */
        cv::Mat gst_frame(
            frame_height,
            frame_width,
            CV_8UC3,
            map.data,
            stride);

        cv::Mat frame = gst_frame.clone();
        gst_buffer_unmap(buffer, &map);
        gst_sample_unref(sample);

        /*
         * OpenCV processing.
         */
        const std::string text = std::to_string(counter++);

        constexpr double font_scale = 2.0;
        constexpr int thickness = 3;

        int baseline = 0;

        const cv::Size text_size = cv::getTextSize(
            text,
            cv::FONT_HERSHEY_SIMPLEX,
            font_scale,
            thickness,
            &baseline);

        const cv::Point position{
            (frame.cols - text_size.width) / 2,
            (frame.rows + text_size.height) / 2
        };

        cv::putText(
            frame,
            text,
            position,
            cv::FONT_HERSHEY_SIMPLEX,
            font_scale,
            cv::Scalar(255, 255, 255),
            thickness,
            cv::LINE_AA);

        /*
         * Allocate a new GStreamer buffer for the processed frame.
         */
        const gsize output_size =
            frame.total() * frame.elemSize();

        GstBuffer* output_buffer =
            gst_buffer_new_allocate(nullptr, output_size, nullptr);

        GstMapInfo output_map{};

        if (!gst_buffer_map(
                output_buffer,
                &output_map,
                GST_MAP_WRITE)) {

            spdlog::error("Failed to map output buffer");

            gst_buffer_unref(output_buffer);
            break;
        }

        std::memcpy(
            output_map.data,
            frame.data,
            output_size);

        gst_buffer_unmap(output_buffer, &output_map);

        /*
         * Ownership of output_buffer transfers to appsrc here.
         */
        const GstFlowReturn result =
            gst_app_src_push_buffer(output_source, output_buffer);

        if (result != GST_FLOW_OK) {
            spdlog::error(
                "Failed to push output buffer: {}",
                static_cast<int>(result));

            break;
        }
    }

    gst_app_src_end_of_stream(output_source);

    gst_element_set_state(input_pipeline, GST_STATE_NULL);
    gst_element_set_state(output_pipeline, GST_STATE_NULL);

    gst_object_unref(input_sink);
    gst_object_unref(output_source);

    gst_object_unref(input_pipeline);
    gst_object_unref(output_pipeline);

    return 0;
}