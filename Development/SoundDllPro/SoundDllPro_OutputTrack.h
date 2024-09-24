//------------------------------------------------------------------------------
/// \file SoundDllPro_OutputTrack.h
/// \author Berg
/// \brief Implementation of class SDPTrack: a virtual track that may contain
/// multiple subsequent instances of SDPOutputData
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
#ifndef SoundDllPro_OutputTrackH
#define SoundDllPro_OutputTrackH

//------------------------------------------------------------------------------
#include "SoundDllPro_OutputChannelData.h"
//------------------------------------------------------------------------------
class SDPOutputData;

//------------------------------------------------------------------------------
/// \class SDPTrack. Virtual audio track containing SDPOutputData instances
//------------------------------------------------------------------------------
class SDPTrack
{
   public:
      SDPTrack(unsigned int nTrackIndex, unsigned int nChannelIndex, bool bAutoCleanup = true);
      ~SDPTrack();
      SDPOutputData* LoadAudio(SDPOD_AUDIO  &rsdpod);
      void           PushData();
      void           AssertDataAllowed();
      void           Cleanup(bool bAllInstances);
      bool           EndlessLoop();
      bool           HasData();
      bool           IsPlaying();
      bool           Underrun();
      void           Start();
      void           Stop();
      int64_t        RemainingDataLength();
      uint64_t       NumTrackSamples();
      unsigned int   TrackLoad();
      unsigned int   ChannelIndex();
      void           ChannelIndex(unsigned int nChannelIndex);
      bool           Multiply();
      void           Multiply(bool bMultiply);
      bool           AutoCleanup();
      void           AutoCleanup(bool bAutoCleanup);
      void           GetBuffer(std::valarray<float>& vafBuffer);
      void           SetPosition(uint64_t nPosition = 0);
      AnsiString     Name(void);
      void           Name(AnsiString strName);
      bool           m_bNotify;
   private:
      CRITICAL_SECTION  m_csLock;
      AnsiString        m_strName;
      unsigned int      m_nTrackIndex;
      unsigned int      m_nChannelIndex;
      bool              m_bMultiply;
      unsigned int      m_nDataUnderrun;
      int64_t           m_nDataUnderrunPos;
      bool              m_bEndlessLoop;
      bool              m_bAutoCleanup;
public:
      SDPOutputData*    m_psdpod[2];      // array with pointer to first and actual SDPOutputData
};
//------------------------------------------------------------------------------
#endif
