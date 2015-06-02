/* The copyright in this software is being made available under the BSD
 * License, included below. This software may be subject to other third party
 * and contributor rights, including patent rights, and no such rights are
 * granted under this license.
 *
 * Copyright (c) 2010-2015, ITU/ISO/IEC
 * All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions are met:
 *
 *  * Redistributions of source code must retain the above copyright notice,
 *    this list of conditions and the following disclaimer.
 *  * Redistributions in binary form must reproduce the above copyright notice,
 *    this list of conditions and the following disclaimer in the documentation
 *    and/or other materials provided with the distribution.
 *  * Neither the name of the ITU/ISO/IEC nor the names of its contributors may
 *    be used to endorse or promote products derived from this software without
 *    specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS"
 * AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
 * IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
 * ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT HOLDER OR CONTRIBUTORS
 * BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR
 * CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF
 * SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS
 * INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN
 * CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE)
 * ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF
 * THE POSSIBILITY OF SUCH DAMAGE.
 */

/** \file     TDecSbac.h
    \brief    SBAC decoder class (header)
*/

#ifndef __TDECSBAC__
#define __TDECSBAC__


#if _MSC_VER > 1000
#pragma once
#endif // _MSC_VER > 1000


#include "TDecEntropy.h"
#include "TDecBinCoder.h"
#include "TLibCommon/ContextTables.h"
#include "TLibCommon/ContextModel.h"
#include "TLibCommon/ContextModel3DBuffer.h"

//! \ingroup TLibDecoder
//! \{

// ====================================================================================================================
// Class definition
// ====================================================================================================================

class SEImessages;

/// SBAC decoder class
class TDecSbac : public TDecEntropyIf
{
public:
  TDecSbac();
  virtual ~TDecSbac();

  Void  init                      ( TDecBinIf* p )    { m_pcTDecBinIf = p; }
  Void  uninit                    (              )    { m_pcTDecBinIf = 0; }

  Void load                       ( const TDecSbac* pSrc );
  Void loadContexts               ( const TDecSbac* pSrc );
  Void xCopyFrom                  ( const TDecSbac* pSrc );
  Void xCopyContextsFrom          ( const TDecSbac* pSrc );

  Void  resetEntropy (TComSlice* pSlice );
  Void  setBitstream              ( TComInputBitstream* p  ) { m_pcBitstream = p; m_pcTDecBinIf->init( p ); }
  Void  parseVPS                  ( TComVPS* /*pcVPS*/ ) {}
  Void  parseSPS                  ( TComSPS* /*pcSPS*/ ) {}
  Void  parsePPS                  ( TComPPS* /*pcPPS*/ ) {}

