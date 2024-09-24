//------------------------------------------------------------------------------
/// \file MMBufConnect.cpp
/// \author Berg
/// \brief Implementation of class TMMBufferConnector Class inherits from
/// MMTools TMMConnector and implements an MMTools visual component connector
/// then can be called directly (without device) with value buffers or vectors.
///
/// Project SoundMexPro
/// Module  SoundDllPro.dll
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

#pragma hdrstop
#include <windowsx.h>
#include <math.h>
#include <limits.h>
#include "MMBufConnect.h"

#pragma package(smart_init)
#pragma warn -use
//------------------------------------------------------------------------------

//------------------------------------------------------------------------------
/// constants for scaling wave data
//------------------------------------------------------------------------------
const float g_fScaleShort  = -(float)SHRT_MIN;
const int   g_fScaleLong   = USHRT_MAX + 1;
//------------------------------------------------------------------------------


//------------------------------------------------------------------------------
/// constructor. sets membervers and initializes internal wave format 
//------------------------------------------------------------------------------
__fastcall TMMBufferConnector::TMMBufferConnector(Classes::TComponent* AOwner)
: TMMConnector(AOwner), m_bInitialized(false)
{
   RealTime                = false;
   RefreshOnStop           = true;
   m_wfx.wFormatTag        = 1;
   m_wfx.nChannels         = 1;
   m_wfx.nBlockAlign       = sizeof(short);
   m_wfx.wBitsPerSample    = 16;
   m_wfx.cbSize            = sizeof(tWAVEFORMATEX);
   m_wfx.nSamplesPerSec    = 0;
   m_wfx.nAvgBytesPerSec   = 0;

   m_wavehdr.dwBufferLength   = 0;
   m_wavehdr.dwBytesRecorded  = 0;
   m_wavehdr.dwUser           = 0;
   m_wavehdr.lpData           = NULL;
   m_wavehdr.lpNext           = NULL;
   m_wavehdr.dwUser           = 0;

   SetBufSize(64);
   SetSampleRate(44100);
}
//------------------------------------------------------------------------------

//------------------------------------------------------------------------------
/// destructor. Does cleanup
//------------------------------------------------------------------------------
__fastcall TMMBufferConnector::~TMMBufferConnector()
{
   if (m_wavehdr.lpData)
      GlobalFreePtr(m_wavehdr.lpData);
}
//------------------------------------------------------------------------------


//------------------------------------------------------------------------------
/// returns number of channels
//------------------------------------------------------------------------------
unsigned int __fastcall TMMBufferConnector::GetChannels(void)
{
   return m_wfx.nChannels;
}
//------------------------------------------------------------------------------

//------------------------------------------------------------------------------
// sets number of channels
//------------------------------------------------------------------------------
void __fastcall TMMBufferConnector::SetChannels(unsigned int nChannels)
{
   if (m_bInitialized)
      throw Exception("cannot change channels while running");
   if (nChannels == (unsigned int)m_wfx.nChannels || nChannels < 1 || nChannels > 2)
      return;
   m_wfx.nChannels         = (WORD)nChannels;
   m_wfx.nBlockAlign       = (WORD)(m_wfx.nChannels * sizeof(short int));
   m_wfx.nAvgBytesPerSec   = m_wfx.nSamplesPerSec * m_wfx.nChannels * sizeof(short int);
   SetPWaveFormat(&m_wfx);
}
//------------------------------------------------------------------------------

//------------------------------------------------------------------------------
/// returns internal buffer size
//------------------------------------------------------------------------------
unsigned int __fastcall TMMBufferConnector::GetBufSize(void)
{
   return m_wavehdr.dwBufferLength;
}
//------------------------------------------------------------------------------

//------------------------------------------------------------------------------
/// sets internal buffer sizes
//------------------------------------------------------------------------------
void __fastcall TMMBufferConnector::SetBufSize(unsigned int nBufferSize)
{
   if (m_bInitialized)
      throw Exception("cannot change buffersize while running");
   if (nBufferSize == m_wavehdr.dwBufferLength || nBufferSize < 64)
      return;

   if (m_wavehdr.lpData)
      GlobalFreePtr(m_wavehdr.lpData);
   m_wavehdr.lpData = (char*)GlobalAllocPtr(GMEM_FIXED, nBufferSize);

   m_wavehdr.dwBufferLength   = nBufferSize;
   m_wavehdr.dwBytesRecorded  = nBufferSize;
   m_wavehdr.dwUser           = 0;
   SetBufferSize(nBufferSize);
}
//------------------------------------------------------------------------------

//------------------------------------------------------------------------------
/// returns internal samplerate
//------------------------------------------------------------------------------
unsigned int __fastcall TMMBufferConnector::GetSampleRate(void)
{
   return m_wfx.nSamplesPerSec;
}
//------------------------------------------------------------------------------

