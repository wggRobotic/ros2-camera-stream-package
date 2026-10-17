#include "rclcpp/rclcpp.hpp"
#include "opencv2/opencv.hpp"

#include <gst/gst.h>
#include <gst/app/gstappsrc.h>
#include <gst/rtsp-server/rtsp-server.h>

#include <thread>

class CameraStreamNode : public rclcpp::Node {
    public:
        // Create the Node itself
        CameraStreamNode() : Node("camera_stream_node") {
            // Initialize the server
            gst_init(nullptr, nullptr);

            // Thdrow an error if the camera can't be loaded
            if (!camera_.open(0)) {
                throw std::runtime_error("ERROR: Unable to connect to the camera");
            }

            // Create a rtsp server
            setup_rtsp_server();

            // Create a new thread and run the Glib-loop
            gst_thread_ = std:thread([this]{
                g_main_loop_run(loop_);
            });

            // Log that the server is up
            RCLCPP_INFO(get_logger(), "RTSP-stream is now available under rtsp://127.0.0.1:8554/test");

            // Call the function once and start the stream
            CameraStreamNode::get_camera_image();
        }
        
        // Handle the GLib-loop within another thread
        ~CameraStreamNode() override {
            if (loop_) {
                g_main_loop_quit(loop_);
                g_main_loop_unref(loop_);
            }

            if (gst_thread_.joinable()){
                gst_thread_.join();
            }

            if (server_) {
                g_object_unref(server_);
            }
        }
    private:
        // Define the resolution and frame rate of the stream
        static constexpr int width_ = 640;
        static constexpr int height_ = 480;
        static constexpr int fps_ = 30;

        // Initialize some required variables
        cv::VideoCapture camera_;
        GMainLoop* loop_ = nullptr;
        GstRTSPServer* server_ = nullptr;
        std::thread gst_thread_;

        // Main function: Create the rtsp server and call a callback for the images
        void setup_rtsp_server() {
            loop_ = g_main_loop_new();
            server_ = gst_rtsp_server_new();

            GstRTSPMountPoints* mounts = gst_rtsp_server_get_mount_points(server_);
            GstRTSPMediaFactory* factory = gst_rtsp_media_factory_new();

            gst_rtsp_media_factory_set_launch(
                factory,
                "( "
                "appsrc name=src is-live=true format=time do-timestamp=true "
                "caps=\"video/x-raw,format=BGR,width=640,height=480,framerate=30/1\" "
                "! queue leaky=downstream max-size-buffers=1 "
                "! videoconvert "
                "! video/x-raw,format=I420 "
                "! x264enc tune=zerolatency speed-preset=ultrafast "
                "bitrate=1500 key-int-max=30 "
                "! rtph264pay name=pay0 pt=96 config-interval=1 "
                ")"
            );

            // All clients use the same rtsp-media pipeline
            gst_rtsp_media_factory_set_shared(factory, TRUE);

            // Once a client connects call appsrc and its callback
            g_signal_connect(factory, "media-configure", G_CALLBACK(on_media_configure), this);
        
            // Define the enpoints of the server
            gst_rtsp_mount_points_add_factory(mounts, "/test", factory);
            g_object_unref(mounts);

            if (gst_rtsp_server_attach(server_, nullptr) == 0) {
                throw std::runtime_error("ERROR: Unable to start the RTSP server");
            }
        }

        static void on_media_configure(GstRTSPMediaFactory*, GstRTSPMedia* media, gpointer user_data) {
            //
        }

        // Definition of the videostream
        struct ImageConfiguration {
            cv::VideoCapture camera;
            int width = 640;
            int heigt = 480;
            int fps = 30;
        };

        // Main function getting the camera image and streaming it via the RTSP protocoll
        void get_camera_image() {
            // Get the camera configuration
            ImageConfiguration image_configuration;

            // Get the current camera image via OpenCV
            // TODO: Support USB cameras with their own SDK
            // Connect to a certain camera
            int id = 0;
            image_configuration.camera(id);
            if (!image_configuration.camera.isOpened()) {
                RCLCPP_FATAL(this->get_logger(), "Unable to connect to the camera with the id '%i'", id);
                return;
            }

            // Apply the required camera configurations
            image_configuration.camera.set(cv::CAP_PROP_FRAME_WIDTH, camera_state.width);
            image_configuration.camera.set(cv::CAP_PROP_FRAME_HEIGHT, camera_state.height);
            image_configuration.camera.set(cv::CAP_PROP_FPS, camera_state.fps);
            
            // TODO: stream the image via RTSP
            // Initialize gstreamer
            gst_init(nullptr, nullptr)

            // Create the main loop
            GMainLoop* loop = g_main_loop_new(nullptr, FALSE);

            // Create a server instance
            GstRTSPServer* server = gst_rtsp_server_new();

            // Get the mount points for this server
            GstRTSPMountPoints* mounts = gst_rtsp_server_get_mount_points(server);

            // Create a media factory
            GstRTSPMediaFactory* factory = gst_rtsp_media_factory_new();

            // appsrc liefert BGR-Bilder. GStreamer wandelt sie in I420 um,
            // encodiert sie mit x264 und verpackt H.264 für RTSP/RTP.
            //
            // Die kurze Queue verwirft bei Rückstau alte Bilder, statt die Latenz
            // durch einen wachsenden Bildpuffer zu erhöhen.

            // Launch the factory and apply the required configuration
            gst_rtsp_media_factory_set_launch(
                factory,
                "( "
                "appsrc name=src "
                "is-live=true "
                "format=time "
                "do-timestamp=true "
                "block=true "
                "caps=\"video/x-raw,format=BGR,width=640,height=480,framerate=30/1\" "
                "! queue leaky=downstream max-size-buffers=1 "
                "! videoconvert "
                "! video/x-raw,format=I420 "
                "! x264enc tune=zerolatency speed-preset=ultrafast "
                "bitrate=1500 key-int-max=30 "
                "! rtph264pay name=pay0 pt=96 config-interval=1 "
                ")"
            );

            // Make sure all clients use the same pipeline
            gst_rtsp_media_factory_set_shared(factory, TRUE);

            // Das appsrc-Element wird erst beim Erzeugen der Medien verfügbar.
            // Der Callback bekommt den Zustand mit Kamera und Bildparametern.
            g_signal_connect(
                factory,
                "media-configure",
                G_CALLBACK(media_configure),
                &camera_state
            );

            // Host the streamer at rtsp://127.0.0.1:8554/test
            gst_rtsp_mount_points_add_factory(mounts, "/test", factory);

            // Remove the mount-point-reference
            g_object_unref(mounts);

            // Attach the server to the GMainContext
            if (gst_rtsp_server_attach(server, nullptr) == 0) {
                RCLCPP_ERROR(this->get_logger(), "Unable to start the RTSP server")
                g_object_unref(factory);
                g_object_unref(server);
                g_main_loop_unref(loop);
                return;
            }

            // TODO: Remove unused sessions regulary
            g_timeout_add_seconds(2, cleanup_sessions, server);

            // Log the servers state
            RCLCPP_INFO(this->get_logger(), "The rtsp server is ready: rtsp://127.0.0.1:8554/test");

            // Run the loop
            g_main_loop_run(loop);

            // Clean everything up and finish
            g_main_loop_unref(loop);
            g_object_unref(server);
        }

