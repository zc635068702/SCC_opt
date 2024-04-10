// ============
// SJTU SCC
// ============

#include "util.h"

using namespace cv;
using namespace std;

Mat getEdgeMap(
  Mat inputImage,
  const int thresholdMin, const int thresholdMax,
  const Size dsize)
{
  Mat cannyImage, edgeImage;

  // Canny
  Canny(inputImage, cannyImage, thresholdMin, thresholdMax);
  // close operation to connect charaters
  const int elementShape = MORPH_RECT;
  const Mat kernel = getStructuringElement(elementShape, dsize, Point(-1, -1));
  morphologyEx(cannyImage, edgeImage, MORPH_CLOSE, kernel);

  return edgeImage;
}
