//------------------------------------------------------------------------------
/// \file HtOLA.cpp
///
/// \author Berg
/// \brief Implementation of Overlapped-Add-Filter using class CHtFFT
///
/// Project SoundMexPro
/// Module  HtVSTEq.dll
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
#include "HtOLA.h"
#include <math.h>

//--------------------------------------------------------------------------

//---------------------------------------------------------------------------
/// tool function converting a number to a power of 2
//---------------------------------------------------------------------------
int ConvertToPowerOfTwo(int n, int* pnOrder)
{
   int nOrder = 0;
   while (n > 1)
      {
      n = n >> 1;
      nOrder++;
      }
   if (nOrder > 0)
      n = n << nOrder;
   if (pnOrder)
      *pnOrder = nOrder;
   return n;
}
//---------------------------------------------------------------------------

//---------------------------------------------------------------------------
/// gerenerats hanning window in passed valarray
//---------------------------------------------------------------------------
void GenerateWindow(std::valarray<float> &rvaf, unsigned int nLen)
{
   rvaf.resize(nLen);
   unsigned int n;
   for (n = 0; n < nLen; n++)
      rvaf[n] = 0.5f - 0.5f*(float)cos(2.0*M_PI*(double)n/(double)nLen);
}
//---------------------------------------------------------------------------


//--------------------------------------------------------------------------
/// Constructor. Intializes critical section and clls Initialize
//--------------------------------------------------------------------------
CHtOLA::CHtOLA(unsigned int nChannels, unsigned int nFFTLen, unsigned int nBufSize, LPFNHTOLAPROC lpfn)
   :  m_pFFT(NULL)
{
   if (!lpfn)
      throw Exception("invalid spectrum callback passed");
   m_lpfnProc = lpfn;
   InitializeCriticalSection(&m_cs);
   Initialize(nChannels, nFFTLen, nBufSize);
}
//--------------------------------------------------------------------------

//--------------------------------------------------------------------------
/// Destructor. Does Cleanup
//--------------------------------------------------------------------------
CHtOLA::~CHtOLA()
{
   Cleanup();
   DeleteCriticalSection(&m_cs);
}
//--------------------------------------------------------------------------

//--------------------------------------------------------------------------
/// Does Cleanup
//--------------------------------------------------------------------------
void CHtOLA::Cleanup()
{
   TRYDELETENULL(m_pFFT);
}
//--------------------------------------------------------------------------

//--------------------------------------------------------------------------
/// Intializes members, data buffers and FFTW
//--------------------------------------------------------------------------
void CHtOLA::Initialize(unsigned int nChannels, unsigned int nFFTLen, unsigned int nBufSize)
{
   Cleanup();
   try
      {
      m_nChannels    = nChannels;
      m_nFFTLen      = (unsigned int)ConvertToPowerOfTwo((int)nFFTLen);

      if (!m_nFFTLen || !nChannels)
         throw Exception("FFTLenght and Channel count must be > 0");

      m_nWindowLen   = 400 * m_nFFTLen / 512;
      if ((m_nWindowLen % 2) == 1)
         m_nWindowLen--;
      m_nWindowShift = m_nWindowLen / 2;

      m_nBufSize     = nBufSize;
      if (m_nBufSize < m_nFFTLen)
         m_nBufSize = m_nFFTLen;

      m_nZeroPadding = (m_nFFTLen - m_nWindowLen) / 2;
      m_nOverlapLen  = m_nFFTLen - m_nWindowShift;


      m_vvafBufIn.resize(nChannels);
      m_vvafBufOut.resize(nChannels);
      m_vvafBufOverlap.resize(nChannels);
      m_vvafBufFFT.resize(nChannels);
      m_vacSpec.resize(nChannels);
      unsigned int n;
      for (n = 0; n < nChannels; n++)
         {
         m_vvafBufIn[n].resize(2*nBufSize);
         m_vvafBufOut[n].resize(2*nBufSize);
         m_vvafBufOverlap[n].resize(m_nOverlapLen);
         m_vvafBufFFT[n].resize(m_nFFTLen);
         m_vacSpec[n].resize(m_nFFTLen/2+1);
         }

      // generate Pre-Window
      GenerateWindow(m_vafPreWindow, m_nWindowLen);

      // calculate the scalingfactor for the spectrum:
      // - respect to zero-padding: scale intensity by wndlen/fftlen
      // - windowing: sum(W^2)/wndlen = 0.375 for hanning window
      // - adjusting to power density rather than power: scale intensity by fftlen
      // - scale by sqrt(N) for FFT
      // - another sqrt(2) to keep intensity rather than amplitude
      // this leads to a total factor of
      //     sqrt(0.375*wndlen*fftlen)
      // float f4SpectrumScaling = sqrt(0.375*0.5*(float)m_nWindowLen*(float)m_nFFTLen);
      // m_vafPreWindow /= f4SpectrumScaling;

      // generate Post-Window
      GenerateWindow(m_vafPostWindow, 2*m_nZeroPadding);

      // for IFFT we have to scale back _and_ we have to take care for WindowFeed's other than
      // wfHalf. The Hanning window will keep energy the same for wfHalf but for higher WindowFeeds we
      // have to devide the wave data by WindowFeeds/2 !!! So we calculate the correction value here
      // WindowFeed is
      //   pflt->WindowFeed       = pflt->WindowLength / pflt->WindowShift;
      // so the total IFFT factor is
      // m_f4IFFTScalingFactor = 2.0f/(float)(m_nWindowLen / m_nWindowShift) * f4SpectrumScaling;

      // create FFT
      m_pFFT = new CHtFFT(m_nFFTLen);
      Reset();
      }
   catch (...)
      {
      Cleanup();
      throw;
      }


}
//--------------------------------------------------------------------------