  Void  parseSliceHeader          ( TComSlice* /*pcSlice*/, ParameterSetManager* /*parameterSetManager*/, const Int /*prevTid0POC*/) {}
  Void  parseTerminatingBit       ( UInt& ruiBit );
  Void  parseRemainingBytes       ( Bool noTrailingBytesExpected);
  Void  parseMVPIdx               ( Int& riMVPIdx          );
  Void  parseSaoMaxUvlc           ( UInt& val, UInt maxSymbol );
  Void  parseSaoMerge             ( UInt&  ruiVal   );
  Void  parseSaoTypeIdx           ( UInt&  ruiVal  );
  Void  parseSaoUflc              ( UInt uiLength, UInt& ruiVal     );
  Void parseSAOBlkParam           (SAOBlkParam& saoBlkParam, Bool* sliceEnabled, Bool leftMergeAvail, Bool aboveMergeAvail, const BitDepths &bitDepths);
  Void parseSaoSign               (UInt& val);

private:
#if RExt__DECODER_DEBUG_BIT_STATISTICS
  Void  xReadUnarySymbol    ( UInt& ruiSymbol, ContextModel* pcSCModel, Int iOffset, const class TComCodingStatisticsClassType &whichStat );
  Void  xReadUnaryMaxSymbol ( UInt& ruiSymbol, ContextModel* pcSCModel, Int iOffset, UInt uiMaxSymbol, const class TComCodingStatisticsClassType &whichStat );
  Void  xReadEpExGolomb     ( UInt& ruiSymbol, UInt uiCount, const class TComCodingStatisticsClassType &whichStat );
  Void  xReadCoefRemainExGolomb ( UInt &rSymbol, UInt &rParam, const Bool useLimitedPrefixLength, const Int maxLog2TrDynamicRange, const class TComCodingStatisticsClassType &whichStat );
  Void  xDecodeRun          (UInt & ruiSymbol, Bool bCopyTopMode, const UInt uiPltIdx, const UInt uiMaxRun, const class TComCodingStatisticsClassType &whichStat);
  Void  xDecodePLTPredIndicator (UChar *bReusedPrev, UInt uiPLTSizePrev, UInt uiMaxPLTSize, const class TComCodingStatisticsClassType &whichStat);
  Void  xReadTruncBinCode   (UInt& ruiSymbol, UInt uiMaxSymbol, const class TComCodingStatisticsClassType &whichStat);
#if SCM_T0065_PLT_IDX_GROUP
  Void xAdjustPLTIndex(UInt siCurLevel, UInt uiIdx, Pel *pLevel, Int iMaxSymbol,
                      const class TComCodingStatisticsClassType &whichStat, UChar *pSPoint = 0, Int iWidth = 0,
                      UChar *pEscapeFlag = 0);
#endif
  Pel   xReadPLTIndex       (UInt uiIdx, Pel *pLevel, Int iMaxSymbol, const class TComCodingStatisticsClassType &whichStat, UChar *pSPoint = 0, Int iWidth = 0, UChar *pEscapeFlag = 0);
  UInt xReadTruncUnarySymbol( ContextModel* pcSCModel, UInt uiMax, UInt uiCtxT, UChar *ucCtxLut, const class TComCodingStatisticsClassType &whichStat);
  UInt xReadTruncMsbP1RefinementBits( ContextModel* pcSCModel, UInt uiMax, UInt uiCtxT, UChar *ucCtxLut, const class TComCodingStatisticsClassType &whichStat);

#else
  Void  xReadUnarySymbol    ( UInt& ruiSymbol, ContextModel* pcSCModel, Int iOffset );
  Void  xReadUnaryMaxSymbol ( UInt& ruiSymbol, ContextModel* pcSCModel, Int iOffset, UInt uiMaxSymbol );
  Void  xReadEpExGolomb     ( UInt& ruiSymbol, UInt uiCount );
  Void  xReadCoefRemainExGolomb ( UInt &rSymbol, UInt &rParam, const Bool useLimitedPrefixLength, const Int maxLog2TrDynamicRange );
  Void  xDecodeRun          (UInt & ruiSymbol, Bool bCopyTopMode, const UInt uiPltIdx, const UInt uiMaxRun);
  Void  xDecodePLTPredIndicator(UChar *bReusedPrev, UInt uiPLTSizePrev, UInt uiMaxPLTSize);
  Void  xReadTruncBinCode       (UInt& ruiSymbol, UInt uiMaxSymbol);
#if SCM_T0065_PLT_IDX_GROUP
  Void xAdjustPLTIndex(UInt siCurLevel, UInt uiIdx, Pel *pLevel, Int iMaxSymbol, UChar *pSPoint = 0, Int iWidth = 0,
                       UChar *pEscapeFlag = 0);
#endif
  Pel   xReadPLTIndex           (UInt uiIdx, Pel *pLevel, Int iMaxSymbol, UChar *pSPoint = 0, Int iWidth = 0, UChar *pEscapeFlag = 0);
  UInt xReadTruncUnarySymbol( ContextModel* pcSCModel, UInt uiMax, UInt uiCtxT, UChar *ucCtxLut);
  UInt xReadTruncMsbP1RefinementBits( ContextModel* pcSCModel, UInt uiMax, UInt uiCtxT, UChar *ucCtxLut);
#endif

private:
  TComInputBitstream* m_pcBitstream;
  TDecBinIf*        m_pcTDecBinIf;

public:

  Void parseSkipFlag      ( TComDataCU* pcCU, UInt uiAbsPartIdx, UInt uiDepth );
  Void parseCUTransquantBypassFlag( TComDataCU* pcCU, UInt uiAbsPartIdx, UInt uiDepth );
  Void parsePLTModeFlag          ( TComDataCU* pcCU, UInt uiAbsPartIdx, UInt uiDepth );
#if SCM_S0043_PLT_DELTA_QP
  Void parsePLTModeSyntax        ( TComDataCU* pcCU, UInt uiAbsPartIdx, UInt uiDepth, UInt unNumComp, Bool& bCodeDQP, Bool& codeChromaQpAdj );
#else
  Void parsePLTModeSyntax        ( TComDataCU* pcCU, UInt uiAbsPartIdx, UInt uiDepth, UInt unNumComp);
#endif
  Void parseScanRotationModeFlag ( TComDataCU* pcCU, UInt uiAbsPartIdx, UInt uiDepth );
  Void parseSplitFlag     ( TComDataCU* pcCU, UInt uiAbsPartIdx, UInt uiDepth );
  Void parseMergeFlag     ( TComDataCU* pcCU, UInt uiAbsPartIdx, UInt uiDepth, UInt uiPUIdx );
  Void parseMergeIndex    ( TComDataCU* pcCU, UInt& ruiMergeIndex );
  Void parsePartSize      ( TComDataCU* pcCU, UInt uiAbsPartIdx, UInt uiDepth );
#if !SCM_T0227_INTRABC_SIG_UNIFICATION
  Void parsePartSizeIntraBC ( TComDataCU* pcCU, UInt uiAbsPartIdx, UInt uiDepth );
#endif
  Void parsePredMode      ( TComDataCU* pcCU, UInt uiAbsPartIdx, UInt uiDepth );

  Void parseIntraDirLumaAng( TComDataCU* pcCU, UInt uiAbsPartIdx, UInt uiDepth );

  Void parseIntraDirChroma( TComDataCU* pcCU, UInt uiAbsPartIdx, UInt uiDepth );

#if !SCM_T0227_INTRABC_SIG_UNIFICATION
  Void parseIntraBCFlag    ( TComDataCU* pcCU, UInt uiAbsPartIdx, UInt uiPartIdx, UInt uiDepth );
  Void parseIntraBC        ( TComDataCU* pcCU, UInt uiAbsPartIdx, UInt uiPartIdx, UInt uiDepth );
  Void parseIntraBCBvd     ( TComDataCU* pcCU, UInt uiAbsPartIdx, UInt uiPartIdx, UInt uiDepth, RefPicList eRefList );
#endif
  Void parseInterDir       ( TComDataCU* pcCU, UInt& ruiInterDir, UInt uiAbsPartIdx );
  Void parseRefFrmIdx      ( TComDataCU* pcCU, Int& riRefFrmIdx, RefPicList eRefList );
  Void parseMvd            ( TComDataCU* pcCU, UInt uiAbsPartIdx, UInt uiPartIdx, UInt uiDepth, RefPicList eRefList );

  Void parseCrossComponentPrediction ( class TComTU &rTu, ComponentID compID );

  Void parseTransformSubdivFlag( UInt& ruiSubdivFlag, UInt uiLog2TransformBlockSize );
  Void parseQtCbf         ( TComTU &rTu, const ComponentID compID, const Bool lowestLevel );
  Void parseColourTransformFlag( UInt uiAbsPartIdx, Bool& uiFlag );

  Void parseQtRootCbf     ( UInt uiAbsPartIdx, UInt& uiQtRootCbf );

  Void parseDeltaQP       ( TComDataCU* pcCU, UInt uiAbsPartIdx, UInt uiDepth );
  Void parseChromaQpAdjustment( TComDataCU* cu, UInt absPartIdx, UInt depth );

  Void parseIPCMInfo      ( TComDataCU* pcCU, UInt uiAbsPartIdx, UInt uiDepth);

