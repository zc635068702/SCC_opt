/* The copyright in this software is being made available under the BSD
 * License, included below. This software may be subject to other third party
 * and contributor rights, including patent rights, and no such rights are
 * granted under this license.
 *
 * Copyright (c) 2010-2021, ITU/ISO/IEC
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

/** \file     dtrace_blockstatistics.h
 *  \brief    DTrace block statistcis support for next software
 */

#ifndef _DTRACE_BLOCKSTATISTICS_H_
#define _DTRACE_BLOCKSTATISTICS_H_

#include <map>
#include <string>
#include <iostream>
#include <sstream>
#include <vector>
#include <cstdlib>
#include <queue>

#include <stdio.h>
#include <list>
#include <map>
#include <set>
#include <cstdarg>


#include "TLibCommon/CommonDef.h"
#include "TLibCommon/TComRom.h"

#if K0149_BLOCK_STATISTICS
extern void dtrace_header( const char *format, /*va_list args*/... );
extern void dtrace_block_scalar(int poc, int posX, int posY, int width, int height, std::string stat_type, signed value);
extern void dtrace_block_vector(int poc, int posX, int posY, int width, int height, std::string stat_type, signed val_x, signed val_y);
#define DTRACE_HEADER(...) dtrace_header( __VA_ARGS__ )
#define DTRACE_BLOCK_SCALAR(poc, posX, posY, width, height, stat_type, val) dtrace_block_scalar(poc, posX, posY, width, height, stat_type, val)
#define DTRACE_BLOCK_VECTOR(poc, posX, posY, width, height, stat_type, val_x, val_y) dtrace_block_vector(poc, posX, posY, width, height, stat_type, val_x, val_y)

enum class BlockStatistic {
  // general
  PredMode,
  QT_Depth,
  Part_Size,
  //QP,

  // intra
  Luma_IntraMode,
  //Chroma_IntraMode,
  PaletteFlag,
  IbcFlag,

  // inter
  SkipFlag,
  //IMVMode,
  InterDir,
  MergeFlag,
  MergeIdx,
  //MVPIdxL0,
  //MVPIdxL1,
  MVL0,
  MVL1,
  MVDL0,
  MVDL1,
  //MotionBufL0,
  //MotionBufL1,
  RefIdxL0,
  RefIdxL1,

  CU_ROOT_CBF,             ///< In the CU is inter, get the root coded block flag of the TU
  TU_CBF_Y,                ///< Get the coded block flag for luma
  // TU_CBF_CB,               ///< Get the coded block flag for chroma U
  // TU_CBF_CR,               ///< Get the coded block flag for chroma V
  TU_COEFF_TR_SKIP_Y,      ///< Get the transform skip flag for luma
  // TU_COEFF_TR_SKIP_CB,     ///< Get the transform skip flag for chroma U
  // TU_COEFF_TR_SKIP_CR,     ///< Get the transform skip flag for chroma V
  TU_COEFF_ENERGY_Y,       ///< If the root CBF of the TU is not 0, get the coefficient energy of the TU for luma
  // TU_COEFF_ENERGY_CB,      ///< If the root CBF of the TU is not 0, get the coefficient energy of the TU for chroma U
  // TU_COEFF_ENERGY_CR,      ///< If the root CBF of the TU is not 0, get the coefficient energy of the TU for chroma V

  CU_BPP,
  NumBlockStatistics,
};

enum class BlockStatisticType {
  Flag,
  Vector,
  Integer,
  Line,
  FlagPolygon,
  VectorPolygon,
  IntegerPolygon,
};

