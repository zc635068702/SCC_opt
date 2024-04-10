/** \file     CNNFilter.cpp
    \brief    convolutional neural network-based filter class
*/

#include "TCNNFilter.h"

#if CNN_FILTERING
#include <string.h>
#include <torch/script.h> // One-stop header.
#include <torch/torch.h>
#include <iostream>
//! \ingroup CommonLib
//! \{

using namespace torch;


TCNNFilter::TCNNFilter()
{
  m_modelInitFlag = false;
}

void TCNNFilter::initFilter(int qp)
{
  if (m_modelInitFlag)
    return;
  
  torch::set_num_threads(CNN_NUM_THREADS);
  std::string sQp;
  if (qp <= 29)
    sQp = "27";
  else if (qp <= 34)
    sQp = "32";
  else if (qp <= 39)
    sQp = "37";
  else
    sQp = "45";

  std::string sLumaModelName = "models/qp" + sQp + "_sampleV2_epoch40.pt";
  m_lumaModule = torch::jit::load(sLumaModelName, torch::Device(torch::kCPU));
  m_modelInitFlag = true;
}

void TCNNFilter::create( const int picWidth, const int picHeight, const ChromaFormat format, const UInt maxCUWidth, const UInt maxCUHeight, const UInt maxCUDepth )
{
  // m_tempBuf = new TComPicYuv;
  // m_tempBuf->create(picWidth, picHeight, format, maxCUWidth, maxCUHeight, maxCUDepth, true);
}

void TCNNFilter::destroy()
{
  // m_tempBuf->destroy();
}

void TCNNFilter::cnnFilterLumaBlock(TComPic* pic, BoundingBox patchArea, int extLeft, int extRight, int extTop, int extBottom)
{
  // only for Y
  const ComponentID compID = COMPONENT_Y;

  // yuv copy
  int strideRec = pic->getPicYuvRec()->getStride(compID);
  Pel* pRec = pic->getPicYuvRec()->getBuf(compID);
  UInt blockSizeHor = patchArea.width;
  UInt blockSizeVer = patchArea.height;
  UInt blockLeft    = patchArea.left;
  UInt blockTop     = patchArea.top;

  // Define input: recBatch and its gradient maps
  torch::Tensor imageBatch = torch::zeros({1, 1, blockSizeVer, blockSizeHor});
  float *pImageBatch = imageBatch.data_ptr<float>();
  int idx = 0;
  int blockSize = blockSizeVer * blockSizeHor;
  torch::NoGradGuard no_grad_guard;
  torch::globalContext().setFlushDenormal(true);
  int yyy = 0, xxx = 0;
  for (int yy = 0; yy < blockSizeVer; yy++)
  {
    for (int xx = 0; xx < blockSizeHor; xx++)
    {
      yyy = yy + blockTop;
      xxx = xx + blockLeft;
      idx = yyy * strideRec + xxx;
      pImageBatch[yy*blockSizeHor + xx] = pRec[idx];
    }
  }

  // gradient operation
  torch::Tensor filterX = torch::tensor({ {1, -1} }, torch::kFloat).unsqueeze(0).unsqueeze(0);
  torch::Tensor filterY = torch::tensor({ {1}, {-1} }, torch::kFloat).unsqueeze(0).unsqueeze(0);

  auto padX = nn::ZeroPad2d(nn::ZeroPad2dOptions({ 1, 0, 0, 0 }));
  auto padY = nn::ZeroPad2d(nn::ZeroPad2dOptions({ 0, 0, 1, 0 }));

  torch::Tensor gradX = nn::functional::conv2d(padX->forward(imageBatch), filterX, nn::functional::Conv2dFuncOptions().stride(1).padding(0));
  torch::Tensor gradY = nn::functional::conv2d(padY->forward(imageBatch), filterY, nn::functional::Conv2dFuncOptions().stride(1).padding(0));

  // Execute the model and turn its output into a tensor.
  double maxValue = 255;
  torch::Tensor output = m_lumaModule.forward({ imageBatch / maxValue, gradX / maxValue, gradY / maxValue }).toTuple()->elements()[0].toTensor();
  output = (output * maxValue).round();
  float *pOutput = output.data_ptr<float>();

  int strideDst = m_tempBuf->getStride(compID);
  Pel* pDst = m_tempBuf->getBuf(compID);
  for (int pixelIdx = 0, yy = 0, xx = 0; pixelIdx < blockSize; pixelIdx++)
  {
    xx = pixelIdx % blockSizeHor;
    yy = pixelIdx / blockSizeHor;
    if (xx < extLeft || yy < extTop || xx >= extLeft + blockSizeHor || yy >= extTop + blockSizeVer)
    {
      continue;
    }
    xx += blockLeft;
    yy += blockTop;
    idx = yy * strideDst + xx;
    pDst[idx] = Pel(Clip3<int>( 0, maxValue, pOutput[pixelIdx]));
  }
}

