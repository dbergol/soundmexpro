//------------------------------------------------------------------------------
/// \file HtFFT.h
///
/// \author Berg
/// \brief Interface of FFTW-encapsulation class CHtFFT
///
/// Project SoundMexPro
/// Module  SoundDllPro.dll
/// Interface of FFTW-encapsulation class CHtFFT
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
#ifndef HtFFT3H
#define HtFFT3H
//--------------------------------------------------------------------------
// avoid warnings from FFTW
// avoid warnings from VST-SDK
#pragma clang diagnostic push
#pragma clang diagnostic ignored "-Wundef"
#include "fftw3.h"
#pragma clang diagnostic pop


#include <vector>
#include <valarray>
#include <complex>


/// include vcl only for Borland compiler
#include <vcl.h>

#define CHtComplex std::complex<float>
/// definition of valarray of floats
typedef std::valarray<float > vaf;
/// definition of valarray of complex numbers
typedef std::valarray<CHtComplex > vac;
/// definition of a vector of valarrays of floats
typedef std::vector<vaf > vvaf;
/// definition of a vector of valarrays of complex numbers
typedef std::vector<vac > vvac;


//--------------------------------------------------------------------------
/// Class encapsulating call to FFTW, prefix fft
//--------------------------------------------------------------------------
class CHtFFT
{
   friend class UNIT_TEST_CLASS;
   public:
      CHtFFT(unsigned int nFFTLen);
      ~CHtFFT();
      void Wave2Spec(const vvaf & vvafWave, vvac& vvacSpec, bool bSwap );
      void Spec2Wave(const vvac & vvacSpec, vvaf & vvafWave);
    private:
      unsigned int         m_nFFTLen;              /// FFTlength to use
      unsigned int         m_nRe;                  /// number of real values
      unsigned int         m_nIm;                  /// number of imaginary values
      float                m_fScale;               /// internal scaling
      std::valarray<float> m_vafBufIn;             /// input data buffer for FFTW
      std::valarray<float> m_vafBufOut;            /// output data buffer for FFTW
      fftwf_plan            m_fftw_plan_Wave2Spec;  /// plan for the FFTW for FFT, see FFTW documentataion
      fftwf_plan            m_fftw_plan_Spec2Wave;  /// plan for the FFTW for IFFT, see FFTW documentataion
      void SortFFTW2Spec(const vaf & vafFFTW, vvac & vvacSpec, unsigned int nChannel);
      void SortSpec2FFTW(vaf & vafFFTW, const vvac & vvacSpec, unsigned int nChannel);
      unsigned int GetNumFrames(const vvaf &vvafWave);
      void Cleanup();
      void InternalCheckSizes(const vaf  & vafFFTW,
                              const vvac & vvacSpec,
                              unsigned int nChannel);
};
//--------------------------------------------------------------------------
#endif // #ifndef HtFFTH
