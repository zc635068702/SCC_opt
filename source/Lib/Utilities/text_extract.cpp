// ============
// SJTU SCC
// ============

#include "text_extract.h"
#include  <algorithm>

using namespace cv;
using namespace std;


Mat calcuColorHistogram(Mat img)
{
  /* convert pixel to 32F, then compute histogram */
  // 8U to 32
  vector<Mat> bgrPlanes;
  split(img, bgrPlanes);
  Mat bUint32, gUint32, rUint32;
  bgrPlanes[0].convertTo(bUint32, CV_32F, 1, 0);
  bgrPlanes[1].convertTo(gUint32, CV_32F, 256, 0);
  bgrPlanes[2].convertTo(rUint32, CV_32F, 65536, 0);
  Mat bgrUint32 = bUint32 + gUint32 + rUint32;

  // histogram
  float hisMax = 255 * (1 + 256 + 65536) + 1; // 16777215
  // parameters for hist
  int histSize[1] = { (int)hisMax + 1 };
  float range[] = { 0, hisMax };
  const float* ranges[] = { range };
  Mat hist;
  calcHist(&bgrUint32, 1, 0, Mat(), hist, 1, histSize, ranges, true, false);

  return hist;
}



double TextDetect::getMainColorRatio(Mat yuvBlock)
{
  /*
  // 4x downsampling
  Mat imgResize;
  double fx = 0.25, fy = 0.25;
  if (imgResize.rows < 4)
      fx = 1;
  if (imgResize.cols < 4)
      fy = 1;
  resize(yuvBlock, imgResize, Size(), fx, fy, INTER_NEAREST);
  float imgSize = imgResize.rows * imgResize.cols;
  */

  Mat hist = calcuColorHistogram(yuvBlock);

  // find main two color
  double mainColorNum1, mainColorNum2;
  Point maxLoc;
  minMaxLoc(hist, 0, &mainColorNum1, 0, &maxLoc);
  hist.at<float>(maxLoc) = 0;
  minMaxLoc(hist, 0, &mainColorNum2, 0, 0);

  double mainColorCount = mainColorNum1 + mainColorNum2;
  int imgSize = yuvBlock.rows * yuvBlock.cols;
  double mainColorRatio = mainColorCount / (double) imgSize;

  return mainColorRatio;
}

// find boundary lines of text boxes from projection
std::vector<vtm::SplitLine> TextDetect::getSplitLine(Mat projection, const int binaryThres)
{
  // binary
  Mat projectionBinary, projectionBinaryUint32;
  threshold(projection, projectionBinaryUint32, binaryThres, 255, cv::THRESH_BINARY);
  projectionBinaryUint32.convertTo(projectionBinary, CV_8U, 1, 0);

  // find boundary lines by connectedComponents
  Mat labels, stats, centroids;
  int numLabels = connectedComponentsWithStats(projectionBinary, labels, stats, centroids);

  vector<vtm::SplitLine> splitLines;
  int startCor, interval;
  for (int idx = 1; idx < numLabels; idx++)
  {
    if (projectAxis == 1)
    {
      startCor = stats.at<int>(idx, CC_STAT_TOP);
      interval = stats.at<int>(idx, CC_STAT_HEIGHT);
    }
    else
    {
      startCor = stats.at<int>(idx, CC_STAT_LEFT);
      interval = stats.at<int>(idx, CC_STAT_WIDTH);
    }
    splitLines.push_back(vtm::SplitLine(startCor, startCor + interval));
  }

  return splitLines;
}

std::vector<vtm::SplitLine> TextDetect::project(Mat EdgeMap)
{
  // projection by reduce()
  Mat projection;
  reduce(EdgeMap, projection, projectAxis, REDUCE_SUM, CV_32F);
  // split image as rows or columns
  vector<vtm::SplitLine> splitLines = getSplitLine(projection);

  return splitLines;
}