//------------------------------------------------------------------------------
/// sets internal samplerate
//------------------------------------------------------------------------------
void __fastcall TMMBufferConnector::SetSampleRate(unsigned int nSampleRate)
{
   if (m_bInitialized)
      throw Exception("cannot change samplerate while running");
   if (nSampleRate < 8000 || nSampleRate == m_wfx.nSamplesPerSec)
      return;

   m_wfx.nSamplesPerSec    = nSampleRate;
   m_wfx.nAvgBytesPerSec   = m_wfx.nSamplesPerSec * m_wfx.nChannels * sizeof(short int);
   SetPWaveFormat(&m_wfx);
}
//------------------------------------------------------------------------------

//------------------------------------------------------------------------------
/// initializes class
//------------------------------------------------------------------------------
void __fastcall TMMBufferConnector::Init(void)
{
   if (m_bInitialized)
      Exit();
   TMMConnector::Opened();
   SetPWaveFormat(&m_wfx);
   TMMConnector::Started();
   m_bInitialized = true;
}
//------------------------------------------------------------------------------

//------------------------------------------------------------------------------
/// exits class
//------------------------------------------------------------------------------
void __fastcall TMMBufferConnector::Exit(void)
{
   if (m_bInitialized)
      {
      m_bInitialized = false;
      TMMConnector::Stopped();
      TMMConnector::Closed();
      }
}
//------------------------------------------------------------------------------

//------------------------------------------------------------------------------
/// calls BufferLoad(float *lpf, unsigned nNumSamples)
//------------------------------------------------------------------------------
void __fastcall TMMBufferConnector::BufferLoad(std::valarray<float> &vaf)
{
   BufferLoad(&vaf[0], vaf.size());
}
//------------------------------------------------------------------------------

//------------------------------------------------------------------------------
/// scales data and calls base class
//------------------------------------------------------------------------------
void __fastcall TMMBufferConnector::BufferLoad(float *lpf, unsigned nNumSamples)
{

   if (Enabled && m_bInitialized && m_wavehdr.lpData && lpf)
      {
      bool MoreBuffers;
      short int *lpData          = (short int*)m_wavehdr.lpData;
      unsigned int nBufferSize   = m_wavehdr.dwBufferLength / sizeof(short int);
      if (nBufferSize > m_wavehdr.dwBufferLength)
         nBufferSize = m_wavehdr.dwBufferLength;
      unsigned int nPos = 0;
      while (nPos < nNumSamples)
         {
         lpData[m_wavehdr.dwUser++] = g_fScaleShort * lpf[nPos++];
         if (m_wavehdr.dwUser == nBufferSize)
            {
            m_wavehdr.dwUser = 0;
            // on stopping an error might happen: ignore it
            try
               {
               TMMConnector::BufferLoad(&m_wavehdr, MoreBuffers);
               }
            catch (...)
               {
               OutputDebugString(__FUNC__ ": ingnored error");
               }
            }
         }
      }
}
//------------------------------------------------------------------------------

//------------------------------------------------------------------------------
/// calls TMMBufferConnector::BufferLoad(int *lpi, unsigned nNumSamples)
//------------------------------------------------------------------------------
void __fastcall TMMBufferConnector::BufferLoad(std::valarray<int> &vai)
{
   BufferLoad(&vai[0], vai.size());
}
//------------------------------------------------------------------------------

//------------------------------------------------------------------------------
/// scales data and calls base class
//------------------------------------------------------------------------------
void __fastcall TMMBufferConnector::BufferLoad(int *lpi, unsigned nNumSamples)
{
   if (Enabled && m_bInitialized && m_wavehdr.lpData)
      {
      bool MoreBuffers;
      short int *lpData          = (short int*)m_wavehdr.lpData;
      unsigned int nBufferSize   = m_wavehdr.dwBufferLength / sizeof(short int);
      unsigned int nPos = 0;
      while (nPos < nNumSamples)
         {
         lpData[m_wavehdr.dwUser++] = (short int)(lpi[nPos++] / g_fScaleLong);
         if (m_wavehdr.dwUser == nBufferSize)
            {
            m_wavehdr.dwUser = 0;
            TMMConnector::BufferLoad(&m_wavehdr, MoreBuffers);
            }
         }
      }
}
//------------------------------------------------------------------------------

//------------------------------------------------------------------------------
/// calls baseclass
//------------------------------------------------------------------------------
void __fastcall TMMBufferConnector::BufferLoad(Mmsystem::PWaveHdr lpwh, bool &MoreBuffers)
{
   if (Enabled)
      TMMConnector::BufferLoad(lpwh, MoreBuffers);
}
//------------------------------------------------------------------------------


