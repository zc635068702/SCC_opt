// ============
// SJTU SCC
// ============

#include <opencv2/opencv.hpp>
#include "displacement.h"
#include "TLibCommon/TComPicYuv.h"

#define Test_Flag    1

#ifndef Text_Optimization
#define Text_Optimization

class TextOptimization
{
public:
  // w&h of text layer
  int yuvTextHgt = 0, yuvTextWdt = 0;
  // w&h of background layer
  int yuvBgHgt,   yuvBgWdt;
  bool textSccCloseFlag = false;

public:
  void textOptimizationPreprocess(
      cv::Mat yuvOrg, DisplacementParameterSet* dps,
      TComPicYuv* pcPicYuvOrg, TComPicYuv* pcPicYuvOrgTextLayer,
      TComPicYuv* pcPicYuvTrueOrg, TComPicYuv* pcPicYuvTrueOrgTextLayer,
      const ChromaFormat chromaFormatIDC,
      const UInt maxCUWidth,
      const UInt maxCUHeight,
      const UInt maxCUDepth,
      const Bool bUseMargin);
};

#endif