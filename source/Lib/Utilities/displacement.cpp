// ============
// SJTU SCC
// ============

#include "displacement.h"
#include <math.h>

using namespace std;


float median(vector<int> vec)
{
  typedef vector<int>::size_type vec_sz;

  vec_sz size = vec.size();
  if (size == 0)
  {
    return 0;
    //throw domain_error("median of an empty vector");
  }
  sort(vec.begin(), vec.end());

  vec_sz mid = size/2;

  return size % 2 == 0 ? ((float)vec[mid] + (float)vec[mid-1])/2 : vec[mid];
}



int DisplacementParameterSet::computeMedianInterval(vector<int> array1, vector<int> array2)
{
  vector<int> intervalList;
  int medianInterval;
  int interval;

  size_t lenOfArray = array1.size();
  if (lenOfArray != 0)
  {
    for (int idx = 1; idx < lenOfArray; idx++)
    {
      // array1[l_idx] - array2[l_idx-1]
      interval = array1[idx] - array2[idx - 1];
      intervalList.push_back(interval);
    }

    medianInterval = (int)round(median(intervalList));
    intervalList.clear();
  }
  else
    medianInterval = 0;

  return medianInterval;
}

void DisplacementParameterSet::update(const std::vector<vtm::BoundingBox> charBoxes)
{
  // 1. box number in current row
  charBoxNum.push_back((int)charBoxes.size());

  // 2. top of first character
  topOfFirstChar.push_back(charBoxes[0].top);

  // 3. block_size of row
  // int rowHeight = charBoxes[0].bottom - charBoxes[0].top;
  int rowHeight = charBoxes[0].bottom - charBoxes[0].top + 1;
  charBoxHeight.push_back(rowHeight);

  // 4. box interval: median interval of boxes
  vector<int> lefts;
  vector<int> rights;
  // for box in charBoxes:
  for (auto box: charBoxes)
  {
    lefts.push_back(box.left);
    rights.push_back(box.right);
  }
  int intervalOfLeft = computeMedianInterval(lefts, lefts);
  intervalOfLeftOfChars.push_back(intervalOfLeft);

  // 5. left & right of character
  leftOfFirstChar.push_back(lefts[0]);
  rightOfLastChar.push_back(rights[rights.size() - 1]);

  // 6. bias width of first character:
  // real box interval - optimal box interval
  // left of previous character
  int LeftOfPreChar = lefts[0] - intervalOfLeft;
  int left, LeftOfChar, biasOfLeft;
  for (int boxIdx = 0; boxIdx < charBoxes.size(); boxIdx++)
  {
    left = lefts[boxIdx];
    LeftOfChar = LeftOfPreChar + intervalOfLeft;
    biasOfLeft = left - LeftOfChar;
    biasOfLeftOfChars.push_back(biasOfLeft);
    LeftOfPreChar = left;
  }
}


void bgColorFilling(Pel* text, const int textStride, const Pel bgColor, const int chShift, int textWdt, int textHgt)
{
  textHgt = textHgt >> chShift;
  textWdt = textWdt >> chShift;

  for (int y = 0; y < textHgt; y++)
  {
    // ::memset(text, (Pel)bgColor, textWdt * sizeof(Pel));
    for (int x = 0; x < textWdt; x++)
    {
      text[x] = bgColor;
    }
    text += textStride;
  }
}

void copyPelBuffer(const vtm::Area textArea, const vtm::Area bgArea, Pel* text, Pel* bg, const int textStride, const int bgStride, const int bgColor, const bool layerFlag)
{
  // start point
  text += textArea.x + textStride * textArea.y;
  bg += bgArea.x + bgStride * bgArea.y;

  for (uint32_t y = 0; y < bgArea.height; y++)
  {
    for (uint32_t x = 0; x < bgArea.width; x++)
    {
      // layerFlag == true: copy pixel from bg to text
      // layerFlag == false (stitch): copy pixel from text to bg
      if (layerFlag)
      {
        text[x] = bg[x];
        bg[x] = bgColor;
      }
      else
        bg[x] = text[x];
    }
    text += textStride;
    bg += bgStride;
  }
}


int judgeMaxPara(const int possiblePara, int finalPara, const int padSize)
{
  if (possiblePara > finalPara)
    finalPara = possiblePara;

  // padding para to multiple of padSize
  if (finalPara % padSize)
    finalPara += padSize - finalPara % padSize;

  return finalPara;
}


