#include "rclcpp/rclcpp.hpp"
#include "opencv2/opencv.hpp"
#include "iostream"

class CameraStreamNode : public rclcpp::Node {
    public:
        CameraStreamNode() : Node("camera_stream_node") {
            //stream_timer_ = this->create_wall_timer(std::chrono::milliseconds(1), std::bind(&CameraStreamNode::stream_camera_image, this));
            // Call the function once and start the stream
            CameraStreamNode::stream_camera_image();
        }
    private:
        // Main function getting the camera image and streaming it via the RTSP protocoll
        void stream_camera_image() {
            // TODO: Get the current image
            // Connect to a certain camera TODO: Support any camera by receiving the id via a topic
            int id = 0;
            cv::VideoCapture camera(id);
            if (!camera.isOpened()) {
                RCLCPP_FATAL(this->get_logger(), "Unable to connect to the camera with the id '%i'", id);
                return;
            }

            // TODO: Remove this once it works
            cv::namedWindow("Test the camera node", cv::WINDOW_AUTOSIZE);

            // Open the connection and store a frame
            cv::Mat frame;

            while (true) {
                // Get the next frame
                camera >> frame;

                // show the image on the window
                cv::imshow("Webcam", frame);
                // wait (10ms) for a key to be pressed
                if (cv::waitKey(10) >= 0)
                    break;
            }
            
            
            
            // TODO: stream the image via RTSP
        }
        rclcpp::TimerBase::SharedPtr stream_timer_; // Timer which starts the stream
};

// Start the node
int main(int argc, char **argv) {
    rclcpp::init(argc, argv);
    auto node = std::make_shared<CameraStreamNode>();
    rclcpp::spin(node);
    rclcpp::shutdown();
    return 0;
}