void TextDetect::secondHorProjection()
{
  Mat edgeBlock = edgeRow(Range::all(), Range(leftBox, rightBox));
  // second horizontal projection
  projectAxis = 1;
  vector<vtm::SplitLine> splitLinesInRow = project(edgeBlock);

  for (auto splitLine : splitLinesInRow)
  {
    totalBoxes.push_back(vtm::BoundingBox(
      leftBox, topLine + splitLine.cor1, rightBox, topLine + splitLine.cor2));
  }
}

void TextDetect::secondProjection()
{
  if (largeAreaFlag)
    // second horizontal projection to find text boxes
    secondHorProjection();
  else
  {
    // directly add boxes by first horizontal & vertical projection
    totalBoxes.push_back(vtm::BoundingBox(
      leftBox, topLine, rightBox, bottomLine));
  }
}

void TextDetect::getRowBox(vector<vtm::SplitLine> splitLinesInCol, const int intervalThres)
{
  // threshold of main color ratio
  double colorRatioThres = 0;
  if (largeAreaFlag)
    colorRatioThres = 0.4;

  double mainColorRatio = 0;
  bool firstBoxFlag = 1;
  leftBox = -1;
  // for each vertical projection region
  for (auto splitLine : splitLinesInCol)
  {
    // left, right of initial box
    leftLine = splitLine.cor1;
    rightLine = splitLine.cor2;
    if (largeAreaFlag)
      mainColorRatio = getMainColorRatio(yuvRow(Range::all(), Range(leftLine, rightLine)));

    // combine adjacent boxes with few colors
    if (mainColorRatio >= colorRatioThres)
    {
      if (firstBoxFlag)  // get position of first box
      {
        // leftBox: left of box
        // rightBox: right of box
        leftBox = leftLine;
        rightBox = rightLine;
        firstBoxFlag = 0;
      }
      // if current box far away from the previous one
      // second projection of the previous box
      if ((leftLine - rightBox) >= intervalThres)
      {
        secondProjection();
        // update left
        leftBox = leftLine;
      }

      // update right
      rightBox = rightLine;
    }
  }

  // last box
  if (leftBox != -1)
  {
    if (largeAreaFlag)
      mainColorRatio = getMainColorRatio(yuvRow(Range::all(), Range(leftBox, rightBox)));
    if (mainColorRatio >= colorRatioThres)
      secondProjection();
  }
}

void TextDetect::getTextRow(
  Mat yuvEdgeMap, Mat yuvOrg, const int yuvOrgHgt,
  const float HgtThresMax, const float HgtThresMin, const float HgtThresForLaf,
  const int aspectRatioMin, const int aspectRatioMax, const int boxNumThres)
{
  // height, width, aspect_ratio of text row
  int TextRowHgt, TextRowWdt, TextRowRatio;
  // parameters for vertical projection
  vector<vtm::SplitLine> splitLinesInCol;
  size_t num_of_split_line;

  // first horizontal projection
  projectAxis = 1;
  vector<vtm::SplitLine> splitLinesInRow = project(yuvEdgeMap);

  // for each horizontal projection region
  for (auto splitLine : splitLinesInRow)
  {
    // top, bottom
    topLine = splitLine.cor1;
    bottomLine = splitLine.cor2;
    yuvRow = yuvOrg(Range(topLine, bottomLine), Range::all());
    edgeRow = yuvEdgeMap(Range(topLine, bottomLine), Range::all());

    // vertical projection
    projectAxis = 0;
    splitLinesInCol = project(edgeRow);

    num_of_split_line = splitLinesInCol.size();
    if (num_of_split_line > 0)
    {
      // left, right
      leftLine = splitLinesInCol[0].cor1;
      rightLine = splitLinesInCol[num_of_split_line - 1].cor2;
      // height, width, TextRowRatio
      TextRowHgt = bottomLine - topLine;
      TextRowWdt = rightLine - leftLine;
      TextRowRatio = TextRowWdt / TextRowHgt;

      // for short text lines
      if (TextRowHgt <= HgtThresForLaf * yuvOrgHgt && TextRowRatio >= aspectRatioMin)
        largeAreaFlag = 0;
      // for large regions
      else
        largeAreaFlag = 1;
      getRowBox(splitLinesInCol);
    }
  }

  // delete boxes that are too big or small
  for (auto box : totalBoxes)
  {
    TextRowHgt = box.bottom - box.top;
    TextRowWdt = box.right - box.left;
    TextRowRatio = TextRowWdt / TextRowHgt;

    if (TextRowHgt <= HgtThresMax * yuvOrgHgt
      && TextRowHgt >= HgtThresMin * yuvOrgHgt
      && TextRowRatio >= aspectRatioMin
      && TextRowRatio <= aspectRatioMax)
      realBoxes.push_back(box);
  }

  if (realBoxes.size() < boxNumThres)
    realBoxes.clear();

#if 0
  cout << "Text Row Box Number (detected by edge & color): " << totalBoxes.size() << endl;
#endif
}



