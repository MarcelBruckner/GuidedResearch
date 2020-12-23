#include <stdio.h>
#include <iostream>
#include "lib/CameraStabilization/CameraStabilization.hpp"
#include "lib/ImageUtils/ImageUtils.hpp"
#include "opencv2/highgui/highgui.hpp"
#include "opencv2/imgproc/imgproc.hpp"
#include "opencv2/videoio.hpp"
#include <opencv2/video.hpp>
#include "opencv2/imgcodecs.hpp"
#include "opencv2/features2d.hpp"
#include "opencv2/xfeatures2d.hpp"
#include "opencv2/xfeatures2d/cuda.hpp"
#include <opencv2/cudafeatures2d.hpp>
#include "opencv2/calib3d.hpp"
#include <chrono>
#include <thread>
#include <fstream>

#include "DynamicCalibration.hpp"
#include "OpticalFlow.h"
#include "BackgroundSegmentation.h"

std::string durationInfo(const std::string &name, long milliseconds) {
    std::stringstream ss;
    ss << name << "- Duration: " << milliseconds << "ms - FPS: " << 1000. / milliseconds;
    return ss.str();
}

void addText(cv::Mat &frame, std::string text, int x, int y) {
    cv::putText(frame, text, cv::Point(x, y),
                cv::FONT_HERSHEY_COMPLEX_SMALL, 1, cv::Scalar(255, 255, 0), 2, cv::FONT_HERSHEY_SIMPLEX);
}

cv::Mat pad(const cv::Mat &frame, int padding) {
    return cv::Mat(frame,
                   cv::Rect(padding, padding, frame.cols - 2 * padding, frame.rows - 2 * padding));
}

int main(int argc, char const *argv[]) {
    cv::cuda::Stream cudaStream;
    std::string basePath = "/mnt/local_data/providentia/test_recordings/videos/";
    // cv::VideoCapture cap("/mnt/local_data/providentia/test_recordings/videos/s40_n_far_image_raw.mp4");
    std::string filename = "s40_n_far_image_raw";
    std::string suffix = ".mp4";
    cv::VideoCapture cap(basePath + filename + suffix);
    if (!cap.isOpened()) // if not success, exit program
    {
        std::cout << "Cannot open the video." << std::endl;
        return -1;
    }

    cv::Mat frame;
    providentia::calibration::dynamic::ExtendedSurfBFDynamicCalibrator calibrator(
            1000, cv::NORM_L2, 0
    );

    int padding = 10;

    std::string windowName = "Dynamic Camera Stabilization";
    cv::namedWindow(windowName, cv::WINDOW_AUTOSIZE);

    double calculationScaleFactor = 1;
    double renderingScaleFactor = 0.5;
    renderingScaleFactor /= calculationScaleFactor;

    auto start = providentia::utils::TimeMeasurable::now().count();

    while (true) {
        cap >> frame;
        if (frame.empty()) {
            break;
        }
        cv::resize(frame, frame, cv::Size(), calculationScaleFactor, calculationScaleFactor);
        cv::Mat originalFrame = frame.clone();
        cv::cuda::GpuMat gpu_frame;
        gpu_frame.upload(frame);

        calibrator.stabilize(gpu_frame);
        cv::Mat stabilized = cv::Mat(calibrator.getStabilizedFrame());

        stabilized = pad(stabilized, padding);
        std::cout << calibrator.durations_str() << std::endl;
        originalFrame = cv::Mat(originalFrame, cv::Rect(padding, padding, originalFrame.cols - 2 * padding,
                                                        originalFrame.rows - 2 * padding));
        cv::Mat finalFrame;

        cv::Mat colorFrames;
        cv::hconcat(std::vector<cv::Mat>{originalFrame, stabilized}, colorFrames);

        cv::Mat calibratorFrames;
        cv::Mat referenceFrame = cv::Mat(calibrator.getReferenceFrame());
        cv::cvtColor(referenceFrame, referenceFrame, cv::COLOR_GRAY2BGR);
        cv::hconcat(std::vector<cv::Mat>{stabilized, pad(referenceFrame, padding)},
                    calibratorFrames);
        cv::Mat calibratorMasks;
        cv::hconcat(std::vector<cv::Mat>{pad(cv::Mat(calibrator.getLatestMask()), padding),
                                         pad(cv::Mat(calibrator.getReferenceMask()), padding)},
                    calibratorMasks);
        cv::cvtColor(calibratorMasks, calibratorMasks, cv::COLOR_GRAY2BGR);

        auto now = providentia::utils::TimeMeasurable::now().count();
//        magnitudeCsv << now << "," << now - start << "," << opticalFlow_original.getMagnitudeMean() << ","
//                     << opticalFlow_stabilized.getMagnitudeMean() << std::endl;

        cv::vconcat(std::vector<cv::Mat>{calibratorFrames, calibratorMasks}, finalFrame);
//        cv::vconcat(std::vector<cv::Mat>{colorFrames,}, finalFrame);

//        finalFrame = calibrator.draw();

        cv::resize(finalFrame, finalFrame, cv::Size(), renderingScaleFactor, renderingScaleFactor);

        addText(finalFrame, durationInfo("Calibration", calibrator.getTotalMilliseconds()), 10, 30);
        addText(finalFrame, durationInfo("Total",
                                         calibrator.getTotalMilliseconds()), 10, 120);


        cv::imshow(windowName, finalFrame);
        // cv::imshow(matchingWindowName, flannMatching);

        if ((char) cv::waitKey(1) == 27) {
            break;
        }
    }

    cap.release();
    cv::destroyAllWindows();

    return 0;
}
