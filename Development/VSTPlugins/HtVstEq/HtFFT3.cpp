//------------------------------------------------------------------------------
/// \file HtFFT.cpp
///
/// \author Berg
/// \brief Implementation of FFTW-encapsulation class CHtFFT
///
/// Project SoundMexPro
/// Module  SoundDllPro.dll
/// Implementation of FFTW-encapsulation class CHtFFT
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
#include "HtFFT3.h"

//--------------------------------------------------------------------------


//--------------------------------------------------------------------------
/// Constructor. Intializes members, data buffers and FFTW
/// \exception CHtException if FFTW and CHtFFT use different precisions
/// \exception Exception if an FFT-length < 2 is specified
//--------------------------------------------------------------------------
CHtFFT::CHtFFT(unsigned int nFFTLen)
   :  m_nFFTLen(nFFTLen),
      m_vafBufIn(0.0f, nFFTLen),
      m_vafBufOut(0.0f, nFFTLen),
      m_fftw_plan_Wave2Spec(NULL),
      m_fftw_plan_Spec2Wave(NULL)
{
   #pragma option push -w-8008 -w-8066
//   if (sizeof(float) != sizeof(fftw_real))
//      throw Exception("FFT-Wrapper and FFTW use different precision");
   #pragma option pop
   if (nFFTLen < 2)
      throw Exception("fft length must be >= 2");


   // number of real parts (+1 for nyquist frequency)
   m_nRe = nFFTLen/2 + 1;


   // number of imaginary parts
   // none for 0 Hz and none for nyquist frequency ...
   m_nIm = nFFTLen/2 - 1;
   // ... but one more for odd fft length!
   if ((nFFTLen % 2) != 0)
      m_nIm++;

   m_fScale = 1.0f / (float)nFFTLen;
   try
      {
      m_fftw_plan_Wave2Spec = fftwf_plan_r2r_1d ((int)m_nFFTLen, &m_vafBufIn[0], &m_vafBufOut[0], FFTW_R2HC , FFTW_ESTIMATE);
      m_fftw_plan_Spec2Wave = fftwf_plan_r2r_1d ((int)m_nFFTLen, &m_vafBufIn[0], &m_vafBufOut[0], FFTW_HC2R, FFTW_ESTIMATE);
      }
   catch (...)
      {
      Cleanup();
      throw;
      }
}
//--------------------------------------------------------------------------

//--------------------------------------------------------------------------
/// Destructor. Calls Cleanup
//--------------------------------------------------------------------------
CHtFFT::~CHtFFT(  )
{
   Cleanup();
}
//--------------------------------------------------------------------------

//--------------------------------------------------------------------------
/// Does cleanup of allocated/created data
//--------------------------------------------------------------------------
void CHtFFT::Cleanup()
{
   if (m_fftw_plan_Wave2Spec)
      {
      // secure silent cleanup
      try
         {
         fftwf_destroy_plan(m_fftw_plan_Wave2Spec);
         }
      catch (...)
         {
         }
      m_fftw_plan_Wave2Spec = NULL;
      }
   if (m_fftw_plan_Wave2Spec)
      {
      // secure silent cleanup
      try
         {
         fftwf_destroy_plan(m_fftw_plan_Wave2Spec);
         }
      catch (...)
         {
         }
      m_fftw_plan_Wave2Spec = NULL;
      }
}
//--------------------------------------------------------------------------


