//------------------------------------------------------------------------------
/// \file HtOLA.h
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
#ifndef HtOLAH
#define HtOLAH
//--------------------------------------------------------------------------
#include <HtFFT3.h>

//------------------------------------------------------------------------------
/// \typedef LPFNSOUNDPROCESS callback for sound modules
//-----------------------------------------------------------------------------
typedef void (__closure *LPFNHTOLAPROC)(vvac & vvacSpectrum);

int ConvertToPowerOfTwo(int n, int* pnOrder = NULL);
void GenerateWindow(std::valarray<float> &rvaf, unsigned int nLen);

#ifndef TRYDELETNULL
//------------------------------------------------------------------------------
/// \define TRYDELETENULL
/// \brief small helper function for calling delete on a pointer with
/// try...catch and setting pointer to NULL
//------------------------------------------------------------------------------
#define TRYDELETENULL(p)   \
   {                       \
   if (p)                  \
      {                    \
      try                  \
         {                 \
         delete p;         \
         }                 \
      catch (...)          \
         {                 \
         }                 \
      p = NULL;            \
      }                    \
   }                       \
//------------------------------------------------------------------------------
#endif

//--------------------------------------------------------------------------
/// Class encapsulating call to FFTW, prefix fft
//--------------------------------------------------------------------------
class CHtOLA
{
   friend class UNIT_TEST_CLASS;
   public:
      CHtOLA(  unsigned int nChannels,
               unsigned int nFFTLen,
               unsigned int nBufSize,
               LPFNHTOLAPROC lpfn
               );
      ~CHtOLA();
      void DoOLA(vvaf& rvvaf);
      void Reset();
    private:
      _RTL_CRITICAL_SECTION   m_cs;
      LPFNHTOLAPROC           m_lpfnProc;
      unsigned int            m_nChannels;
      unsigned int            m_nBufSize;
      unsigned int            m_nFFTLen;
      unsigned int            m_nWindowLen;
      unsigned int            m_nWindowShift;
      unsigned int            m_nZeroPadding;
      unsigned int            m_nOverlapLen;
      CHtFFT*                 m_pFFT;
      unsigned int            m_nBufInPos;
      unsigned int            m_nBufOutPos;
      vvaf                    m_vvafBufIn;
      vvaf                    m_vvafBufOut;
      vvaf                    m_vvafBufOverlap;
      vvaf                    m_vvafBufFFT;
      vvac                    m_vacSpec;    // buffer for spectrum calculation using CHtFFT
      std::valarray<float >   m_vafPreWindow;
      std::valarray<float >   m_vafPostWindow;
      float                   m_f4IFFTScalingFactor;


      void                    Initialize(unsigned int nChannels, unsigned int nFFTLen, unsigned int nBufSize);
      void                    Cleanup();
};
//--------------------------------------------------------------------------
#endif // #ifndef HtOLAH
