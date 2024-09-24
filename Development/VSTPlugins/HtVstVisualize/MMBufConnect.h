//------------------------------------------------------------------------------
/// \file MMBufConnect.h
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
#ifndef MMBufConnectH
#define MMBufConnectH
//---------------------------------------------------------------------------
#include <vcl.h>
#include <vector>
#include <valarray>
#include "MMConect.hpp"
//------------------------------------------------------------------------------

//------------------------------------------------------------------------------
/// \class TMMBufferConnector. Inherited TMMConnector to call visualization
/// components without any device
//------------------------------------------------------------------------------
class TMMBufferConnector : public TMMConnector
{
   public:
      __fastcall TMMBufferConnector(Classes::TComponent* AOwner);
      __fastcall ~TMMBufferConnector();
      __property unsigned int BufSize     = {read=GetBufSize, write=SetBufSize, default=64};
      __property unsigned int SampleRate  = {read=GetSampleRate, write=SetSampleRate, default=44100};
      __property unsigned int Channels    = {read=GetChannels, write=SetChannels, default=1};
      void __fastcall Init(void);
      void __fastcall Exit(void);
      void __fastcall BufferLoad(std::valarray<float> &vaf);
      void __fastcall BufferLoad(float *lpf, unsigned nNumSamples);
      void __fastcall BufferLoad(std::valarray<int> &vai);
      void __fastcall BufferLoad(int *lpi, unsigned nNumSamples);
      void __fastcall BufferLoad(Mmsystem::PWaveHdr lpwh, bool &MoreBuffers);
      inline bool IsInit(){return m_bInitialized;}

   protected:

   private:
      bool              m_bInitialized;
      wavehdr_tag       m_wavehdr;
      tWAVEFORMATEX     m_wfx;
      void __fastcall   SetChannels(unsigned int nChannels);
      void __fastcall   SetBufSize(unsigned int nBufferSize);
      void __fastcall   SetSampleRate(unsigned int nSampleRate);
      unsigned int  __fastcall  GetChannels(void);
      unsigned int  __fastcall  GetBufSize(void);
      unsigned int __fastcall   GetSampleRate(void);
};

#endif
