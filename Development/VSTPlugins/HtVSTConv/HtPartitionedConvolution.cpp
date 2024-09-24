//------------------------------------------------------------------------------
/// \file HtPartitionedConvolution.cpp
///
/// \author Berg
/// \brief Implementation of class CHtPartitionedConvolution for partitioned
/// convolution
///
/// Project SoundMexPro
/// Module  SoundDllPro.dll
/// Implementation of class CHtPartitionedConvolution for partitioned
/// convolution
///
///
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
#include "HtPartitionedConvolution.h"
#include <mem.h>
//--------------------------------------------------------------------------

//--------------------------------------------------------------------------
/// Constructor. Initializes all members and buffers and breaks up impulse
/// responses into partitions.
/// \param[in] nFragSize Audio fragment size, equal to partition size.
/// \param[in] nChannels Number of audio channels.
/// \param[in] tfs CHtTransferFunctions sparse vector of impulse responses.
/// \exception Exception on invalid indices
//--------------------------------------------------------------------------
CHtPartitionedConvolution::CHtPartitionedConvolution( unsigned int nFragSize,
                                                      unsigned int nChannels,
                                                      const CHtTransferFunctions & tfs)
    : m_nFragSize(nFragSize),
      m_nChannels(nChannels),
      m_nOutputPartitions(tfs.Partitions(m_nFragSize).max()),
      m_nFilterPartitions(tfs.PartitionsNonEmpty(m_nFragSize).sum()),
      m_nWaveInBufferHalfCurrentIndex(0U),
      m_vvafWaveIn(m_nChannels, std::valarray<float>(2*m_nFragSize)),
      m_vvacSpecIn(m_nChannels, std::valarray<CHtComplex>(m_nFragSize+1)),
      m_vvacFrequencyResponse(m_nFilterPartitions, std::valarray<CHtComplex>(m_nFragSize+1)),
      m_fiBookKeeping(m_nFilterPartitions),
      m_vvacSpecOut(m_nOutputPartitions, vvac(m_nChannels, std::valarray<CHtComplex>(m_nFragSize+1))),
      m_nOutputPartitionCurrentIndex(0U),
      m_vvafWaveOut(m_nChannels, std::valarray<float>(m_nFragSize))
{
   // create CHtFFT instance
   unsigned int nFragSize_2 = 2*m_nFragSize;
   m_pfft = new CHtFFT(nFragSize_2);

   // create temporary wave buffer for impulse response
   vvaf vvafPartitions(m_nFilterPartitions, std::valarray<float>(nFragSize_2));

   // break up impulse responses into partitions
   // index into transfer
   unsigned int nTransferFunction;
   // ammount of delay, in blocks
   unsigned int nDelay;
   // index into frequency_response and partitions (channel index)
   unsigned int nFilterPartition = 0;
   unsigned int nFrame;

   for (nTransferFunction = 0; nTransferFunction < tfs.size(); nTransferFunction++)
      {
      for (nDelay = 0; nDelay < tfs[nTransferFunction].Partitions(m_nFragSize); nDelay++)
         {
         if (!tfs[nTransferFunction].IsEmpty(m_nFragSize, nDelay))
            {
            if (nFilterPartition >= m_nFilterPartitions)
               // should not happen
               throw Exception("Error calculating the number of needed filter partitions");
            for (nFrame = 0; nFrame < m_nFragSize && (nFrame + nDelay * m_nFragSize) < (tfs[nTransferFunction].m_vfImpulseResponse.size()); nFrame++)
               vvafPartitions[nFilterPartition][nFrame] = tfs[nTransferFunction].m_vfImpulseResponse[nFrame + nDelay * m_nFragSize];
            m_fiBookKeeping[nFilterPartition].m_nChannelIndex = tfs[nTransferFunction].m_nChannelIndex;
            if (m_fiBookKeeping[nFilterPartition].m_nChannelIndex >= m_nChannels)
               throw Exception("Channel index is out of range");
            m_fiBookKeeping[nFilterPartition].m_nDelay = nDelay;
            nFilterPartition++;
            }
        }
    }


   // do FFT of complete wave buffer
   m_pfft->Wave2Spec(vvafPartitions, m_vvacFrequencyResponse, false);

   // Forward and inverse transform of the signal will lose a factor
   // fftlength. Normalize by multiplying the frequency response with
   // fftlength.
   for (nFilterPartition = 0; nFilterPartition <  m_nFilterPartitions; nFilterPartition++)
      for (nFrame = 0; nFrame <  m_nFragSize+1; nFrame++)
         m_vvacFrequencyResponse[nFilterPartition][nFrame] *= float(nFragSize_2);

}
//--------------------------------------------------------------------------

