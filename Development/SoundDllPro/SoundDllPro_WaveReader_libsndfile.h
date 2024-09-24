/// \file SoundDllPro_WaveReader_libsndfile.h
/// \author Berg
/// \brief Implementation of class SDPWaveReader. Encapsulates buffered reading
/// of a single channel of a PCM wave file using a thread for non-blocking reading.
/// Uses libsndfile
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
#ifndef SoundDllPro_WaveReader_libsndfileH
#define SoundDllPro_WaveReader_libsndfileH
//---------------------------------------------------------------------------
#include <Classes.hpp>
#include <limits.h>
#include <vector>
#include <valarray>
#pragma clang diagnostic push
#pragma clang diagnostic ignored "-Wundef"
#include <sndfile.h>
#pragma clang diagnostic pop
#include "HanningWindow.h"
#include "SoundDllPro_OutputChannelData.h"
//---------------------------------------------------------------------------

#define WAVREAD_DEFAULTBUFSIZE   655360
#define WAVREAD_MINBUFSIZE       65536

//------------------------------------------------------------------------------
/// enumeration of buffer status values formats
//------------------------------------------------------------------------------
enum SDPWaveReaderBufferStatus
{
   SDP_WAVEREADERBUFFERSTATUS_FILLED = 0,
   SDP_WAVEREADERBUFFERSTATUS_DONE
};
//------------------------------------------------------------------------------

//------------------------------------------------------------------------------
/// enumeration of sync events
//------------------------------------------------------------------------------
enum SDPWaveReaderEvents
{
   SDP_WAVEREADEREVENT_LOAD = 0,
   SDP_WAVEREADEREVENT_STOP
};
//------------------------------------------------------------------------------

//------------------------------------------------------------------------------
/// helper class encapsulating one float buffer an it's status
//------------------------------------------------------------------------------
class SDPWaveReaderBuffer
{
   public:
      std::valarray<float>       m_vafBuffer;          /// float buffer containing one channel as normalized floats
      SDPWaveReaderBufferStatus  m_wrbStatus;          /// status of buffer (filled or done)
};
//------------------------------------------------------------------------------

//------------------------------------------------------------------------------
/// thread class for reading one channel of a wave file
//------------------------------------------------------------------------------
class SDPWaveReader : public TThread
{
   private:
      SDPWaveReaderBuffer        m_sdpWB[2];         /// two read buffers for ping-pong reading-writing
      CRITICAL_SECTION           m_csFile;

      AnsiString                 m_strFileName;    /// filename of wavefile to read
      SNDFILE*                   m_pSndFile;       /// file handle to wave file
      HANDLE                     m_hEvents[2];     /// events for thread synchronisation

      unsigned int               m_nBufferSize;    /// size of internal buffer(s)
      unsigned int               m_nFileChannel;   /// channel index to read
      unsigned int               m_nNumFileChannels;   /// total number of channels 

      unsigned int               m_nReadPos;       /// current read position within current SDPWaveReaderBuffer
      uint64_t                   m_nSamplesRead;   /// total number of samples read
      uint64_t                   m_nSamplesReadFromFile;   /// total number of samples read
      uint64_t                   m_nFilePos;       /// internal file pos
      bool                       m_bDone;          /// flag, if all data were read
      uint64_t                   m_nStartPos;      /// internal start read position within file
      uint64_t                   m_nFileSize;      /// file size in samples
      uint64_t                   m_nLength;        /// used length per loop
      int                        m_nReadBufIndex;  /// index of current buffer for reading in GetSample
      AnsiString                 m_strThreadError; /// string for thread func error
      uint64_t                   m_nFileOffset;       /// offset in file for snippet
      uint64_t                   m_nFileLastSample;   /// last sample to use for snippet
      bool                       m_bStarted;          /// flag, if any sample was ever retrieved
      uint64_t                   m_nCrossfadeOffset;  /// length of crossfade for looping
      uint64_t                   m_nTotalLength;      /// total playing length
      void                       ReadData();
   protected:
      void __fastcall Execute();
   public:
      __fastcall SDPWaveReader(  SDPOD_AUDIO &rsdpodFile,
                                 unsigned int      nDeviceSampleRate,
                                 unsigned int      nMinBufsize = 0,
                                 int               nThreadPriority = 2); // corresponds to tpHighest
      __fastcall ~SDPWaveReader();
      static unsigned int sm_nWaveReaderBufSize;
      AnsiString        GetFileName();
      unsigned int      GetFileChannel();
      uint64_t          GetStartOffset();
      uint64_t          GetFileOffset();
      uint64_t          GetSamplesRead();
      uint64_t          GetLoopPosition();
      float             GetFileSample();
      uint64_t          FileSize();
      uint64_t          TotalLength();
      bool              Done();
      bool              IsEndlessLoop();
      bool              Started();
      uint64_t          UsedLength();
      static void       WaveFileProperties(AnsiString strFileName,
                                          unsigned int &nNumChannels,
                                          unsigned int &nNumSamples,
                                          double       &dSampleRate
                                          );

      void              SetPosition(uint64_t nPosition);
};
//---------------------------------------------------------------------------
#endif