//--------------------------------------------------------------------------
/// resets data buffers
//--------------------------------------------------------------------------
void CHtOLA::Reset()
{
   EnterCriticalSection(&m_cs);
   try
      {
      unsigned int n;
      for (n = 0; n < m_nChannels; n++)
         {
         m_vvafBufIn[n]    = 0.0f;
         m_vvafBufOut[n]   = 0.0f;
         m_vvafBufFFT[n]   = 0.0f;
         m_vvafBufOverlap[n] = 0.0f;
         m_vacSpec[n] = 0;
         }

      m_nBufInPos    = 0;
      // set output position to 'pre-load' filter with zeroes for latency compensation
      m_nBufOutPos   = m_nWindowLen + m_nZeroPadding;
      }
   __finally
      {
      LeaveCriticalSection(&m_cs);
      }
}
//--------------------------------------------------------------------------


//--------------------------------------------------------------------------
/// Does OLA......
//--------------------------------------------------------------------------
void CHtOLA::DoOLA(vvaf& rvvaf)
{
   EnterCriticalSection(&m_cs);

   try
      {
      // first: consistancy checks and copy input data
      if (rvvaf.size() != m_nChannels)
         {
         OutputDebugString("DoOLA: invalid number of channels passed");
         return;
         }
      unsigned int nSamples = (unsigned int)rvvaf[0].size();

      if (m_nBufInPos > nSamples + m_vvafBufIn[0].size())
         {
         OutputDebugString("DoOLA: try to write behind end of input buffer");
         return;
         }


      unsigned int nChannel;
      // copy input data to internal buffer
      for (nChannel = 0; nChannel < m_nChannels; nChannel++)
         CopyMemory(&m_vvafBufIn[nChannel][m_nBufInPos], &rvvaf[nChannel][0], sizeof(float)*nSamples);

      m_nBufInPos += nSamples;

      // determine number of FFTs
      unsigned int nTotalNumFFT = 0;
      if (m_nBufInPos >= m_nWindowLen)
         nTotalNumFFT = (m_nBufInPos - m_nWindowLen) / m_nWindowShift + 1;

      //----------------------------------------------------------------------------------------------------------------------------------------
      // iteration through buffers!
      // - write zeros and data*windowfunction to m_vvafBufFFT
      // - hand this buffer as ComplexArry (re, im, re, im...) to ComplexFFT
      // - extract spectra from complex fft and hand them to spectrum callback
      // - rebuild complex spectrum and hand it to complex IFFT
      // - apply window to zeros and do OLA
      // - hand wave data to wave callback
      // - write reconstructed data back
      //----------------------------------------------------------------------------------------------------------------------------------------
      unsigned int nReadPos = 0;
      unsigned int nNumFFT, nSample;
      for (nNumFFT = 0; nNumFFT < nTotalNumFFT; nNumFFT++)
         {
         for (nChannel = 0; nChannel < m_nChannels; nChannel++)
            {
            // clear FFT buffer (write zeros!)
            m_vvafBufFFT[nChannel] = 0.0f;
            // copy audio data
            CopyMemory(&m_vvafBufFFT[nChannel][m_nZeroPadding], &m_vvafBufIn[nChannel][nReadPos], sizeof(float)*m_nWindowLen);

            // apply window
            for (nSample = 0; nSample < m_nWindowLen; nSample++)
               m_vvafBufFFT[nChannel][m_nZeroPadding+nSample] *= m_vafPreWindow[nSample];
            }

         // do the FFT
         m_pFFT->Wave2Spec(m_vvafBufFFT, m_vacSpec, false);

         // call user function
         m_lpfnProc(m_vacSpec);

         // do IFFT
         m_pFFT->Spec2Wave(m_vacSpec, m_vvafBufFFT);


         for (nChannel = 0; nChannel < m_nChannels; nChannel++)
            {
            // apply back window
            for (nSample = 0; nSample < m_nZeroPadding; nSample++)
               {
               m_vvafBufFFT[nChannel][nSample] *= m_vafPostWindow[nSample];
               m_vvafBufFFT[nChannel][m_nFFTLen-nSample-1] *= m_vafPostWindow[2*m_nZeroPadding-nSample-1];
               }

            // do the overlap add
            for (nSample = 0; nSample < m_nOverlapLen; nSample++)
               m_vvafBufFFT[nChannel][nSample] += m_vvafBufOverlap[nChannel][nSample];

            // copy new data to overlap buffer for next run
            CopyMemory(&m_vvafBufOverlap[nChannel][0], &m_vvafBufFFT[nChannel][m_nFFTLen - m_nOverlapLen], sizeof(float)*m_nOverlapLen);

            // write data to out buffer
            CopyMemory(&m_vvafBufOut[nChannel][m_nBufOutPos], &m_vvafBufFFT[nChannel][0], sizeof(float)*m_nWindowShift);
            } // channels

         // adjust read position by m_nWindowShift (half of  windowlen (overlap))
         nReadPos       += m_nWindowShift;
         m_nBufOutPos   += m_nWindowShift;
         }   // num ffts

      // final channel loop
      // - move remaining input data to the beginning of the input buffer
      // - write output data
      int nRemainIn  = (int)(m_nBufInPos - nTotalNumFFT * m_nWindowShift);
      int nRemainOut = (int)(m_nBufOutPos - nSamples);
      for (nChannel = 0; nChannel < m_nChannels; nChannel++)
         {
         if (nRemainIn > 0)
            MoveMemory(&m_vvafBufIn[nChannel][0], &m_vvafBufIn[nChannel][nTotalNumFFT * m_nWindowShift], sizeof(float)*(size_t)nRemainIn);

         // one full buffer available?
         if (m_nBufOutPos < nSamples)
            {
            // No: write zeroes and available samples
            rvvaf[nChannel] = 0.0f;
            if (m_nBufOutPos)
               CopyMemory(&rvvaf[nChannel][nSamples-m_nBufOutPos-1], &m_vvafBufOut[nChannel][0], sizeof(float)*m_nBufOutPos);
            m_nBufOutPos = 0;
            }
         else
            {
            // copy data
            CopyMemory(&rvvaf[nChannel][0], &m_vvafBufOut[nChannel][0], sizeof(float)*nSamples);
            // move remaining output data to the beginning of the output buffer
            if (nRemainOut > 0)
               MoveMemory(&m_vvafBufOut[nChannel][0], &m_vvafBufOut[nChannel][nSamples], sizeof(float)*(size_t)nRemainOut);

            }
         }

      m_nBufOutPos -= nSamples;
      // adjust input buffer pos
      m_nBufInPos = (unsigned int)nRemainIn;
      }
   __finally
      {
      LeaveCriticalSection(&m_cs);
      }
}
//------------------------------------------------------------------------------
