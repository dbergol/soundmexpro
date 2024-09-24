//------------------------------------------------------------------------------
/// \file SoundDllPro_RecFile.h
/// \author Berg
/// \brief Implementation of classes SDPRecFile and PCMWaveFileHeader.
/// Simple classes to write a normalized 32bit float mono PCM wave file
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
#ifndef SoundDllPro_RecFileH
#define SoundDllPro_RecFileH

#include <vcl.h>
#include <vector>
#include <valarray>
//------------------------------------------------------------------------------

//------------------------------------------------------------------------------
/// structs as definitions for different wave header chunks
//------------------------------------------------------------------------------
#pragma pack(push, 1)
struct riff_hdr {
   char  id[4];         // identifier string = "RIFF"
   DWORD len;           // remaining length after this header
   char  typeStr[4];    // 4-byte data type ("WAVE")
};

struct chunk_hdr {      // CHUNK 8-byte header
   char  id[4];         // identifier, e.g. "fmt " or "data"
   DWORD len;           // remaining chunk(!) length after header, here == 16
};

struct wave_hdr {          //pcm-wave format chunk
   WORD  wFormatTag;       // Format category
   WORD  wChannels;        // Number of channels
   DWORD dwSamplesPerSec;  // Sampling rate
   DWORD dwAvgBytesPerSec; // For buffer estimation
   WORD  wBlockAlign;      // Data block size
   WORD  wBitsPerSample;   // Sample size (PCM specific!!)
};
#pragma pack(pop)
//------------------------------------------------------------------------------

//------------------------------------------------------------------------------
/// \class PCMWaveFileHeader. Simple helper class to create a valid PCM wave file
/// header
//------------------------------------------------------------------------------
class PCMWaveFileHeader
{
   public:
      PCMWaveFileHeader();
      void  SetDataSize_Byte(DWORD dwDataByteCount);
      void  SetDataSize_Sample(DWORD dwDataSampleCount);
      void  SetWaveFormat(WORD wChannels, DWORD dwSamplesPerSec, WORD wBitsPerSample, WORD wFormatTag = 3);
   private:
      riff_hdr       RiffHeader;
      chunk_hdr      fmtChunk;
      wave_hdr       pcmFormatRec;
      chunk_hdr      dataChunk;
};
//------------------------------------------------------------------------------

//------------------------------------------------------------------------------
/// \class SDPRecFile. Simple class to write a normalized 32bit float mono
/// PCM wave files
//------------------------------------------------------------------------------
class SDPRecFile
{
   public:
      SDPRecFile(unsigned int nSampleRate, int nChannelIndex, uint64_t nIgnoreSamples, bool bEnabled);
      ~SDPRecFile();
      void           OpenFile();
      void           OpenFile(UnicodeString usFileName);
      void           CloseFile();
      bool           IsOpen();
      void           WriteBuffer(std::valarray<float> &vaf);
      void           Reset();
      void           Clear();
      int64_t        Size();
      void           Enabled(bool bEnable);
      bool           Enabled();
      UnicodeString  FileName();
      void           FileName(UnicodeString usFileName);

      void              RecordLength(uint64_t nRecordLength);
      uint64_t          RecordLength(void);
   protected:
      PCMWaveFileHeader m_wfh;
      TFileStream       *m_pfs;
      UnicodeString     m_usFileName;
      unsigned int      m_nSampleRate;
      int               m_nChannelIndex;
      bool              m_bEnabled;
      uint64_t          m_nRecLength;
      uint64_t          m_nSamplesToIgnore;
      uint64_t          m_nIgnoreSamples;
      UnicodeString     FileAndChannelStr();
};
//------------------------------------------------------------------------------
#endif
