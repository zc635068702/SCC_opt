/** \file     EncCNNFilter.h
    \brief    encoder convolutional neural network-based filter class (header)
*/

#ifndef __ENCCNNFILTER__
#define __ENCCNNFILTER__

#include "CommonDef.h"

#if CNN_FILTERING

#include "TLibCommon/TCNNFilter.h"
#include <torch/script.h> // One-stop header.
#include <torch/torch.h>
//! \ingroup CommonLib
//! \{


class TEncCNNFilter : public TCNNFilter
{
public:
  TEncCNNFilter();
  virtual ~TEncCNNFilter();
};

#endif
//! \}
#endif

