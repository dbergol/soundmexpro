//------------------------------------------------------------------------------
/// \file HtTransferFunction.cpp
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
#pragma hdrstop

#include "HtTransferFunction.h"
// include vcl.h only for borland compiler
#ifdef __BORLANDC__
   #include <vcl.h>
#else
   #include "BorlandException.h"
#endif

//--------------------------------------------------------------------------
/// define CHtException for use of CHtFFT outside of MOLM
//--------------------------------------------------------------------------
#ifndef CHtException
   #define CHtException Exception
#endif
//--------------------------------------------------------------------------

//--------------------------------------------------------------------------
/// Constructor. Initializes members
/// \param[in] nChannelIndex channel index to store
/// \param[in] vfImpulseResponse float vector containing impulse response
//--------------------------------------------------------------------------
CHtTransferFunction::CHtTransferFunction( unsigned int nChannelIndex,
                                          const std::vector<float> & vfImpulseResponse)
   :  m_vfImpulseResponse(vfImpulseResponse),
      m_nChannelIndex(nChannelIndex)
{
}
//--------------------------------------------------------------------------

//--------------------------------------------------------------------------
/// Returns number of partitions for a given fragment size
/// \param[in] nFragsize fragment size to calculate partition number for
/// \retval number of partitions
//--------------------------------------------------------------------------
unsigned int CHtTransferFunction::Partitions(unsigned int nFragsize) const
{
   if (nFragsize == 0)
      throw CHtException("Fragsize must be >0");
   return ((unsigned int)m_vfImpulseResponse.size() + nFragsize - 1) / nFragsize;
}
//--------------------------------------------------------------------------

//--------------------------------------------------------------------------
/// Returns number of non-empty partitions for a given fragment size
/// \param[in] nFragsize fragment size to calculate non-empty partition number for
/// \retval number of non-empty partitions
//--------------------------------------------------------------------------
unsigned int CHtTransferFunction::PartitionsNonEmpty(unsigned int nFragsize) const
{
   unsigned int nResult = 0;
   unsigned int nPartitions = Partitions(nFragsize);
   unsigned int nPartition;
   for (nPartition = 0; nPartition < nPartitions; nPartition++)
   if (!IsEmpty(nFragsize, nPartition))
      nResult++;
   return nResult;
}
//--------------------------------------------------------------------------

//--------------------------------------------------------------------------
/// Checks, if a part of the impulse response contains only zeros
/// \param[in] nFragsize fragment size to check
/// \param[in] nIndex index of partition to check
/// \retval true if corresponding part of the impulse contains zeros only,
/// or if length of impulse response is exceeded
/// \retval false else 
//--------------------------------------------------------------------------
bool CHtTransferFunction::IsEmpty(unsigned int nFragsize, unsigned int nIndex) const
{
   unsigned int nSize = (unsigned int)m_vfImpulseResponse.size();
   unsigned int nFrame;
   for (nFrame = 0; nFrame < nFragsize; nFrame++)
      {
      if (nIndex * nFragsize + nFrame >= nSize)
         return true;
      if (m_vfImpulseResponse[nIndex * nFragsize + nFrame] != 0.0f)
         return false;
      }
   return true;
}
//--------------------------------------------------------------------------

//--------------------------------------------------------------------------
///                  Class CHtTransferFunctions
//--------------------------------------------------------------------------

//--------------------------------------------------------------------------
/// returns a valarray containing the number of partitions of all CHtTransferFunction
/// instances contained in class vector
/// \param[in] nFragsize fragment size to calculate partition number for
/// \retval valarray with numbers of partitions
//--------------------------------------------------------------------------
std::valarray<unsigned int> CHtTransferFunctions::Partitions(unsigned int nFragsize) const
{
   unsigned int nSize = (unsigned int)size();
   std::valarray<unsigned int> vanResult(0U, nSize);
   unsigned int nPartition;
   for (nPartition = 0; nPartition < nSize; nPartition++)
      vanResult[nPartition] = (*this)[nPartition].Partitions(nFragsize);
   return vanResult;
}
//--------------------------------------------------------------------------

//--------------------------------------------------------------------------
/// returns a valarray containing the number of non-empty partitions of all
/// CHtTransferFunction instances contained in class vector
/// \param[in] nFragsize fragment size to calculate non-empty partition number for
/// \retval valarray with numbers of non-empty partitions
//--------------------------------------------------------------------------
std::valarray<unsigned int> CHtTransferFunctions::PartitionsNonEmpty(unsigned int nFragsize) const
{
   unsigned int nSize = (unsigned int)size();
   std::valarray<unsigned int> vanResult(0U, nSize);
   unsigned int nPartition;
   for (nPartition = 0; nPartition < nSize; nPartition++)
      vanResult[nPartition] = (*this)[nPartition].PartitionsNonEmpty(nFragsize);
   return vanResult;
}
//--------------------------------------------------------------------------

