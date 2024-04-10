// ============
// SJTU SCC
// ============

#include <vector>
#include <opencv2/opencv.hpp>

#define SHIFT_COR                             1
#define SAMPLE_STARTPOINT_RIGHTMOVE_1         1
#define SELECT_ODD_EVEN_ALIGN                 1

#ifndef Displace
#define Displace

typedef       int             Int;
typedef       short           Short;
typedef       Short           Pel;               ///< pixel type

namespace vtm
{
  typedef int PosType;
  typedef uint32_t SizeType;

  struct SplitLine
  {
    PosType cor1;
    PosType cor2;

    SplitLine() : cor1(0), cor2(0) { }
    SplitLine(const PosType _cor1, const PosType _cor2) : cor1(_cor1), cor2(_cor2) { }
  };

  struct BoundingBox
  {
    PosType left;
    PosType top;
    PosType right;
    PosType bottom;

    BoundingBox() : left(0), top(0), right(0), bottom(0) { }
    BoundingBox(const PosType _left, const PosType _top, const PosType _right, const PosType _bottom) : left(_left), top(_top), right(_right), bottom(_bottom) { }
  };

  struct Position
  {
    PosType x;
    PosType y;

    Position() : x(0), y(0) { }
    Position(const PosType _x, const PosType _y) : x(_x), y(_y) { }
  };

  struct Size
  {
    SizeType width;
    SizeType height;

    Size() : width(0), height(0) { }
    Size(const SizeType _width, const SizeType _height) : width(_width), height(_height) { }
  };

  struct Area : public Position, public Size
  {
    Area() : Position(), Size() { }
    Area(const Position &_pos, const Size &_size) : Position(_pos), Size(_size) { }
  };

  struct DisBox
  {
    Area orgArea;
    Area disArea;
    DisBox() : orgArea(Area()), disArea(Area()) {}
    DisBox(Area _orgArea, Area _disArea) : orgArea(_orgArea), disArea(_disArea) {}
  };
}



float median(std::vector<int> vec);



class DisplacementParameterSet
{
public:
  /* parameters may be encoded */
  // number of row box
  int rowNumber;
  // parameters in height
  // box number of characters per text row
  std::vector<int> charBoxNum;
  std::vector<int> charBoxNumDiff;
  // top of first character per text row
  std::vector<int> topOfFirstChar;
  std::vector<int> topOfFirstCharDiff;
  // charBoxHeight for displacing char box
  // bottom of characters: top + charBoxHeight
  std::vector<int> charBoxHeight;
  std::vector<int> charBoxHeightDiff;

  // parameters in width
  // left of first character per text row
  std::vector<int> leftOfFirstChar;
  std::vector<int> leftOfFirstCharDiff;
  // right of last character per text row
  std::vector<int> rightOfLastChar;
  std::vector<int> rightOfLastCharDiff;
  // box interval: optimal(median) box interval to decode char box position
  std::vector<int> intervalOfLeftOfChars;
  std::vector<int> intervalOfLeftOfCharsDiff;
  // left & right bias of characters:
  // real box interval - optimal box interval
  std::vector<int> biasOfLeftOfChars;
  std::vector<int> biasOfLeftOfCharsDiff;

  // displacement size in width & height
  int widthAlignSize = 8;
  int heightAlignSize = 32;

  // filled color in per text row
  // int backGroundColor[3];  // bgColor
  int* backGroundColor;  // bgColor

  // if oddevenAlign or not
  bool oddevenAlignFlag = false;

protected:
  int computeMedianInterval(std::vector<int> array1, std::vector<int> array2);

public:
  void update(const std::vector<vtm::BoundingBox> charBoxes);
  void updateColor(const int* bgColor)
  {
    backGroundColor = new int[3];
    backGroundColor[0] = bgColor[0];
    backGroundColor[1] = bgColor[1];
    backGroundColor[2] = bgColor[2];
  }
  void updateAlignFlag(cv::Mat yuvOrg, int stdThres = 5)
  {
    cv::Mat meanMat, stdMat;
    cv::meanStdDev(yuvOrg, meanMat, stdMat);
    double meanStdOfUV = (stdMat.at<double>(1) + stdMat.at<double>(2)) / 2.0;
    if (meanStdOfUV > stdThres)
      oddevenAlignFlag = true;
    std::cout << "Std of UV: " << meanStdOfUV << std::endl;
  }
};



void copyPelBuffer(const vtm::Area textArea, const vtm::Area bgArea, Pel* text, Pel* bg, const int textStride, const int bgStride, const int bgColor, const bool layerFlag);
int judgeMaxPara(const int possiblePara, int finalPara, const int padSize = 8);
int  getCoordinate(int corReal, int corOrg, int& corNext, const int orgSize, const int alignSize, const int chromaFormat, bool oddevenAlignFlag);
std::vector<vtm::DisBox> getDisplacementPara(const DisplacementParameterSet* dps, int chShift, int& disImgWidth, int& disImgHeight, bool displacementFlag, int chromaFormat);
void getTextLayerReso(cv::Mat img, const DisplacementParameterSet* dps, int& TextLayerWdt, int& yuvTextLayerHgt, const int chromaFormat);
void textBlockDisplacement(const DisplacementParameterSet* dps, Pel *bufBg, Pel *bufText, const int strideBg, const int strideText, const int chShift, const int chromaFormat, const int compNum = 0, const bool layerFlag = false);
#endif