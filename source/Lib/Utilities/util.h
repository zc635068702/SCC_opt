// ============
// SJTU SCC
// ============

#include <opencv2/opencv.hpp>

#ifndef Utility
#define Utility

cv::Mat getEdgeMap(
  cv::Mat inputImage,
  const int thresholdMin = 300, const int thresholdMax = 300,
  const cv::Size dsize = cv::Size(10, 1));

#endif