        // Called once the pipeline was created
        static void media_configure(GstRTSPMediaFactory*, GstRTSPMedia* media, gpointer user_data) {
            // Get the first element fromt the pipeline
            GstElement* pipeline = gst_rtsp_media_get_element(media);

            // Get the appsrc element called "src"
            GstElement* appsrc_element = gst_bin_get_by_name(GST_BIN(pipeline), "src");

            if (!appsrc_element) {
                g_warning("appsrc mit dem Namen 'src' wurde nicht gefunden");
                g_object_unref(pipeline);
                return;
            }

            // Call need_data if there's a lack of frames
            g_signal_connect(
                appsrc_element,
                "need-data",
                G_CALLBACK(need_data),
                user_data
            );

            // Get the required references => Clear then afterwards
            g_object_unref(appsrc_element);
            g_object_unref(pipeline);
        }

        // Periodically clean unused rtsp sessions
        static gboolean cleanup_sessions(gpointer user_data) {
            auto* server = GST_RTSP_SERVER(user_data);

            GstRTSPSessionPool* pool = gst_rtsp_server_get_session_pool(server);
            gst_rtsp_session_pool_cleanup(pool);
            g_object_unref(pool);

            return G_SOURCE_CONTINUE;
        }

        // Wird aufgerufen, wenn appsrc neue Daten benötigt.
        static void need_data(GstAppSrc* src, guint /* length */, gpointer user_data) {
            auto* state = static_cast<ImageConfiguration*>(user_data);

            cv::Mat frame;

            // Das Kamerabild einlesen. OpenCV liefert hier üblicherweise BGR.
            if (!state->camera.read(frame) || frame.empty()) {
                g_warning("Kein Kamerabild erhalten");
                return;
            }

            // Die Pipeline-Caps erwarten eine feste Bildgröße.
            // Für einen einfachen Test skalieren wir abweichende Größen passend.
            if (frame.cols != state->width || frame.rows != state->height) {
                cv::resize(frame, frame, cv::Size(state->width, state->height));
            }

            // Diese Pipeline erwartet BGR mit 8 Bit und 3 Kanälen.
            if (frame.type() != CV_8UC3) {
                g_warning("Unerwartetes OpenCV-Format: erwartet wird CV_8UC3/BGR");
                return;
            }

            const gsize row_bytes = static_cast<gsize>(state->width) * 3;
            const gsize buffer_size = row_bytes * state->height;

            // Einen GStreamer-Puffer für ein komplettes Bild anlegen.
            GstBuffer* buffer = gst_buffer_new_allocate(nullptr, buffer_size, nullptr);
            if (!buffer) {
                g_warning("GStreamer-Puffer konnte nicht angelegt werden");
                return;
            }

            // Das Bild zeilenweise kopieren. So funktioniert es auch, wenn die
            // cv::Mat-Zeilen wegen eines Ausschnitts nicht direkt hintereinanderliegen.
            GstMapInfo map;
            if (!gst_buffer_map(buffer, &map, GST_MAP_WRITE)) {
                gst_buffer_unref(buffer);
                g_warning("GStreamer-Puffer konnte nicht beschrieben werden");
                return;
            }

            for (int y = 0; y < state->height; ++y) {
                std::memcpy(
                    map.data + static_cast<gsize>(y) * row_bytes,
                    frame.ptr(y),
                    row_bytes
                );
            }

            gst_buffer_unmap(buffer, &map);

            // appsrc versieht den Puffer anhand seiner laufenden Uhr mit einem
            // Zeitstempel, weil in der Pipeline do-timestamp=true gesetzt ist.
            const GstFlowReturn result = gst_app_src_push_buffer(src, buffer);

            // gst_app_src_push_buffer übernimmt den Puffer auch bei einem Fehler.
            if (result != GST_FLOW_OK) {
                g_warning("Fehler beim Übergeben des Bildes an appsrc: %s",
                        gst_flow_get_name(result));
            }
        }

};

// Start the node
int main(int argc, char **argv) {
    rclcpp::init(argc, argv);
    auto node = std::make_shared<CameraStreamNode>();
    rclcpp::spin(node);
    rclcpp::shutdown();
    return 0;
}
