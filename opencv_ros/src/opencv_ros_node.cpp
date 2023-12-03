#include <ros/ros.h>
#include <image_transport/image_transport.h>
#include <cv_bridge/cv_bridge.h>
#include <sensor_msgs/image_encodings.h>
#include <opencv2/imgproc/imgproc.hpp>
#include <opencv2/highgui/highgui.hpp>
#include <opencv2/features2d.hpp>

//the library features2d implements the keypoints
using namespace cv;

static const std::string OPENCV_WINDOW = "Image window";

class ImageConverter
{
  ros::NodeHandle nh_;
  image_transport::ImageTransport it_;
  image_transport::Subscriber image_sub_;
  image_transport::Publisher image_pub_;

public:
  ImageConverter()
    : it_(nh_)
  {
    // Subscribe to input video feed and publish output video feed
    image_sub_ = it_.subscribe("/iiwa/camera1/image_raw", 1,
      &ImageConverter::imageCb, this);
    image_pub_ = it_.advertise("/image_converter/output_video", 1);

    cv::namedWindow(OPENCV_WINDOW);
  }

  ~ImageConverter()
  {
    cv::destroyWindow(OPENCV_WINDOW);
  }

  void imageCb(const sensor_msgs::ImageConstPtr& msg)
  {
    cv_bridge::CvImagePtr cv_ptr;
    try
    {
      cv_ptr = cv_bridge::toCvCopy(msg, sensor_msgs::image_encodings::BGR8);
    }
    catch (cv_bridge::Exception& e)
    {
      ROS_ERROR("cv_bridge exception: %s", e.what());
      return;
    }

    //detecting based on circle (BLOB)
    //i need to define the MAT format for pixel matrix
    Mat gray_image;
    //to detect it i use a scale of gray
    cvtColor(cv_ptr->image, gray_image,cv::COLOR_BGR2GRAY);

    //Parameter configuration for the blobdetector
    SimpleBlobDetector::Params params;
    params.minThreshold=0;
    params.maxThreshold=255;
    //to detect a circular shape i set to true the filter
    params.filterByCircularity=true;
    params.minCircularity=0;
    params.maxCircularity=1;

    //Detector Creation
    Ptr<SimpleBlobDetector> detector = SimpleBlobDetector::create(params);

    // Blob detection
    // if i detect a circle i save the points in keypoints
    std::vector<cv::KeyPoint> keypoints;
    detector->detect(gray_image, keypoints);

    //Draw the circle on the video stream
    //the first argument is where i want to draw, the second is what i want to draw, the third is where i want to save, the fourth is the color, the fifth is to make it continuous
    drawKeypoints(cv_ptr->image, keypoints, cv_ptr->image, cv::Scalar(0, 0, 255), DrawMatchesFlags::DRAW_RICH_KEYPOINTS);

    // // Draw an example circle on the video stream
    // if (cv_ptr->image.rows > 60 && cv_ptr->image.cols > 60)
    //   cv::circle(cv_ptr->image, cv::Point(50, 50), 10, CV_RGB(255,0,0));

    // Update GUI Window
    imshow(OPENCV_WINDOW, cv_ptr->image);
    waitKey(3);

    // Output modified video stream
    image_pub_.publish(cv_ptr->toImageMsg());
  }
};

int main(int argc, char** argv)
{
  ros::init(argc, argv, "image_converter");
  ImageConverter ic;
  ros::spin();
  return 0;
}