//--------------------------------------------------------------------------
/// Arrange the order of an FFTW spectrum to the internal order:
/// The FFTW spectrum is arranged [r0 r1 r2 ... rn-1 rn in-1 ... i1],
/// while the interal order is [r0 -- r1 i1 r2 i2 ... rn-1 in-1 rn --].
/// \param[in] vafFFTW float valarray containing FFTW-type spectrum
/// \param[out] vvacSpec vector of complex valarrays containing internal spectrum
/// \param[in] nChannel channel number of destination channel in vvacSpec
/// \exception Exception if dimensions are invalid
//--------------------------------------------------------------------------
void CHtFFT::SortFFTW2Spec(const vaf & vafFFTW, vvac & vvacSpec, unsigned int nChannel)
{
   // check vector sizes.
   InternalCheckSizes(vafFFTW, vvacSpec, nChannel);

   // write zeroes to complete channel
   memset(&vvacSpec[nChannel][0], 0, vvacSpec[nChannel].size()*sizeof(CHtComplex));
   unsigned int nFrame;
   for (nFrame = 0; nFrame < m_nRe; nFrame++)
      vvacSpec[nChannel][nFrame].real(vafFFTW[nFrame]);
   for (nFrame = 1; nFrame < m_nIm + 1; nFrame++)
      vvacSpec[nChannel][nFrame].imag(vafFFTW[m_nFFTLen - nFrame]);
}
//--------------------------------------------------------------------------

//--------------------------------------------------------------------------
/// Arrange the order of an internal order to the FFTW spectrum:
/// The FFTW spectrum is arranged [r0 r1 r2 ... rn-1 rn in-1 ... i1],
/// while the interal order is [r0 -- r1 i1 r2 i2 ... rn-1 in-1 rn --].
/// \param[out] vafFFTW float valarray containing FFTW-type spectrum
/// \param[in] vvacSpec vector of complex valarrays containing internal spectrum
/// \param[in] nChannel channel number of source channel in vvacSpec
/// \exception Exception if dimensions are invalid
//--------------------------------------------------------------------------
void CHtFFT::SortSpec2FFTW(vaf & vafFFTW, const vvac & vvacSpec, unsigned int nChannel)
{
   // check vector sizes.
   InternalCheckSizes(vafFFTW, vvacSpec, nChannel);

   unsigned int nFrame;
   for (nFrame = 0; nFrame < m_nRe; nFrame++)
      vafFFTW[nFrame] = vvacSpec[nChannel][nFrame].real();
   for (nFrame = 1; nFrame < m_nIm + 1; nFrame++)
      vafFFTW[m_nFFTLen - nFrame] = vvacSpec[nChannel][nFrame].imag();
}
//--------------------------------------------------------------------------

//--------------------------------------------------------------------------
/// Does FFT.
/// \param[in] vvafWave vector of float valarrays containing the source waveform
/// \param[out] vvacSpec vector of complex valarrays receiving spectrum
/// \param[in] bSwap if true the two input wave data halves are swapped
/// \exception Exception if dimensions are invalid/mismatching
//--------------------------------------------------------------------------
void CHtFFT::Wave2Spec(const vvaf & vvafWave, vvac & vvacSpec, bool bSwap)
{
   unsigned int nChannels = (unsigned int)vvafWave.size();
   if (nChannels != vvacSpec.size())
      throw Exception(UnicodeString().sprintf(L"channel number mismatch (spec has %u channels, wave has %u channels)", vvacSpec.size(), nChannels));

   unsigned nFrames = GetNumFrames(vvafWave);
   if (nFrames != m_nFFTLen)
      throw Exception(UnicodeString().sprintf(L"waveform has invalid length (%u, FFTLen: %u)", nFrames, m_nFFTLen));

   unsigned int nFrame, nChannel;
   for (nChannel = 0; nChannel < nChannels; nChannel++)
      {
      // copy values to internal buffer with respect to 'swap'-flag
      if (bSwap)
         {
         for (nFrame = 0; nFrame < nFrames; nFrame++)
            m_vafBufIn[(nFrame + nFrames / 2) % nFrames] = m_fScale * vvafWave[nChannel][nFrame];
         }
      else
         {
         for (nFrame = 0; nFrame < nFrames; nFrame++)
            m_vafBufIn[nFrame] = m_fScale * vvafWave[nChannel][nFrame];
         }

      // call FFT
      fftwf_execute(m_fftw_plan_Wave2Spec);
      // sort fftw-type to internal order
      SortFFTW2Spec(m_vafBufOut, vvacSpec, nChannel);
      }
}
//--------------------------------------------------------------------------