void TCNNFilter::cnnFilter( TComPic* pic )
{
  TComPicYuv* pcPicYuvRec = pic->getPicYuvRec();
  m_tempBuf = new TComPicYuv;
  m_tempBuf->create( pic->getSlice(0)->getSPS()->getPicWidthInLumaSamples(), pic->getSlice(0)->getSPS()->getPicHeightInLumaSamples(), pic->getSlice(0)->getSPS()->getChromaFormatIdc(),
    pic->getSlice(0)->getSPS()->getMaxCUWidth(), pic->getSlice(0)->getSPS()->getMaxCUHeight(), pic->getSlice(0)->getSPS()->getMaxTotalCUDepth(), true);
  pcPicYuvRec->copyToPic(m_tempBuf);
  int paddingSize = PADDING_SIZE;

  for (int comp = 0; comp <= COMPONENT_Y; comp ++)
  {
    unsigned int lumaWidth = pic->getSlice(0)->getSPS()->getPicWidthInLumaSamples();
    unsigned int lumaHeight = pic->getSlice(0)->getSPS()->getPicHeightInLumaSamples();

    unsigned int filterUint = CNNLF_INFER_BLOCK_SIZE;
    unsigned int unitWidth = filterUint;
    unsigned int unitHeight = filterUint;
    unsigned int widthInUnits = (lumaWidth + unitWidth - 1) / unitWidth;
    unsigned int hetghtInUnits = (lumaHeight + unitHeight - 1) / unitHeight;

    for( int unitIdx = 0; unitIdx < widthInUnits*hetghtInUnits; unitIdx++ )
    {
      int unitXPosInUnits        = unitIdx % widthInUnits;
      int unitYPosInUnits        = unitIdx / widthInUnits;
      int posX = unitXPosInUnits * unitWidth;
      int posY = unitYPosInUnits * unitHeight;
      int width = (posX + unitWidth > lumaWidth) ? (lumaWidth - posX) : unitWidth;
      int height = (posY + unitHeight > lumaHeight) ? (lumaHeight - posY) : unitHeight;
      
      int extLeft = posX > 0 ? paddingSize : 0;
      int extRight = (posX + width + paddingSize > lumaWidth) ? (lumaWidth - posX - width) : paddingSize;
      int extTop = posY > 0 ? paddingSize : 0;
      int extBottom = (posY + height + paddingSize > lumaHeight) ? (lumaHeight - posY - height) : paddingSize;
      
      int extPosX = posX - extLeft;
      int extPosY = posY - extTop;
      int extWidth = width + extLeft + extRight;
      int extHeight =  height + extTop + extBottom;

      cnnFilterLumaBlock(pic, BoundingBox(extPosX, extPosY, extWidth, extHeight), extLeft, extRight, extTop, extBottom);
    }
  }

  m_tempBuf->copyToPic(pcPicYuvRec);
  m_tempBuf->destroy();
}
#endif
//! \}