//--------------------------------------------------------------------------
/// destructor. Does Cleanup
//--------------------------------------------------------------------------
CHtPartitionedConvolution::~CHtPartitionedConvolution()
{
   if (m_pfft)
      {
      delete m_pfft;
      m_pfft = NULL;
      }
}
//--------------------------------------------------------------------------


//--------------------------------------------------------------------------
/// Processing. Copies input data, calls DoProcess, copies output data back
/// \param[in] ppfSignalIn array with float pointers to input wave data
/// \param[in] ppfSignalOut array with float pointers to output wave data
/// \param[in] nChannels number of channels
/// \param[in] nFrames number of frames
//--------------------------------------------------------------------------
void CHtPartitionedConvolution::Process(  float * const * ppfSignalIn,
                                          float * const * ppfSignalOut,
                                          unsigned int nChannels,
                                          unsigned int nFrames)
{
   if (nChannels != m_nChannels)
      FFTERROR_STR_INT_INT("Input signal num_channels (%u) differs from m_nChannels (%u)", nChannels, m_nChannels);
   if (nFrames != m_nFragSize)
      FFTERROR_STR_INT_INT("Input signal num_frames (%u) differs from fragsize (%u)", nFrames, m_nFragSize);


   // replace half of the input buffer with the new input signal. The other half
   // buffer retains the signal of the previous call to this method.
   unsigned int nChannel;
   for (nChannel = 0; nChannel < nChannels; nChannel++)
      memcpy(&m_vvafWaveIn[nChannel][m_nFragSize * m_nWaveInBufferHalfCurrentIndex], ppfSignalIn[nChannel], nFrames*sizeof(float));

   // do filtering
   DoProcess();

   // copy output data back
   unsigned int nFrame;
   for (nFrame = 0; nFrame < nFrames; nFrame++)
      {
      for (nChannel = 0; nChannel < nChannels; nChannel++)
        {
        ppfSignalOut[nChannel][nFrame] = m_vvafWaveOut[nChannel][nFrame];
        }
    }
}
//--------------------------------------------------------------------------

//--------------------------------------------------------------------------
/// Processing. Copies input data, calls DoProcess, copies output data back
/// \param[in] vvafSignalIn vector of float valarrays with input wave data
/// \param[in] vvafSignalOut vector of float valarrays for output wave data
//--------------------------------------------------------------------------
void CHtPartitionedConvolution::Process(vvaf & vvafSignalIn, vvaf & vvafSignalOut)
{
   unsigned int nChannel;
   unsigned int nChannels = (unsigned int)vvafSignalIn.size();
   if (nChannels != m_nChannels)
      FFTERROR_STR_INT_INT("Input signal num_channels (%u) differs from m_nChannels (%u)", nChannels, m_nChannels);

   for (nChannel = 0; nChannel < nChannels; nChannel++)
      {
      if (vvafSignalIn[nChannel].size() != m_nFragSize)
         FFTERROR_STR_INT_INT("Input signal num_frames (%u) differs from fragsize (%u)", vvafSignalIn[nChannel].size(), m_nFragSize);
      }
   nChannels = (unsigned int)vvafSignalOut.size();
   if (nChannels != m_nChannels)
      FFTERROR_STR_INT_INT("Output signal num_channels (%u) differs from m_nChannels (%u)", nChannels, m_nChannels);

   for (nChannel = 0; nChannel < nChannels; nChannel++)
      {
      if (vvafSignalOut[nChannel].size() != m_nFragSize)
         FFTERROR_STR_INT_INT("Output signal num_frames (%u) differs from fragsize (%u)", vvafSignalOut[nChannel].size(), m_nFragSize);
      }

   // replace half of the input buffer with the new input signal.
   // The other half buffer retains the signal of the previous call
   // to this method.
   for (nChannel = 0; nChannel < nChannels; nChannel++)
      memcpy(&m_vvafWaveIn[nChannel][m_nFragSize * m_nWaveInBufferHalfCurrentIndex], &vvafSignalIn[nChannel][0], m_nFragSize*sizeof(float));

   // do filtering
   DoProcess();

   // copy output data back
   for (nChannel = 0; nChannel < nChannels; nChannel++)
      vvafSignalOut[nChannel] = m_vvafWaveOut[nChannel];
}
//--------------------------------------------------------------------------