//--------------------------------------------------------------------------
/// Does IFFT.
/// \param[out] vvafWave vector of float valarrays receiving the waveform
/// \param[in] vvacSpec vector of complex valarrays containing source spectrum
/// \exception Exception if dimensions are invalid/mismatching
//--------------------------------------------------------------------------
void CHtFFT::Spec2Wave(const vvac &vvacSpec, vvaf &vvafWave)
{
   unsigned int nChannels = (unsigned int)vvafWave.size();
   if (nChannels != vvacSpec.size())
      throw Exception(UnicodeString().sprintf(L"channel number mismatch (spec has %u channels, wave has %u channels)", vvacSpec.size(), nChannels));

   unsigned nFrames = GetNumFrames(vvafWave);

   if (nFrames > m_nFFTLen)
      throw Exception(UnicodeString().sprintf(L"waveform has invalid length (%u, FFTLen: %u)", nFrames, m_nFFTLen));

   unsigned int nChannel, nFrame;
   for (nChannel = 0; nChannel < nChannels; nChannel++)
      {
      // sort internal order to fftw-type
      SortSpec2FFTW(m_vafBufIn, vvacSpec, nChannel);
      // call ifft
      fftwf_execute(m_fftw_plan_Spec2Wave);
      // copy data back to output buffer
      for (nFrame = 0; nFrame < nFrames; nFrame++)
         vvafWave[nChannel][nFrame] = m_vafBufOut[nFrame];
      }
}
//--------------------------------------------------------------------------

//--------------------------------------------------------------------------
/// Returns the number of values in each valarrays of a vector of valarrays.
/// \param[in] vvafWave reference to vecor of vallarrays
/// \retval number of values in each valarray
/// \exception Exception if not all valarrays contain the identical number of values
//--------------------------------------------------------------------------
unsigned int CHtFFT::GetNumFrames(const vvaf &vvafWave)
{
   unsigned int nChannels  = (unsigned int)vvafWave.size();
   if (!nChannels)
      return nChannels;
   unsigned int nFrames = (unsigned int)vvafWave[0].size();
   unsigned nChannel;
   for (nChannel = 1; nChannel < nChannels; nChannel++)
      {
      if (vvafWave[nChannel].size() != nFrames)
         throw Exception("internal error: wave channels with different sample number detected");
      }
   return nFrames;
}
//--------------------------------------------------------------------------

//--------------------------------------------------------------------------
/// Checks sizing constraints of passed vectors and returns number of complex
/// numbers contained in vvacSpec[nChannel] on success
/// \param[in] vafFFTW float valarray
/// \param[in] vvacSpec vector of complex valarrays
/// \param[in] nChannel channel number in vvacSpec
/// \retval returns number of complex contained in vvacSpec[nChannel] 
/// \exception Exception, if one of the following constraints is violated
/// - size of vafFFTW must be 2*internel FFT length
/// - vvacSpec must have at a sizeof at least nChannel
/// - vvacSpec[nChannel] must contain at least m_nRe complex values
/// - vvacSpec[nChannel] must contain at least m_nIm+1 complex values
//--------------------------------------------------------------------------
void CHtFFT::InternalCheckSizes( const vaf & vafFFTW,
                                 const vvac & vvacSpec,
                                 unsigned int nChannel)
{
   // NOTE: this is an internal constraint of class (passed buffers are always
   // m_vafBufIn or m_vafBufOut, sizes set in constructor)
   if (vafFFTW.size() != m_nFFTLen)
      throw Exception("internal error: fftw spectrum buffer has wrong size");

   if (vvacSpec.size() <= nChannel)
      throw Exception(UnicodeString().sprintf(L"Input spectrum contains only %u channels, but index %u is requested.", vvacSpec.size(), nChannel));

   unsigned int nFrames = (unsigned int)vvacSpec[nChannel].size();
   if (nFrames < m_nRe)
      throw Exception(UnicodeString().sprintf(L"Input spectrum contains only %u bins, but %u real parts are available.", nFrames, m_nRe));

   if (nFrames < m_nIm + 1)
      throw Exception(UnicodeString().sprintf(L"Input spectrum contains only %u bins, but %u imaginary parts are available.", nFrames, m_nIm));
}
//--------------------------------------------------------------------------
