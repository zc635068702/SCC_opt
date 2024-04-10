/** \file     CNNFilter.h
    \brief    convolutional neural network-based fiter class (header)
*/

#ifndef __CNNFILTER__
#define __CNNFILTER__

#include "CommonDef.h"
#include "TLibCommon/TComPic.h"
#include "TLibCommon/TComPicYuv.h"

#if CNN_FILTERING
#include <torch/script.h> // One-stop header.
#include <torch/torch.h>
//! \ingroup CommonLib
//! \{


typedef int PosType;
struct BoundingBox
{
  PosType left;
  PosType top;
  PosType width;
  PosType height;

  BoundingBox() : left(0), top(0), width(0), height(0) { }
  BoundingBox(const PosType _left, const PosType _top, const PosType _width, const PosType _height) : left(_left), top(_top), width(_width), height(_height) { }
};

class TCNNFilter
{
public:
  TCNNFilter();
  torch::jit::script::Module m_lumaModule;
  TComPicYuv* m_tempBuf;
  bool m_modelInitFlag;
  
  void initFilter(int qp);
  void cnnFilterLumaBlock( TComPic* pic, BoundingBox patchArea, int extLeft, int extRight, int extTop, int extBottom);
  void cnnFilter( TComPic* pic);
  void create( const int picWidth, const int picHeight, const ChromaFormat format, const UInt maxCUWidth, const UInt maxCUHeight, const UInt maxCUDepth);
  void destroy();
};
#endif

//! \}
#endif