int getCoordinate(int corReal, int corOrg, int& corNonoverlap, const int orgSize, const int alignSize, const int chromaFormat, bool oddevenAlignFlag)
{
  // corDis1: align original coordinate of text box to multiple of alignSize
  int corDis1 = corOrg;
  if (corDis1 % alignSize)
  {
    corDis1 += alignSize - corOrg % alignSize;
  }
  // corDis2: coordinate that at least not overlap with previous text box
  int corDis2 = corNonoverlap;
  // target coordiante after displacement
  int corDisTgt = max(corDis1, corDis2);
#if SELECT_ODD_EVEN_ALIGN
  if (oddevenAlignFlag)
  {
    // odd corDisTgt when odd start point
    if (corReal % 2)
      corDisTgt += 1;
  }
#endif
  // displacement offset
  int disOffset = corDisTgt - corOrg;

  // corNonoverlap for next box:
  // at least target coordiant of current box + box size
  corNonoverlap = corDisTgt + orgSize;
  if (corNonoverlap % alignSize)
  {
    corNonoverlap += alignSize - corNonoverlap % alignSize;
  }

  return disOffset;
}


std::vector<vtm::DisBox> getDisplacementPara(const DisplacementParameterSet* dps, int chShift, int& disImgWdt, int& disImgHgt, bool displacementFlag, int chromaFormat)
{
  std::vector<vtm::DisBox> disBoxes;

  int targetBoxWdt = dps->widthAlignSize;
  int targetBoxHgt = dps->heightAlignSize;

  // character box start number
  int boxCurRowStart = 0;
  int boxNextRowStart = 0;

  // original position
  int boxOrgPosX = 0;
  int boxOrgPosY = 0;
  // position after displacement
  int boxDisPosX = 0;
  int boxDisPosY = 0;
  // (next box) position after displacement:
  // at least not overlap with current box
  int nextboxDisPosX = 0;
  int nextboxDisPosY = 0;
  // box width & height
  int boxOrgWdt = 0;
  int boxOrgHgt = 0;
  // displacement offset
  int boxDisOffset = 0;
#if SHIFT_COR
  // for shift
  int boxOrgPosXForShift = 0;
  int boxOrgPosYForShift = 0;
  int boxDisPosXForShift = 0;
  int boxDisPosYForShift = 0;
  int boxOrgWdtForShift = 0;
  int boxOrgHgtForShift = 0;
#endif

  for (int textRowNum = 0; textRowNum < dps->charBoxHeight.size(); textRowNum++)
  {
    boxOrgPosY = dps->topOfFirstChar[textRowNum];
    boxOrgHgt = dps->charBoxHeight[textRowNum];

    // get box displacement coordinate Y
    // - dps->topOfFirstChar[0]: minus the coordinate of background regions above the first text row for displace
    boxDisOffset = getCoordinate(boxOrgPosY, boxOrgPosY - dps->topOfFirstChar[0], nextboxDisPosY, boxOrgHgt, targetBoxHgt,
      chromaFormat, dps->oddevenAlignFlag);

    if (displacementFlag)
      boxDisPosY = boxOrgPosY + boxDisOffset - dps->topOfFirstChar[0];

    // for X
    int boxRowPosX = dps->leftOfFirstChar[textRowNum];
    boxOrgPosX = boxRowPosX - dps->intervalOfLeftOfChars[textRowNum];
    boxNextRowStart = boxCurRowStart + dps->charBoxNum[textRowNum];
    nextboxDisPosX = 0;

    for (int boxIdx = boxCurRowStart; boxIdx < boxNextRowStart; boxIdx++)
    {
      // get box original coordinate X
      boxOrgPosX += dps->intervalOfLeftOfChars[textRowNum] + dps->biasOfLeftOfChars[boxIdx];
      // get box width:
      // interval of left of char boxes when not last box
      if (boxIdx == boxNextRowStart - 1)
        // boxOrgWdt = dps->rightOfLastChar[textRowNum] - boxOrgPosX;
        boxOrgWdt = dps->rightOfLastChar[textRowNum] - boxOrgPosX + 1;
      else
        // boxOrgWdt = dps->intervalOfLeftOfChars[textRowNum] + dps->biasOfLeftOfChars[boxIdx + 1] - 1;
        boxOrgWdt = dps->intervalOfLeftOfChars[textRowNum] + dps->biasOfLeftOfChars[boxIdx + 1];

      // get box displacement coordinate X
      // - boxRowPosX: minus the coordinate of background regions at the left current text row for displace
      boxDisOffset = getCoordinate(boxOrgPosX, boxOrgPosX - boxRowPosX, nextboxDisPosX, boxOrgWdt, targetBoxWdt,
        chromaFormat, dps->oddevenAlignFlag);

      if (displacementFlag)
      {
        boxDisPosX = boxOrgPosX + boxDisOffset - boxRowPosX;

#if SHIFT_COR
        boxOrgPosXForShift = boxOrgPosX;
        boxOrgPosYForShift = boxOrgPosY;
        boxDisPosXForShift = boxDisPosX;
        boxDisPosYForShift = boxDisPosY;
        boxOrgWdtForShift = boxOrgWdt;
        boxOrgHgtForShift = boxOrgHgt;

        if (chShift)
        {
#if SAMPLE_STARTPOINT_RIGHTMOVE_1
          // STARTPOINT: boxOrgWdtForShift >> 1, boxOrgHgtForShift >> 1
          // correct width or height when odd start point
          if (boxOrgPosXForShift % 2)
              boxOrgWdtForShift += 1;
          if (boxOrgPosYForShift % 2)
              boxOrgHgtForShift += 1;
#else
          // STARTPOINT: first even corbox
          // correct width or height when even start point
          if (boxOrgPosXForShift % 2 == 0)
            boxOrgWdtForShift += 1;
          if (boxOrgPosYForShift % 2 == 0)
            boxOrgHgtForShift += 1;
          // STARTPOINT: first even corbox
          boxOrgPosXForShift += 1;
          boxDisPosXForShift += 1;
          boxOrgPosYForShift += 1;
          boxDisPosYForShift += 1;
#endif
        }

        vtm::Area orgArea(vtm::Position(boxOrgPosXForShift >> chShift, boxOrgPosYForShift >> chShift), vtm::Size(boxOrgWdtForShift >> chShift, boxOrgHgtForShift >> chShift));
        vtm::Area disArea(vtm::Position(boxDisPosXForShift >> chShift, boxDisPosYForShift >> chShift), vtm::Size(boxOrgWdtForShift >> chShift, boxOrgHgtForShift >> chShift));
#else
        vtm::Area orgArea(vtm::Position(boxOrgPosX >> chShift, boxOrgPosY >> chShift), vtm::Size(boxOrgWdt >> chShift, boxOrgHgt >> chShift));
        vtm::Area disArea(vtm::Position(boxDisPosX >> chShift, boxDisPosY >> chShift), vtm::Size(boxOrgWdt >> chShift, boxOrgHgt >> chShift));
#endif

        // printf("\nOrgBox: %d\t%d\t%d\t%d, TextBox: %d\t%d", orgArea.x, orgArea.y, orgArea.x + orgArea.width, orgArea.y + orgArea.height, disArea.x, disArea.y);
        disBoxes.push_back(vtm::DisBox(orgArea, disArea));
      }
    }

    boxCurRowStart = boxNextRowStart;

    // get w&h of text layer when displacementFlag == false
    if (1) // if (displacementFlag == false)
    {
      // use max value as disImgWdt or disImgHgtbufDst
      disImgWdt = judgeMaxPara(nextboxDisPosX, disImgWdt);
      disImgHgt = judgeMaxPara(nextboxDisPosY, disImgHgt);
    }
  }

  return disBoxes;
}


