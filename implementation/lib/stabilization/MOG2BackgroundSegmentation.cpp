#include <opencv2/cudawarping.hpp>
#include <utility>
#include "BackgroundSegmentionBase.hpp"
#include "MOG2BackgroundSegmentation.hpp"
#include "opencv2/cudaarithm.hpp"
#include "opencv2/cudabgsegm.hpp"

namespace providentia {
	namespace stabilization {
		namespace segmentation {
			MOG2BackgroundSegmention::MOG2BackgroundSegmention(cv::Size calculationSize,
															   int history,
															   double varThreshold,
															   bool detectShadows)
				: BackgroundSegmentionBase(std::move(calculationSize)) {
				algorithm = cv::cuda::createBackgroundSubtractorMOG2(history, varThreshold, detectShadows);
				addFilters();
			}

			void MOG2BackgroundSegmention::specificApply() {
				algorithm->apply(calculationFrame, foregroundMask, -1, stream);
			}

			void MOG2BackgroundSegmention::addFilters() {
				BackgroundSegmentionBase::addFilters();

//    filters.emplace_back(cv::cuda::createMorphologyFilter(cv::MORPH_OPEN, CV_8UC1,
//                                                          cv::getStructuringElement(cv::MORPH_RECT,
//                                                                                    cv::Size(3, 3))));
//    filters.emplace_back(
//            cv::cuda::createMorphologyFilter(cv::MORPH_DILATE, CV_8UC1,
//                                             cv::getStructuringElement(cv::MORPH_RECT,
//                                                                       cv::Size(5, 5)), cv::Point(-1, -1), 3
//            ));
				filters.emplace_back(
					cv::cuda::createMorphologyFilter(cv::MORPH_ERODE, CV_8UC1,
													 cv::getStructuringElement(cv::MORPH_RECT,
																			   cv::Size(3, 3))
					));
//    filters.emplace_back(
//            cv::cuda::createMorphologyFilter(cv::MORPH_DILATE, CV_8UC1,
//                                             cv::getStructuringElement(cv::MORPH_RECT,
//                                                                       cv::Size(3, 3)), cv::Point(-1, -1), 5
//            ));
				filters.emplace_back(
					cv::cuda::createMorphologyFilter(cv::MORPH_DILATE, CV_8UC1,
													 cv::getStructuringElement(cv::MORPH_RECT,
																			   cv::Size(5, 5)), cv::Point(-1, -1), 3
					));
			}
		}
	}
}