  Void parseLastSignificantXY( UInt& uiPosLastX, UInt& uiPosLastY, Int width, Int height, ComponentID component, UInt uiScanIdx );
  Void parseCoeffNxN      ( class TComTU &rTu, ComponentID compID  );
  Void parseTransformSkipFlags ( class TComTU &rTu, ComponentID component );

  Void  parseScalingList ( TComScalingList* /*scalingList*/ ) {}

  Void  parseExplicitRdpcmMode( TComTU &rTu, ComponentID compID );

private:
  UInt* m_puiScanOrder;
  ContextModel         m_contextModels[MAX_NUM_CTX_MOD];
  Int                  m_numContextModels;
  ContextModel3DBuffer m_cCUSplitFlagSCModel;
  ContextModel3DBuffer m_cCUSkipFlagSCModel;
  ContextModel3DBuffer m_cCUMergeFlagExtSCModel;
  ContextModel3DBuffer m_cCUMergeIdxExtSCModel;
  ContextModel3DBuffer m_cCUPartSizeSCModel;
  ContextModel3DBuffer m_cCUPredModeSCModel;
  ContextModel3DBuffer m_cCUIntraPredSCModel;
  ContextModel3DBuffer m_cCUChromaPredSCModel;
  ContextModel3DBuffer m_cCUDeltaQpSCModel;
  ContextModel3DBuffer m_cCUInterDirSCModel;
  ContextModel3DBuffer m_cCURefPicSCModel;
  ContextModel3DBuffer m_cCUMvdSCModel;
  ContextModel3DBuffer m_cCUQtCbfSCModel;
  ContextModel3DBuffer m_cCUTransSubdivFlagSCModel;
  ContextModel3DBuffer m_cCUQtRootCbfSCModel;

  ContextModel3DBuffer m_cCUSigCoeffGroupSCModel;
  ContextModel3DBuffer m_cCUSigSCModel;
  ContextModel3DBuffer m_cCuCtxLastX;
  ContextModel3DBuffer m_cCuCtxLastY;
  ContextModel3DBuffer m_cCUOneSCModel;
  ContextModel3DBuffer m_cCUAbsSCModel;

  ContextModel3DBuffer m_cMVPIdxSCModel;

  ContextModel3DBuffer m_cSaoMergeSCModel;
  ContextModel3DBuffer m_cSaoTypeIdxSCModel;
  ContextModel3DBuffer m_cTransformSkipSCModel;
  ContextModel3DBuffer m_CUTransquantBypassFlagSCModel;
  ContextModel3DBuffer m_explicitRdpcmFlagSCModel;
  ContextModel3DBuffer m_explicitRdpcmDirSCModel;
#if !SCM_T0227_INTRABC_SIG_UNIFICATION
  ContextModel3DBuffer m_cIntraBCPredFlagSCModel;
#endif
  ContextModel3DBuffer m_cCrossComponentPredictionSCModel;

  ContextModel3DBuffer m_ChromaQpAdjFlagSCModel;
  ContextModel3DBuffer m_ChromaQpAdjIdcSCModel;
  ContextModel3DBuffer m_cCUColourTransformFlagSCModel;
#if !SCM_T0227_INTRABC_SIG_UNIFICATION
  ContextModel3DBuffer m_cIntraBCBVDSCModel;
#endif

  UInt m_golombRiceAdaptationStatistics[RExt__GOLOMB_RICE_ADAPTATION_STATISTICS_SETS];
  ContextModel3DBuffer m_PLTModeFlagSCModel;
  ContextModel3DBuffer m_SPointSCModel;
  ContextModel3DBuffer m_cCopyTopRunSCModel;
  ContextModel3DBuffer m_cRunSCModel;
#if SCM_T0065_PLT_IDX_GROUP
  ContextModel3DBuffer m_PLTLastRunTypeSCModel;  
#endif
  ContextModel3DBuffer m_PLTScanRotationModeFlagSCModel;  
};

//! \}

#endif // !defined(AFX_TDECSBAC_H__CFCAAA19_8110_47F4_9A16_810C4B5499D5__INCLUDED_)
