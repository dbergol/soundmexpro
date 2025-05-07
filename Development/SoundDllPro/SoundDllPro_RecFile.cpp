//------------------------------------------------------------------------------
/// \file SoundDllPro_RecFile.cpp
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
#include "SoundDllPro_RecFile.h"
#ifndef TRYDELETENULL
   #define TRYDELETENULL(p) {if (p!=NULL) { try {delete p;} catch (...){;} p = NULL;}}
#endif
//------------------------------------------------------------------------------

//------------------------------------------------------------------------------
/// constructor. Intitlizes members
//------------------------------------------------------------------------------
SDPRecFile::SDPRecFile( unsigned int nSampleRate,
                        int nChannelIndex,
                        uint64_t nIgnoreSamples,
                        bool bEnabled)
   :  m_pfs(NULL),
      m_nSampleRate(nSampleRate),
      m_nChannelIndex(nChannelIndex),
      m_bEnabled(bEnabled),
      m_nRecLength(0),
      m_nSamplesToIgnore(nIgnoreSamples),
      m_nIgnoreSamples(nIgnoreSamples)
{
}
//------------------------------------------------------------------------------

//------------------------------------------------------------------------------
/// destructor. Does cleanup. 
//------------------------------------------------------------------------------
SDPRecFile::~SDPRecFile()
{
   if (m_pfs)
      CloseFile();
   TRYDELETENULL(m_pfs);
}
//------------------------------------------------------------------------------

//------------------------------------------------------------------------------
/// Sets internal enabled-flag
//------------------------------------------------------------------------------
void SDPRecFile::Enabled(bool bEnable)
{
   m_bEnabled = bEnable;
}
//------------------------------------------------------------------------------

//------------------------------------------------------------------------------
/// Returns internal enabled-flag
//------------------------------------------------------------------------------
bool SDPRecFile::Enabled()
{
   return m_bEnabled;
}
//------------------------------------------------------------------------------

//------------------------------------------------------------------------------
/// Returns internal filename
//------------------------------------------------------------------------------
UnicodeString SDPRecFile::FileName()
{
   return m_usFileName;
}
//------------------------------------------------------------------------------

//------------------------------------------------------------------------------
/// Sets internal filename
//------------------------------------------------------------------------------
void SDPRecFile::FileName(UnicodeString usFileName)
{
   if (!!m_pfs)
      throw Exception("cannot set filename while writing");
   m_usFileName = usFileName;
}
//------------------------------------------------------------------------------

//------------------------------------------------------------------------------
/// Returns internal recording length in samples
//------------------------------------------------------------------------------
uint64_t  SDPRecFile::RecordLength(void)
{
   return m_nRecLength;
}
//------------------------------------------------------------------------------

//------------------------------------------------------------------------------
/// Sets internal recording length in samples
//------------------------------------------------------------------------------
void SDPRecFile::RecordLength(uint64_t nRecordLength)
{
   if (!!m_pfs && m_bEnabled)
      throw Exception("cannot set record length while writing");
   if (!!m_pfs && nRecordLength > (uint64_t)Size())
      throw Exception("cannot set record length: requested size is smaller than already recorded samples");
   m_nRecLength = nRecordLength;
}
//------------------------------------------------------------------------------

//------------------------------------------------------------------------------
/// Opens filestream and writes valid PCMWaveHeader to it. If already done,
/// it simply returns. 
//------------------------------------------------------------------------------
void SDPRecFile::OpenFile()
{
   if (m_pfs)
      return;
   if (m_usFileName.IsEmpty())
      throw Exception("cannot open file for writing: filename not set (" + IntToStr(m_nChannelIndex) + ")");
   try
      {
      // reset latenty compensation
      m_nIgnoreSamples = m_nSamplesToIgnore;
      m_pfs = new TFileStream(m_usFileName, fmCreate | fmShareDenyWrite);
      m_wfh.SetWaveFormat(1, m_nSampleRate, 32, 3);
      m_pfs->WriteBuffer((const void*)&m_wfh, sizeof(PCMWaveFileHeader));
      }
   catch (Exception &e)
      {
      TRYDELETENULL(m_pfs);
      throw Exception("cannot create wave file for saving audio data: " + FileAndChannelStr() +  "(" + e.Message + ")");
      }
}
//------------------------------------------------------------------------------

//------------------------------------------------------------------------------
/// sets internal filename and calls OpenFile()
//------------------------------------------------------------------------------
void SDPRecFile::OpenFile(UnicodeString usFileName)
{
   FileName(usFileName);
   OpenFile();
}
//------------------------------------------------------------------------------

//------------------------------------------------------------------------------
/// Writes current correct size informations to PCMWaveHeader of file and closes
/// file
//------------------------------------------------------------------------------
void SDPRecFile::CloseFile()
{
   if (m_pfs)
      {
      try
         {
         m_wfh.SetDataSize_Byte((DWORD)(m_pfs->Size - (DWORD)sizeof(PCMWaveFileHeader)));
         m_pfs->Seek(0, soFromBeginning);
         m_pfs->WriteBuffer((const void*)&m_wfh, sizeof(PCMWaveFileHeader));
         }
      __finally
         {
         TRYDELETENULL(m_pfs);
         }
      }
}
//------------------------------------------------------------------------------

//------------------------------------------------------------------------------
/// returns open-status of file
//------------------------------------------------------------------------------
bool SDPRecFile::IsOpen()
{
   return !!m_pfs;
}
//------------------------------------------------------------------------------