static const std::map<BlockStatistic, std::tuple<std::string, BlockStatisticType, std::string>> blockstatistic2description =
{
  // Statistics enum                                                                                Statistics name string         Statistic Type                              Type specific information:
  //                                                                                                                                                                           Value range, vector scale
  { BlockStatistic::PredMode,               std::tuple<std::string, BlockStatisticType, std::string>{"PredMode",                    BlockStatisticType::Integer,                "[0, " + std::to_string(NUMBER_OF_PREDICTION_MODES) + "]"}},
  { BlockStatistic::MergeFlag,              std::tuple<std::string, BlockStatisticType, std::string>{"MergeFlag",                   BlockStatisticType::Flag,                   ""}},
  { BlockStatistic::MVL0,                   std::tuple<std::string, BlockStatisticType, std::string>{"MVL0",                        BlockStatisticType::Vector,                 "Scale: 4"}},
  { BlockStatistic::MVL1,                   std::tuple<std::string, BlockStatisticType, std::string>{"MVL1",                        BlockStatisticType::Vector,                 "Scale: 4"}},
  { BlockStatistic::Luma_IntraMode,         std::tuple<std::string, BlockStatisticType, std::string>{"Luma_IntraMode",              BlockStatisticType::Integer,                "[0, " + std::to_string(NUM_INTRA_MODE) + "]"}},
  //{ BlockStatistic::Chroma_IntraMode,       std::tuple<std::string, BlockStatisticType, std::string>{"Chroma_IntraMode",            BlockStatisticType::Integer,                "[0, " + std::to_string(NUM_INTRA_MODE) + "]"}},
  { BlockStatistic::SkipFlag,               std::tuple<std::string, BlockStatisticType, std::string>{"SkipFlag",                    BlockStatisticType::Flag,                   ""}},

  { BlockStatistic::IbcFlag,                std::tuple<std::string, BlockStatisticType, std::string>{"IbcFlag",                     BlockStatisticType::Flag,                   ""}},
  { BlockStatistic::PaletteFlag,            std::tuple<std::string, BlockStatisticType, std::string>{"PaletteFlag",                 BlockStatisticType::Flag,                   ""}},
  { BlockStatistic::QT_Depth,               std::tuple<std::string, BlockStatisticType, std::string>{"QT_Depth",                    BlockStatisticType::Integer,                "[0, 3]"}},
  { BlockStatistic::Part_Size,              std::tuple<std::string, BlockStatisticType, std::string>{"Part_Size",                   BlockStatisticType::Integer,                "[0, " + std::to_string( NUMBER_OF_PART_SIZES ) + "]"}},
  //{ BlockStatistic::QP,                     std::tuple<std::string, BlockStatisticType, std::string>{"QP",                          BlockStatisticType::Integer,                "[0, 51]"}},
  { BlockStatistic::MergeIdx,               std::tuple<std::string, BlockStatisticType, std::string>{"MergeIdx",                    BlockStatisticType::Integer,                "[0, 7]"}},
  { BlockStatistic::InterDir,               std::tuple<std::string, BlockStatisticType, std::string>{"InterDir",                    BlockStatisticType::Integer,                "[1, 3]"}},
  //{ BlockStatistic::MVPIdxL0,               std::tuple<std::string, BlockStatisticType, std::string>{"MVPIdxL0",                    BlockStatisticType::Integer,                "[0, 1]"}},
  { BlockStatistic::MVDL0,                  std::tuple<std::string, BlockStatisticType, std::string>{"MVDL0",                       BlockStatisticType::Vector,                 "Scale: 4"}},
  { BlockStatistic::RefIdxL0,               std::tuple<std::string, BlockStatisticType, std::string>{"RefIdxL0",                    BlockStatisticType::Integer,                "[0, 4]"}},
  //{ BlockStatistic::MVPIdxL1,               std::tuple<std::string, BlockStatisticType, std::string>{"MVPIdxL1",                    BlockStatisticType::Integer,                "[0, 1]"}},
  { BlockStatistic::MVDL1,                  std::tuple<std::string, BlockStatisticType, std::string>{"MVDL1",                       BlockStatisticType::Vector,                 "Scale: 4"}},
  { BlockStatistic::RefIdxL1,               std::tuple<std::string, BlockStatisticType, std::string>{"RefIdxL1",                    BlockStatisticType::Integer,                "[0, 4]"}},
  //{ BlockStatistic::IMVMode,                std::tuple<std::string, BlockStatisticType, std::string>{"IMVMode",                     BlockStatisticType::Integer,                "[0, 2]"}},

  { BlockStatistic::CU_ROOT_CBF,            std::tuple<std::string, BlockStatisticType, std::string>{"Root_Cbf",                 BlockStatisticType::Flag,                    ""}},
  { BlockStatistic::TU_CBF_Y,               std::tuple<std::string, BlockStatisticType, std::string>{"TU_Cbf_Y",                       BlockStatisticType::Flag,                   ""}},
  // { BlockStatistic::TU_CBF_CB,              std::tuple<std::string, BlockStatisticType, std::string>{"TU_Cbf_Cb",                      BlockStatisticType::Flag,                   ""}},
  // { BlockStatistic::TU_CBF_CR,              std::tuple<std::string, BlockStatisticType, std::string>{"TU_Cbf_Cr",                      BlockStatisticType::Flag,                   ""}},
  { BlockStatistic::TU_COEFF_TR_SKIP_Y,     std::tuple<std::string, BlockStatisticType, std::string>{"TU_TRS_Y",                       BlockStatisticType::Flag,                   ""}},
  // { BlockStatistic::TU_COEFF_TR_SKIP_CB,    std::tuple<std::string, BlockStatisticType, std::string>{"TU_TRS_Cb",                      BlockStatisticType::Flag,                   ""}},
  // { BlockStatistic::TU_COEFF_TR_SKIP_CR,    std::tuple<std::string, BlockStatisticType, std::string>{"TU_TRS_Cr",                      BlockStatisticType::Flag,                   ""}},
  { BlockStatistic::TU_COEFF_ENERGY_Y,      std::tuple<std::string, BlockStatisticType, std::string>{"TU_Coeff_Energy_Y",                       BlockStatisticType::Integer,       "[0, 1000]"}},
  // { BlockStatistic::TU_COEFF_ENERGY_CB,     std::tuple<std::string, BlockStatisticType, std::string>{"TU_Coeff_Energy_Cb",                      BlockStatisticType::Integer,       "[0, 1000]"}},
  // { BlockStatistic::TU_COEFF_ENERGY_CR,     std::tuple<std::string, BlockStatisticType, std::string>{"TU_Coeff_Energy_Cr",                      BlockStatisticType::Integer,       "[0, 1000]"}},
  { BlockStatistic::CU_BPP,                 std::tuple<std::string, BlockStatisticType, std::string>{"CU_BPP",                                BlockStatisticType::Integer,        "[0, 2000]"}},
};


std::string GetBlockStatisticName(BlockStatistic statistic);
std::string GetBlockStatisticTypeString(BlockStatistic statistic);
std::string GetBlockStatisticTypeSpecificInfo(BlockStatistic statistic);
#endif

#endif // _DTRACE_BLOCKSTATISTICS_H_