int PriorInfo::getMainColor(Mat yuvBlock)
{
  Mat imgResize;
  resize(yuvBlock, imgResize, Size(), 0.25, 0.25, INTER_NEAREST);
  Mat hist = calcuColorHistogram(imgResize);

  // find main color
  Point maxLoc;
  minMaxLoc(hist, 0, 0, 0, &maxLoc);

  return maxLoc.y;
}

void PriorInfo::getBgColor(Mat img, const std::vector<vtm::BoundingBox> rowBoxes)
{
  // box location & area
  int x0, y0, x1, y1, rowArea;
  // dictionary: key: color, value: area
  map<int, int> colorDict{};
  vector<int> rowColors;
  // dominateColor of one block or one frame
  int dominateColor = 0;

  for (auto box : rowBoxes)
  {
    x0 = box.left;
    y0 = box.top;
    x1 = box.right;
    y1 = box.bottom;
    rowArea = (x1 - x0) * (y1 - y0);
    dominateColor = getMainColor(img(Range(y0, y1), Range(x0, x1)));
    colorDict[dominateColor] += rowArea;
    rowColors.push_back(dominateColor);
  }

  // background_color: largest area
  int bgColorArea = 0;
  auto map = colorDict.begin();
  while (map != colorDict.end())
  {
    if (map->second > bgColorArea)
    {
      dominateColor = map->first;
      bgColorArea = map->second;
    }
    map++;
  }
  bgColor[0] = dominateColor % 256;
  bgColor[1] = (dominateColor / 256) % 256;
  bgColor[2] = dominateColor / 65536;

  // only keep rowBoxes in background color
  for (int rowIdx = 0; rowIdx < rowColors.size(); rowIdx++)
  {
    if (rowColors[rowIdx] == dominateColor)
      bgColorBoxes.push_back(rowBoxes[rowIdx]);
  }
}