//------------------------------------------------------------------------------
/// writes floating point buffer to file
//------------------------------------------------------------------------------
void SDPRecFile::WriteBuffer(std::valarray<float> &vaf)
{
   if (!m_bEnabled || !m_pfs)
      return;

   unsigned int nNumSamples = (unsigned int)vaf.size();
   unsigned int nReadPos = 0;


   if (m_nIgnoreSamples > 0)
      {
      // complete buffer to be ignored?
      if (m_nIgnoreSamples >= nNumSamples)
         {
         m_nIgnoreSamples -= nNumSamples;
         return;
         }
      nNumSamples -= (unsigned int)m_nIgnoreSamples;
      nReadPos += (unsigned int)m_nIgnoreSamples;
      m_nIgnoreSamples = 0;
      }


   if (!!m_nRecLength)
      {
      if ((uint64_t)Size() + nNumSamples > m_nRecLength)
         {
         m_bEnabled = false;
         nNumSamples =(unsigned int) (m_nRecLength - (uint64_t)Size());
         }
      }
   m_pfs->WriteBuffer(&vaf[nReadPos], (int)(nNumSamples*sizeof(float)));
}
//------------------------------------------------------------------------------

//------------------------------------------------------------------------------
/// clears and reopens file
//------------------------------------------------------------------------------
void SDPRecFile::Reset()
{
   Clear();
   OpenFile();
}
//------------------------------------------------------------------------------

//------------------------------------------------------------------------------
/// clears file
//------------------------------------------------------------------------------
void SDPRecFile::Clear()
{
   TRYDELETENULL(m_pfs);
}
//------------------------------------------------------------------------------

//------------------------------------------------------------------------------
/// returns current size in samples 
//------------------------------------------------------------------------------
int64_t SDPRecFile::Size()
{
   if (!m_pfs)
      return 0;
   return (m_pfs->Size - (int64_t)sizeof(PCMWaveFileHeader)) / (int64_t)sizeof(float);
}
//------------------------------------------------------------------------------

//------------------------------------------------------------------------------
// constructor. Sets initial values only. See *.H for explanations
//------------------------------------------------------------------------------
PCMWaveFileHeader::PCMWaveFileHeader()
{
   RiffHeader.id[0] = 'R';RiffHeader.id[1] = 'I';RiffHeader.id[2] = 'F';RiffHeader.id[3] = 'F';
   RiffHeader.len = 0;
   RiffHeader.typeStr[0] = 'W';RiffHeader.typeStr[1] = 'A';RiffHeader.typeStr[2] = 'V';RiffHeader.typeStr[3] = 'E';

   fmtChunk.id[0] = 'f';fmtChunk.id[1] = 'm';fmtChunk.id[2] = 't';fmtChunk.id[3] = ' ';
   fmtChunk.len = 16;// sizeof(wave_hdr);// 16;


   dataChunk.id[0] = 'd';dataChunk.id[1] = 'a';dataChunk.id[2] = 't';dataChunk.id[3] = 'a';
   dataChunk.len = 0;

   //setting default wave format: mono, 44kHz, 32bit, format tag 3 (normalized float data)
   SetWaveFormat(1, 44100, 32, 3);
}
//------------------------------------------------------------------------------

//------------------------------------------------------------------------------
// Set DataSize by total byte count
//------------------------------------------------------------------------------
void PCMWaveFileHeader::SetDataSize_Byte(DWORD dwDataByteCount)
{
   // writing datasize
   dataChunk.len  = dwDataByteCount;
   // writing size after RiffHeader
   RiffHeader.len = dataChunk.len + sizeof(chunk_hdr) + sizeof(wave_hdr) + sizeof(riff_hdr) ;
}
//------------------------------------------------------------------------------

//------------------------------------------------------------------------------
// Set DataSize by total sample count
//------------------------------------------------------------------------------
void PCMWaveFileHeader::SetDataSize_Sample(DWORD dwDataSampleCount)
{
   // writing datasize
   dataChunk.len  =  dwDataSampleCount * pcmFormatRec.wChannels
                     * (DWORD)(div(pcmFormatRec.wBitsPerSample,8).quot);
   // writing size after RiffHeader
   RiffHeader.len =  dataChunk.len + sizeof(chunk_hdr) + sizeof(wave_hdr) + sizeof(riff_hdr) ;
}
//------------------------------------------------------------------------------

//------------------------------------------------------------------------------
// setting wave format using channel count, sample count and bit resolution
//------------------------------------------------------------------------------
void  PCMWaveFileHeader::SetWaveFormat(WORD wChannels, DWORD dwSamplesPerSec, WORD wBitsPerSample, WORD wFormatTag)
{
   pcmFormatRec.wChannels          = wChannels;
   pcmFormatRec.dwSamplesPerSec    = dwSamplesPerSec;
   pcmFormatRec.wBitsPerSample     = wBitsPerSample;
   pcmFormatRec.dwAvgBytesPerSec   = wChannels * (DWORD)div(wBitsPerSample,8).quot * dwSamplesPerSec;
   pcmFormatRec.wBlockAlign        = (WORD)(wChannels * div(wBitsPerSample,8).quot);
   pcmFormatRec.wFormatTag         = wFormatTag;
}
//------------------------------------------------------------------------------

//------------------------------------------------------------------------------
// returns current filename and channel as string
//------------------------------------------------------------------------------
UnicodeString SDPRecFile::FileAndChannelStr()
{
   UnicodeString us = "file '" + m_usFileName + "', channel " + IntToStr(m_nChannelIndex);
   return us;
}
//------------------------------------------------------------------------------

