//------------------------------------------------------------------------------
/// \file SoundDllPro_OutputChannelData.h
/// \author Berg
/// \brief Implementation of class SDPOutputData describing output data either
/// as memory block or as file descriptor (SDPFileChannel). Support for an offser
/// (zeros to be returned prior to data) and looping. Used by SDPOutput
/// to create track audio data.
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
#ifndef SoundDllPro_OutputChannelDataH
#define SoundDllPro_OutputChannelDataH
//------------------------------------------------------------------------------

#include <vcl.h>
#include <stdint.h>
#include <valarray>
class SDPWaveReader;
//------------------------------------------------------------------------------
// base class (struct) for audio data
//------------------------------------------------------------------------------
class SDPOD_AUDIO
{
public:
   SDPOD_AUDIO() :
      bIsFile(false),
      nChannelIndex(0),
      nNumChannels(0),
      nNumSamples(0),
      nLoopCount(1),
      nOffset(0),
      nStartOffset(0),
      nFileOffset(0),
      nGlobalPosition(0),
      fGain(1.0f),
      pData(NULL),
      nRampLenght(0),
      nLoopRampLenght(0),
      bLoopCrossfade(false),
      nCrossfadeLengthLeft(0),
      nCrossfadeLengthRight(0)
      {;}
   // = operator
   SDPOD_AUDIO& operator=(const SDPOD_AUDIO& r);              
   #ifdef __clang__   
   #pragma clang diagnostic push
   #pragma clang diagnostic ignored "-Wc++98-compat"
   SDPOD_AUDIO(SDPOD_AUDIO const&) = default;
   #pragma clang diagnostic pop
   #endif
   bool              bIsFile;
   unsigned int      nChannelIndex;
   unsigned int      nNumChannels;
   uint64_t          nNumSamples;
   unsigned int      nLoopCount;
   uint64_t          nOffset;
   int64_t           nStartOffset;
   int64_t           nFileOffset;
   uint64_t          nGlobalPosition;
   float             fGain;
   AnsiString        strName;
   double*           pData;
   // ramp values
   unsigned int  nRampLenght;
   unsigned int  nLoopRampLenght;
   bool          bLoopCrossfade;
   // crossfade value between subsequent (!) SDPOD_AUDIO objects
   unsigned int  nCrossfadeLengthLeft;    // length of left crossfade ramp
   unsigned int  nCrossfadeLengthRight;   // length of right crossfade ramp
};
//------------------------------------------------------------------------------
//------------------------------------------------------------------------------
/// \class SDPOutputData class for storing audio data as memory block or file
/// descriptor
//------------------------------------------------------------------------------
class SDPOutputData
{
   public:
      SDPOutputData*       m_psdpodNext;              /// pointer to 'next' SDPOutputData instance
      SDPOD_AUDIO          m_sdopAudio;               /// info about audio snippet
      bool                 m_bIsInUse;                /// flag, if this SDPOutputData is (still) in use
      bool                 m_bReady;                  /// flag, if this SDPOutputData is ready for use!
      SDPOutputData(SDPOD_AUDIO &rsdpod, SDPOutputData* psdop = NULL);
      ~SDPOutputData();
      float                GetSample(bool bNoRamp = false);
      bool                 IsEndlessLoop();
      uint64_t             TotalLength();
      uint64_t             RemainingLength();
      unsigned int         LengthSingleLoop();
      void                 Offset(unsigned int nOffset);
      void                 SetGlobalPosition(uint64_t nGlobalPosition);
      void                 SetPosition(uint64_t nPosition = 0);
      uint64_t             GetPosition();
      AnsiString           Name();
      AnsiString           Id();
      bool                 IsFile();
      unsigned int         LoopCount();
      uint64_t             GetFileOffset();
      uint64_t             GetFileUsedLength();
      uint64_t             GetStartOffset();
      uint64_t             GetGlobalPosition();
      uint64_t             GetTotalLength();
   private:
      SDPWaveReader*       m_psdpwr;
      std::valarray<float> m_vafBuffer;
      std::valarray<float> m_vafCrossfadeBuffer;
      uint64_t             m_nPosition;
      uint64_t             m_nTotalLength;
      uint64_t             m_nTotalPosition;
      unsigned int         m_nSingleLoopSamples;
      AnsiString           m_strID;
      void                 InitializeMemWav(SDPOutputData* psdop);
      void                 InitializeFileWav();
      void                 InitializeRamps();
      AnsiString           InitializeGUID();
};
//------------------------------------------------------------------------------
#endif
