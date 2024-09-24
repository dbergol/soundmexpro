//------------------------------------------------------------------------------
/// \file HtTransferFunction.h
///
/// \author Berg
/// \brief Implementation helper classes CHtTransferFunction and
/// CHtTransferFunctions
///
/// Project SoundMexPro
/// Module  SoundDllPro.dll
/// \brief Implementation helper classes CHtTransferFunction and
/// CHtTransferFunctions used in CHtPartitionedConvolution
///
///               Copyright 2023 Daniel Berg, Oldenburg, Germany
///
/// ****************************************************************************
/// Copyright 2023 Daniel Berg, Oldenburg, Germany
/// ****************************************************************************
///
/// This file is part of SoundMexPro.
///
///    SoundMexPro is free software: you can redistribute it and/or modify
///    it under the terms of the GNU General Public License as published by
///    the Free Software Foundation, either version 3 of the License, or
///    (at your option) any later version.
///
///    SoundMexPro is distributed in the hope that it will be useful,
///    but WITHOUT ANY WARRANTY; without even the implied warranty of
///    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
///    GNU General Public License for more details.
///
///    You should have received a copy of the GNU General Public License
///    along with SoundMexPro.  If not, see <http:///www.gnu.org/licenses/>.
///
//------------------------------------------------------------------------------
#ifndef HtTransferFunctionH
#define HtTransferFunctionH
//--------------------------------------------------------------------------

#include <vector>
#include <valarray>

//--------------------------------------------------------------------------
/// Helper class containing an impulse response for one channel with helper
/// functions. Prefix tf.
//--------------------------------------------------------------------------
class CHtTransferFunction
{
   public:
      std::vector<float>   m_vfImpulseResponse;    /// float vector containing impulse response for one channel
      unsigned int         m_nChannelIndex;        /// corresponing channel index

      CHtTransferFunction( unsigned int nChannelIndex, const std::vector<float> & vfImpulseResponse);
      unsigned int   Partitions(unsigned int nFragsize) const;
      unsigned int   PartitionsNonEmpty(unsigned int nFragsize) const;
      bool           IsEmpty(unsigned int nFragsize, unsigned int nIndex) const;
};
//--------------------------------------------------------------------------

//--------------------------------------------------------------------------
/// Helper class containing vector of CHtTransferFunction with helper functions.
/// Prefix tfs.
//--------------------------------------------------------------------------
class CHtTransferFunctions : public std::vector<CHtTransferFunction>
{
   public:
      std::valarray<unsigned int> Partitions(unsigned int nFragsize) const;
      std::valarray<unsigned int> PartitionsNonEmpty(unsigned int nFragsize) const;
};
//--------------------------------------------------------------------------

#endif // #ifndef HtTransferFunctionH