void PriorInfo::identifyParagraph(
  Mat imgEdge, const int xThres, const float lhRelTol, const float loRelTol,
  const float wThres, const float rbhThres, const float rbwThres)
{
  /*
  identify paragraph from rowBoxes.
  layout analysis task, but simple and heuristic for text paragraph.
  return: paragraphs, each paragraph is a list of rowBox index.
  */
  const size_t lenOfRowBox = bgColorBoxes.size();
  int *visited = new int[lenOfRowBox];
  int *unionSet = new int[lenOfRowBox];
  for (int k = 0; k < lenOfRowBox; k++)
  {
    visited[k] = 0;
    unionSet[k] = 0;
  }

  // for each row, find a first overlap
  // then judge if the two belongs to one para
  int iboxX0, iboxY0, iboxX1, iboxY1;
  int jboxX0, jboxY0, jboxX1, jboxY1;
  int jboxYdiff, iboxYdiff, ijboxYdiff;
  float max1, max2;
  for (int i = 0; i < lenOfRowBox; i++)
  {
    iboxX0 = bgColorBoxes[i].left;
    iboxY0 = bgColorBoxes[i].top;
    iboxX1 = bgColorBoxes[i].right;
    iboxY1 = bgColorBoxes[i].bottom;

    for (int j = 0; j < lenOfRowBox; j++)
    {
      jboxX0 = bgColorBoxes[j].left;
      jboxY0 = bgColorBoxes[j].top;
      jboxX1 = bgColorBoxes[j].right;
      jboxY1 = bgColorBoxes[j].bottom;

      if (visited[j] || iboxX0 > (jboxX1 + xThres) || iboxX1 < (jboxX0 - xThres))
        continue;

      // previous line_height vs current line_height
      // previous line_height + line_offset vs current line_height
      jboxYdiff = jboxY1 - jboxY0;
      iboxYdiff = iboxY1 - iboxY0;
      ijboxYdiff = jboxY0 - iboxY0;
      if (jboxYdiff >= iboxYdiff)
        max1 = (float)jboxYdiff;
      else
        max1 = (float)iboxYdiff;
      if (ijboxYdiff >= iboxYdiff)
        max2 = (float)ijboxYdiff;
      else
        max2 = (float)iboxYdiff;
      if (abs(jboxYdiff - iboxYdiff) <= max1 * lhRelTol
        && abs(ijboxYdiff - iboxYdiff) <= max2 * loRelTol)
      {
        unionSet[i] = j;
        visited[j] = 1;
        break;
      }
    }
  }

  // memset(visited, 0, sizeof(visited));
  for (int k = 0; k < lenOfRowBox; k++)
  {
    visited[k] = 0;
  }
  // related rows are united into one paragraph
  int v;
  for (int k = 0; k < lenOfRowBox; k++)
  {
    if (visited[k] == 0)
    {
      paragraphs.push_back(k);
      visited[k] = 1;
      v = unionSet[k];
      while (v != 0)
      {
        paragraphs.push_back(v);
        visited[v] = 1;
        v = unionSet[v];
      }
    }
  }
  delete[] visited;
  delete[] unionSet;

#if SELECT_TEXT_CODEC == 1
  // If TextSCC process or not by rowbox height & width statistic"""
  vector<int> rowBoxHgt;
  vector<int> rowBoxWdt;
  Mat edgeRow, projection;
  int h = 0, w = 0, left = 0, right = 0;
  for (auto box : bgColorBoxes)
  {
    // height
    h = box.bottom - box.top;
    rowBoxHgt.push_back(h);
    // width
    // rough character location from edge map
    edgeRow = imgEdge(Range(box.top, box.bottom), Range(box.left, box.right));
    // vertical projection
    reduce(edgeRow, projection, 0, REDUCE_MAX, CV_8U);
    // left of first character
    for (int i = 0; i < projection.cols; i++)
    {
      if (projection.at<uint8_t>(i) != 0)
      {
        left = i;
        break;
      }
    }
    // right of last character
    for (int i = projection.cols - 1; i >= 0; i--)
    {
      if (projection.at<uint8_t>(i) != 0)
      {
        right = i;
        break;
      }
    }
    w = right - left;
    rowBoxWdt.push_back(w);
  }

  size_t rowNum = bgColorBoxes.size();

  // number of differential rowbox height
  int diffHgtNum = 0;
  for (int rowIdx = 1; rowIdx < rowNum; rowIdx++)
  {
    if (rowBoxHgt[rowIdx] != rowBoxHgt[rowIdx - 1])
      diffHgtNum += 1;
  }
  float diffHgtRatio = (float)diffHgtNum / (float)rowNum;

  // number of small rowbox width
  int smallWdtNum = 0;
  float maxWdt = *max_element(rowBoxWdt.begin(), rowBoxWdt.end());
  for (auto w : rowBoxWdt)
  {
    if (w < wThres * maxWdt)
      smallWdtNum += 1;
  }
  float smallWdtRatio = (float)smallWdtNum / (float)rowNum;

  // rbhThres & rbwThres: threshold of rowbox height & width
  if (diffHgtRatio >= rbhThres && smallWdtRatio >= rbwThres)
    paragraphs.clear();

#if 1
  cout << "diffHgtRatio & smallWdtRatio: " << diffHgtRatio << " " << smallWdtRatio << endl;
#endif
#endif
}