void getTextLayerReso(cv::Mat img, const DisplacementParameterSet* dps, int& TextLayerWdt, int& TextLayerHgt, const int chromaFormat)
{
 std::vector<vtm::DisBox> disBoxes = getDisplacementPara(dps, 0, TextLayerWdt, TextLayerHgt, false, chromaFormat);
}


void textBlockDisplacement(const DisplacementParameterSet* dps, Pel *bufBg, Pel *bufText, const int strideBg, const int strideText, const int chShift, const int chromaFormat, const int compNum, const bool layerFlag)
{
  int disImgWdt = 0, disImgHgt = 0;
  std::vector<vtm::DisBox> disBoxes = getDisplacementPara(dps, chShift, disImgWdt, disImgHgt, true, chromaFormat);

  if (layerFlag)
  {
    bgColorFilling(bufText, strideText, Pel(dps->backGroundColor[compNum]), chShift, disImgWdt, disImgHgt);
  }

  // displacement for each text box
  for (int boxCnt = 0; boxCnt < disBoxes.size(); boxCnt++)
  {
    copyPelBuffer(disBoxes[boxCnt].disArea, disBoxes[boxCnt].orgArea, bufText, bufBg, strideText, strideBg, layerFlag? dps->backGroundColor[compNum]: 0, layerFlag);
  }
}