//--------------------------------------------------------------------------
/// Processing. Calls Process(vvaf & vvafSignalIn, vvaf & vvafSignalOut)
/// for in place processing
/// \param[in] vvafSignal vector of float valarrays with input wave data and
/// for output wave data
//--------------------------------------------------------------------------
void CHtPartitionedConvolution::Process(vvaf & vvafSignal)
{
   Process(vvafSignal, vvafSignal);
}
//--------------------------------------------------------------------------

//--------------------------------------------------------------------------
/// Internal processing. Does FFT, filtering, IFFT
//--------------------------------------------------------------------------
void CHtPartitionedConvolution::DoProcess()
{
   // do the FFT
   m_pfft->Wave2Spec(m_vvafWaveIn, m_vvacSpecIn, bool(m_nWaveInBufferHalfCurrentIndex));

   unsigned int nFilterPartition, nChannel, nDelay, nFrame;

   // NOTE: speed tests were done for Borland compiler and gcc.
   // They showed for Borland
   //    - pointer access in inner loop over the frames is faster than vector array access
   //    - in place calculation  inner loop is faster than using temp variable
   // For gcc it was vice versa for both points!!
   #ifdef __BORLANDC__
   unsigned int nFrameIterator = m_nFragSize+1;
   CHtComplex *pcplx1, *pcplx2, *pcplx3;
   for (nFilterPartition = 0; nFilterPartition < m_nFilterPartitions; nFilterPartition++)
      {
      nChannel = m_fiBookKeeping[nFilterPartition].m_nChannelIndex;
      nDelay   = (m_fiBookKeeping[nFilterPartition].m_nDelay + m_nOutputPartitionCurrentIndex) % m_nOutputPartitions;
      pcplx1   = &(m_vvacSpecIn[nChannel][0]);
      pcplx2   = &(m_vvacFrequencyResponse[nFilterPartition][0]);
      pcplx3   = &(m_vvacSpecOut[nDelay][nChannel][0]);
      nFrame = nFrameIterator;
      while (nFrame--)
         {
         *pcplx3++  += *pcplx1++ * *pcplx2++;
         }
       }
   #else  // no Borland compiler
   CHtComplex ct;
   for (nFilterPartition = 0; nFilterPartition < m_nFilterPartitions; nFilterPartition++)
      {
      nChannel = m_fiBookKeeping[nFilterPartition].m_nChannelIndex;
      nDelay   = (m_fiBookKeeping[nFilterPartition].m_nDelay + m_nOutputPartitionCurrentIndex) % m_nOutputPartitions;
      // apply
      for (nFrame = 0; nFrame < m_nFragSize+1; nFrame++)
         {
         ct  = m_vvacSpecIn[nChannel][nFrame];
         ct *= m_vvacFrequencyResponse[nFilterPartition][nFrame];
         m_vvacSpecOut[nDelay][nChannel][nFrame] += ct;
         }
      }
   #endif // #ifdef __BORLANDC__

   // IFFT
   m_pfft->Spec2Wave(m_vvacSpecOut[m_nOutputPartitionCurrentIndex], m_vvafWaveOut);


    // clear output buffer for reuse
   unsigned nSize = sizeof(CHtComplex)*(m_nFragSize+1);
   for (nChannel = 0; nChannel < m_nChannels; nChannel++)
      memset(&m_vvacSpecOut[m_nOutputPartitionCurrentIndex][nChannel][0], 0, nSize);

    // update counters
   m_nOutputPartitionCurrentIndex++;
   if (m_nOutputPartitionCurrentIndex >= m_nOutputPartitions)
      m_nOutputPartitionCurrentIndex = 0;
   m_nWaveInBufferHalfCurrentIndex =
      1U - m_nWaveInBufferHalfCurrentIndex;
}
//--------------------------------------------------------------------------



