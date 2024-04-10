// ============
// SJTU SCC
// ============

#include <vector>
#include "text_optimization_main.h"
#include "text_extract.h"
#include "util.h"

using namespace cv;
using namespace std;

#define Print        1

void TextOptimization::textOptimizationPreprocess(
  Mat yuvOrg, DisplacementParameterSet* dps,
  TComPicYuv* pcPicYuvOrg, TComPicYuv* pcPicYuvOrgTextLayer,
  TComPicYuv* pcPicYuvTrueOrg, TComPicYuv* pcPicYuvTrueOrgTextLayer,
  const ChromaFormat chromaFormatIDC,
  const UInt maxCUWidth,
  const UInt maxCUHeight,
  const UInt maxCUDepth,
  const Bool bUseMargin)
{
  // Step0: Read yuv
  int yuvOrgHgt = yuvOrg.rows;
  vector<Mat> yuvChannels;
  split(yuvOrg, yuvChannels);
  Mat yOrg = yuvChannels.at(0);

  // class init
  TextDetect* textDetect = new TextDetect;
  PriorInfo* priorInfo = new PriorInfo;
  CharacterSegmentation* charSeg = new CharacterSegmentation;

#if Print
  cout << "YUV Size: " << yuvOrg.size() << " Channels: " << yuvOrg.channels() << " Type: " << yuvOrg.type() << endl;
#endif

  // Step1: Get text regions from edge map
  // Step1.1: get (canny) edge map
  Mat yuvEdgeMap = getEdgeMap(yOrg);

  // Step1.2: get text row boxes
  textDetect->getTextRow(yuvEdgeMap, yuvOrg, yuvOrgHgt); // yuvOrg
  if (!textDetect->realBoxes.size())
  {
    printf("No text in yuv.\n");
#if 1 // SELECT_TEXT_CODEC
    textSccCloseFlag = true;
    goto TextSccJumpPoint;
#endif
  }

#if 0
  cout << "Text Row Number: " << textDetect->realBoxes.size() << endl;
  for (auto box: textDetect->realBoxes)
  {
    cout << "Text Row Box:" << " " << box[0] << " " << box[1] << " " << box[2] << " " << box[3] << endl;
  }
#endif

  // Step2: get prior information of text: color & paragraph & characters
  // Step2.1: get background color
  // only process rowBoxes in background color
  priorInfo->getBgColor(yuvOrg, textDetect->realBoxes); // yuvOrg

#if Print
  cout << "Background Color: " << priorInfo->bgColor[0] << " " << priorInfo->bgColor[1] << " " << priorInfo->bgColor[2] << " " << endl;
  cout << "Text Row Box Number (all colors): " << textDetect->realBoxes.size() << endl;
  cout << "Text Row Box Number (background color): " << priorInfo->bgColorBoxes.size() << endl;
#endif

  // Step2.2: identify paragraph
  priorInfo->identifyParagraph(yuvEdgeMap);
  if (!priorInfo->paragraphs.size())
  {
    printf("Scattered text in pic \n");
    textSccCloseFlag = true;
    goto TextSccJumpPoint;
  }

#if Print
  for (auto box: priorInfo->paragraphs)
  {
    cout << "Paragraph:" << " " << box << endl;
  }
#endif

  // Step3: get displacement parameters & displacement
  // Step3.1: character segmentation
  /* get displacement parameter set (dps):
  charBoxNum topOfFirstChar charBoxHeight
  leftOfFirstChar c_first_right
  intervalOfLeftOfChars c_right_interval
  biasOfLeftOfChars c_right_bias */
  charSeg->charSegment(yOrg, priorInfo->bgColorBoxes, priorInfo->paragraphs, priorInfo->bgColor, dps); // yOrg bgColor

#if Print
  cout << "Character Box Number: " << charSeg->totalCharBoxes.size() << endl;
#endif

  // Step3.2: text block displacement
#if SELECT_ODD_EVEN_ALIGN == 1
  if (pcPicYuvOrg->getChromaFormat() != 3) // not YUV444
    dps->updateAlignFlag(yuvOrg);
#elif SELECT_ODD_EVEN_ALIGN == 2 // force odd even align
  dps->oddevenAlignFlag = true;
#endif
  getTextLayerReso(yuvOrg, dps, yuvTextWdt, yuvTextHgt, pcPicYuvOrg->getChromaFormat()); // yuvOrg
  // layer buffer by dps
  pcPicYuvOrgTextLayer->create(yuvTextWdt, yuvTextHgt, chromaFormatIDC, maxCUWidth, maxCUHeight, maxCUDepth, bUseMargin);
  pcPicYuvTrueOrgTextLayer->create(yuvTextWdt, yuvTextHgt, chromaFormatIDC, maxCUWidth, maxCUHeight, maxCUDepth, bUseMargin);
#if TEXT_CODEC
  pcPicYuvOrg->xyuvLayerAndStitch(pcPicYuvOrgTextLayer, dps, true, pcPicYuvOrg->getChromaFormat());
  pcPicYuvTrueOrg->xyuvLayerAndStitch(pcPicYuvTrueOrgTextLayer, dps, true, pcPicYuvTrueOrg->getChromaFormat());
#endif

TextSccJumpPoint :

#if 1
  cout << "yuvTextHgt & yuvTextWdt: " << yuvTextHgt << " " << yuvTextWdt << endl;
#endif

  delete textDetect;
  textDetect = NULL;
  delete priorInfo;
  priorInfo = NULL;
  delete charSeg;
  charSeg = NULL;
}
