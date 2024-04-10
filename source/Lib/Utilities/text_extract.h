// ============
// SJTU SCC
// ============

#include <vector>
#include <map>
#include <opencv2/opencv.hpp>
#include "displacement.h"
#include "text_optimization_main.h"

#ifndef Text_Detect
#define Text_Detect


class TextDetect
{
public:
  // select from totalBoxes
  std::vector<vtm::BoundingBox> realBoxes;

private:
  // total text row boxes: 4 parameter (left, top, right, bottom)
  std::vector<vtm::BoundingBox> totalBoxes;
  // matrix
  cv::Mat yuvRow, edgeRow;
  // top, bottom, left, right of inital row box
  int topLine, bottomLine, leftLine, rightLine;
  // left, right of revised row box
  int leftBox, rightBox;
  // if box if large
  int largeAreaFlag;
  int projectAxis;

protected:
  double getMainColorRatio(cv::Mat yuvBlock);
  std::vector<vtm::SplitLine> getSplitLine(cv::Mat projection, const int binaryThres = 1 * 255);
  std::vector<vtm::SplitLine> project(cv::Mat imgEdge);
  void secondHorProjection();
  void secondProjection();
  void getRowBox(std::vector<vtm::SplitLine> splitLinesInCol, const int intervalThres = 30); // 30 / 50

public:
  void getTextRow(
    cv::Mat yuvEdgeMap, cv::Mat yuvOrg, const int yuvOrgHgt,
    const float HgtThresMax = 0.05, const float HgtThresMin = 0.005, const float HgtThresForLaf = 0.04,
    const int aspectRatioMin = 2, const int aspectRatioMax = 100, const int boxNumThres = 5);
};


class PriorInfo
{
public:
  std::vector<vtm::BoundingBox> bgColorBoxes;
  int bgColor[3];
  std::vector<int> paragraphs;

protected:
  int getMainColor(cv::Mat yuvBlock);

public:
  void getBgColor(cv::Mat img, const std::vector<vtm::BoundingBox> rowBoxes);
  void identifyParagraph(
    cv::Mat imgEdge, const int xThres = 10, const float lhRelTol = 0.6, const float loRelTol = 0.7,
    const float wThres = 0.6, const float rbhThres = 0.45, const float rbwThres = 0.55);
};


class CharacterSegmentation
{
public:
  // total character boxes: (left, top, right, bottom)
  std::vector<vtm::BoundingBox> totalCharBoxes;

private:
  cv::Mat imgY, imgU, imgV;
  int imgHgt;
  int imgWdt;

protected:
  std::vector<vtm::BoundingBox> onerowSegment(
    vtm::BoundingBox rowBox, const int bgY,
    const int YThres = 30, const int eOffset1 = 3, const int eOffset2 = 5, const int charOffset = 2, const float charThres = 0.6);

public:
  void charSegment(
    cv::Mat yOrg, const std::vector<vtm::BoundingBox> rowBoxes, const std::vector<int> paras, const int* bgColor, DisplacementParameterSet* dps);
};

#endif