std::vector<vtm::BoundingBox> CharacterSegmentation::onerowSegment(
  vtm::BoundingBox rowBox, const int bgY,
  const int YThres, const int ec_offset1, const int ec_offset2, const int charOffset, const float charThres)
{
  /* calculate connected components to split char in one row */
  // Step1: process image: error correction, gray, binary
  // error correction: correct row position obtained from OCR
  // initial coordinate of row box
  int rowX0Init = rowBox.left;
  int rowY0Init = rowBox.top;
  int rowX1Init = rowBox.right;
  int rowY1Init = rowBox.bottom;
  // coordinate with offset of row box
  int rowX0Offset1 = max(rowX0Init - ec_offset1, 0);
  int rowX1Offset1 = min(rowX1Init + ec_offset1, imgWdt);
  int rowY0Offset1 = max(rowY0Init - ec_offset1, 0);
  int rowY1Offset1 = min(rowY1Init + ec_offset1, imgHgt);
  int rowX0Offset2 = rowX0Init + ec_offset2;
  int rowX1Offset2 = rowX1Init - ec_offset2;
  int rowY0Offset2 = rowY0Init + ec_offset2;
  int rowY1Offset2 = rowY1Init - ec_offset2;
  // real coordinate after error correction
  int rowX0Real = 0, rowX1Real = 0, rowY0Real = 0, rowY1Real = 0;

  // matrix
  cv::Mat rowBoxImg, colorResidualImg, colorResidualImg8U, binaryMap, cropBinaryMap;
  // get row box regions
  rowBoxImg = imgY(Range(rowY0Offset1, rowY1Offset1), Range(rowX0Offset1, rowX1Offset1));
  // calculate color residual
  colorResidualImg = abs(rowBoxImg - bgY);
  colorResidualImg.convertTo(colorResidualImg8U, CV_8U, 1, 0);
  // binary by YThres: 0, background; 1, text
  threshold(colorResidualImg8U, binaryMap, YThres, 255, cv::THRESH_BINARY);

  // find optimal row position when complete character in box
  // from center to boundary, from offset2 to offset1
  int x0Error = 0, x1Error = 0, y0Error = 0, y1Error = 0;
  Scalar sumOfLine;
  for (rowX0Real = rowX0Offset2; rowX0Real > rowX0Offset1; rowX0Real--)
  {
    x0Error = rowX0Real - rowX0Offset1;
    // if no text pixels in line
    sumOfLine = sum(binaryMap(Range::all(), Range(x0Error, x0Error + 1)));
    if (sumOfLine[0] == 0)
      break;
  }
  for (rowX1Real = rowX1Offset2; rowX1Real < rowX1Offset1; rowX1Real++)
  {
    x1Error = rowX1Real - rowX0Offset1;
    sumOfLine = sum(binaryMap(Range::all(), Range(x1Error, x1Error + 1)));
    if (sumOfLine[0] == 0)
      break;
  }
  for (rowY0Real = rowY0Offset2; rowY0Real > rowY0Offset1; rowY0Real--)
  {
    y0Error = rowY0Real - rowY0Offset1;
    sumOfLine = sum(binaryMap(Range(y0Error, y0Error + 1), Range::all()));
    if (sumOfLine[0] == 0)
      break;
  }
  for (rowY1Real = rowY1Offset2; rowY1Real < rowY1Offset1; rowY1Real++)
  {
    y1Error = rowY1Real - rowY0Offset1;
    sumOfLine = sum(binaryMap(Range(y1Error, y1Error + 1), Range::all()));
    if (sumOfLine[0] == 0)
      break;
  }
  rowY0Real += 1;
  rowY1Real -= 1;

  // Step2: char segmentation by binary map
  // character boxes of one row
  std::vector<vtm::BoundingBox> charBoxes;
  // record intervals of characters
  std::vector<vector<int >> pass1CharIntervals, pass2CharIntervals, pass3CharIntervals;
  vector<int> interval, intervalNext;
  int interval0, interval1;

  if (y0Error < y1Error && x0Error < x1Error)
  {
    cropBinaryMap = binaryMap(Range(y0Error, y1Error), Range(x0Error, x1Error));
    // find connectedComponents
    Mat labels, stats, centroids;
    int numLabels = connectedComponentsWithStats(cropBinaryMap, labels, stats, centroids, 8);

    // pass1: initial char boxes by connectedComponents
    int x, w;
    for (int i = 1; i < numLabels; i++)
    {
      x = stats.at<int>(i, CC_STAT_LEFT);
      w = stats.at<int>(i, CC_STAT_WIDTH);
      interval.push_back(x);
      interval.push_back(x + w - 1);
      pass1CharIntervals.push_back(interval);
      interval.clear();
    }
    sort(pass1CharIntervals.begin(), pass1CharIntervals.end());

    // combine components in one char
    int leftCharBox = 0;
    int rightCharBox = 0;
    if (pass1CharIntervals.size() > 0)
    {
      // pass2: combine upper & lower components in one char
      interval.push_back(pass1CharIntervals[0][0]);
      interval.push_back(pass1CharIntervals[0][1]);
      pass2CharIntervals.push_back(interval);

      for (auto box : pass1CharIntervals)
      {
        // if the component not overlap with
        if (box[0] > interval[1] || box[1] < interval[0])
        {
          interval.clear();
          interval.push_back(box[0]);
          interval.push_back(box[1]);
          pass2CharIntervals.push_back(interval);
        }
        else
        {
          interval0 = interval[0];
          interval1 = interval[1];
          interval.clear();
          interval.push_back(min(interval0, box[0]));
          interval.push_back(max(interval1, box[1]));
          pass2CharIntervals.pop_back();
          pass2CharIntervals.push_back(interval);
        }
      }

      // pass3: combine left & right components in one char
      // list of char box width
      vector<int> boxWidthList;
      // get median box width
      for (auto interval : pass2CharIntervals)
      {
        boxWidthList.push_back(interval[1] - interval[0]);
      }
      float boxWidth = median(boxWidthList);

      // combine left & right components
      int leftBox = pass2CharIntervals[0][0];
      for (int boxIdx = 0; boxIdx < pass2CharIntervals.size(); boxIdx++)
      {
        interval = pass2CharIntervals[boxIdx];
        if (boxIdx < pass2CharIntervals.size() - 1)
        {
          intervalNext = pass2CharIntervals[boxIdx + 1];
          // if combined box are big enough
          // and far away from next box
          if (interval[1] - leftBox > boxWidth * charThres
            || intervalNext[0] - interval[1] > charOffset)
          {
            interval1 = interval[1];
            interval.clear();
            interval.push_back(leftBox);
            interval.push_back(interval1);
            pass3CharIntervals.push_back(interval);
            leftBox = intervalNext[0];
          }
        }
        else
        {
          interval1 = interval[1];
          interval.clear();
          interval.push_back(leftBox);
          interval.push_back(interval1);
          pass3CharIntervals.push_back(interval);
        }
      }

      for (auto interval : pass3CharIntervals)
      {
        leftCharBox = rowX0Real + interval[0];
        rightCharBox = rowX0Real + interval[1];

        charBoxes.push_back(vtm::BoundingBox(
          leftCharBox, rowY0Real, rightCharBox, rowY1Real));
      }
    }
  }

  return charBoxes;
}

// identify character from row boxes & get displacement parameters
void CharacterSegmentation::charSegment(
  Mat yOrg, const std::vector<vtm::BoundingBox> rowBoxes, const vector<int> paras, const int* bgColor, DisplacementParameterSet* dps)
{
  imgY = yOrg;
  imgHgt = yOrg.rows;
  imgWdt = yOrg.cols;

  // parameters for displacement
  dps->updateColor(bgColor);
  // Y value of bachground color
  int bgY = bgColor[0];

  // character segmentation for all rowBoxes
  std::vector<vtm::BoundingBox> charBoxes;
  for (int rowIdx : paras)
  {
    // character segmentation
    charBoxes = onerowSegment(rowBoxes[rowIdx], bgY);

    if (charBoxes.size() >= 1)
    {
      // get displacement parameters
      dps->update(charBoxes);

      for (auto box : charBoxes)
        totalCharBoxes.push_back(box);

      charBoxes.clear();
    }
  }
}
