//------------------------------------------------------------------------------
/// \file SoundDllPro_InputChannel.cpp
/// \author Berg
/// \brief Implementation of class SDPInput describing an input channel. Includes
/// a ringbuffer for cyclic storing of recorded data in memory and storing of
/// recorded data to file. Used by SoundDllProMain class
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
#include <math.h>
#include "SoundDllPro_InputChannel.h"
#include "SoundDllPro_Main.h"
#include "SoundDllPro_RecFile.h"
//------------------------------------------------------------------------------

//------------------------------------------------------------------------------
/// constructor. Intialises members, buffers and recording file.
//------------------------------------------------------------------------------
SDPInput::SDPInput(unsigned int nChannelIndex, bool bDisableRecFile, unsigned int nDownSampleFactor)
   :  m_bSaveToFile(true),
      m_nChannelIndex(nChannelIndex),
      m_prfRecordFile(NULL),
      m_nBufPos(0),
      m_bStarted(false)
{
   if (!SoundClass())
      throw Exception("global SoundClass is invalid");

   m_strName = "Input " + IntToStr((int)m_nChannelIndex);

   m_vvafBuffer.resize(2);

   // create record file in disabled status
   if (!bDisableRecFile)
      {
      long lLatency  = 0;
      if (SoundClass()->m_bRecCompensateLatency &&  SoundClass()->GetRecDownSampleFactor() == 1)
         lLatency = (SoundClass()->SoundGetLatency(Asio::INPUT) + SoundClass()->SoundGetLatency(Asio::OUTPUT));

      m_prfRecordFile = new SDPRecFile((unsigned int)SoundClass()->SoundGetSampleRate()/nDownSampleFactor, (int)nChannelIndex, (uint64_t)lLatency, true);
      // set default record filename
      m_prfRecordFile->FileName("rec_" + IntToStr((int)nChannelIndex) + ".wav");
      }
}
//------------------------------------------------------------------------------

//------------------------------------------------------------------------------
/// dstructor. Does cleanup
//------------------------------------------------------------------------------
SDPInput::~SDPInput()
{
   Stop();
   TRYDELETENULL(m_prfRecordFile);
}
//------------------------------------------------------------------------------

//------------------------------------------------------------------------------
/// sets buffersiz of internal ringbuffer for cyclic storing of record data
//------------------------------------------------------------------------------
void SDPInput::BufferSize(unsigned int nBufSize)
{
   if (!SoundClass())
      throw Exception("global SoundClass is invalid");

   if (SoundClass()->DeviceIsRunning())
      throw Exception("cannot set buffer size while device is running");

   if (nBufSize != 0 && nBufSize < (unsigned int)SoundClass()->SoundBufsizeSamples())
      nBufSize = (unsigned int)SoundClass()->SoundBufsizeSamples();

   m_vvafBuffer[0].resize(nBufSize);
   m_vvafBuffer[1].resize(nBufSize);
   if (nBufSize)
      {
      m_vvafBuffer[0] = 0.0f;
      m_vvafBuffer[1] = 0.0f;
      }
}
//------------------------------------------------------------------------------

//------------------------------------------------------------------------------
/// retuens ringbuffer's buffer size
//------------------------------------------------------------------------------
unsigned int SDPInput::BufferSize(void)
{
   return (unsigned int)m_vvafBuffer[0].size();
}
//------------------------------------------------------------------------------

//------------------------------------------------------------------------------
/// Sets internal record length. Recording to file will be stopped after corresponding
/// number of samples is recorded.
//------------------------------------------------------------------------------
void SDPInput::RecordLength(unsigned int nRecordLength)
{
   if (!!m_prfRecordFile)
      m_prfRecordFile->RecordLength(nRecordLength);
}
//------------------------------------------------------------------------------

//------------------------------------------------------------------------------
/// returns internal record length.
//------------------------------------------------------------------------------
unsigned int SDPInput::RecordLength(void)
{
   if (!m_prfRecordFile)
      return 0;
   return (unsigned int)m_prfRecordFile->RecordLength();
}
//------------------------------------------------------------------------------

//------------------------------------------------------------------------------
/// Starts recording to memory and file
//------------------------------------------------------------------------------
void SDPInput::Start()
{
   if (m_vvafBuffer[0].size())
      {
      m_vvafBuffer[0] = 0.0f;
      m_vvafBuffer[1] = 0.0f;
      }

   m_nBufPos = 0;
   m_bStarted = false;
   if (!!m_prfRecordFile)
      {
      if (!m_prfRecordFile->IsOpen())
         m_prfRecordFile->OpenFile();
      }
}
//------------------------------------------------------------------------------

//------------------------------------------------------------------------------
/// closes recording file
//------------------------------------------------------------------------------
void SDPInput::Stop()
{
   if (!!m_prfRecordFile)
      {
      EnterCriticalSection(&SoundClass()->m_csBufferDone);
      try
         {
         m_prfRecordFile->CloseFile();
         }
      __finally
         {
         LeaveCriticalSection(&SoundClass()->m_csBufferDone);
         }
      }
}
//------------------------------------------------------------------------------

//------------------------------------------------------------------------------
/// Toggles recording to file
//------------------------------------------------------------------------------
void SDPInput::Pause(bool bDoPause, bool bCloseFileOnPause)
{
   if (!m_prfRecordFile)
      return;

   if (bDoPause)
      {
      if (bCloseFileOnPause)
         Stop();
      else
         m_prfRecordFile->Enabled(false);
      }
   else
      {
      if (!m_prfRecordFile->IsOpen())
         Start();
      else
         m_prfRecordFile->Enabled(true);
      }
}
//------------------------------------------------------------------------------

//------------------------------------------------------------------------------
/// returns current pause status
//------------------------------------------------------------------------------
bool SDPInput::Pause()
{
   if (!m_prfRecordFile)
      return true;
   return !m_prfRecordFile->Enabled();
}
//------------------------------------------------------------------------------

//------------------------------------------------------------------------------
/// returns true, if data are recorded to file at the moment
//------------------------------------------------------------------------------
bool SDPInput::IsRecording()
{
   if (!m_prfRecordFile)
      return false;
   return m_bSaveToFile && m_prfRecordFile->IsOpen() && m_prfRecordFile->Enabled();
}
//------------------------------------------------------------------------------

//------------------------------------------------------------------------------
/// Saves a buffer to memory and/or file.
/// MUST BE SYNCED E.G. WITH CRITICAL SECTION BY CALLER (see GetBuffer)
//------------------------------------------------------------------------------
void SDPInput::SaveBuffer(std::valarray<float> &vaf, bool bSaveToFile)
{
   unsigned int nSize = (unsigned int)vaf.size();
   // do we have to save to a buffer?
   if (m_vvafBuffer[0].size())
      {
      unsigned int n = (unsigned int)m_vvafBuffer[0].size() - m_nBufPos;
      if (n < nSize)
         {
         // write to it end
         CopyMemory(&m_vvafBuffer[0][m_nBufPos], &vaf[0], n * sizeof(float));
         // write to start
         CopyMemory(&m_vvafBuffer[0][0], &vaf[n], (nSize - n) * sizeof(float));
         m_nBufPos = nSize - n;
         }
      else
         {
         CopyMemory(&m_vvafBuffer[0][m_nBufPos], &vaf[0], nSize * sizeof(float));
         m_nBufPos += nSize;
         }
      if (m_nBufPos >= m_vvafBuffer[0].size())
         m_nBufPos = 0;
      }
   if (!bSaveToFile)
      return;

   if (!m_prfRecordFile)
      return;

   if (m_bSaveToFile && m_prfRecordFile->IsOpen() && m_prfRecordFile->Enabled())
      {
      m_bStarted = true;

      // save data
      m_prfRecordFile->WriteBuffer(vaf);

      // if file is disabled _afterwards_, then stop it (then record length
      // is exceeded and m_prfRecordFile has disabled itself)
      if (!m_prfRecordFile->Enabled() && m_prfRecordFile->Size() >= (int64_t)m_prfRecordFile->RecordLength())
         {
         Stop();
         // now enabled file again even if it's closed: otherwise restart becomes impossible
         m_prfRecordFile->Enabled(true);
         }
      }
}
//------------------------------------------------------------------------------

//------------------------------------------------------------------------------
/// returns reference to last ringbuffer. NOTE: it creates a copy of the data
/// flipping them to correct order.
/// MUST BE SYNCED E.G. WITH CRITICAL SECTION BY CALLER (see SaveBuffer)
//------------------------------------------------------------------------------
std::valarray<float>& SDPInput::GetBuffer()
{
   unsigned int nSize = (unsigned int)m_vvafBuffer[0].size();
   if (!nSize)
      throw Exception("no buffers recorded");

   // copy data from m_vvafBuffer[0] to m_vvafBuffer[1] with 'flipping' them
   // to have correct time course!

   // 1: copy from current position to end
   CopyMemory(&m_vvafBuffer[1][0], &m_vvafBuffer[0][m_nBufPos], (size_t)((nSize - m_nBufPos) * sizeof(float)));
   // 2: copy from beginning to current pos
   CopyMemory(&m_vvafBuffer[1][nSize - m_nBufPos], &m_vvafBuffer[0][0], (size_t)(m_nBufPos * sizeof(float)));

   return m_vvafBuffer[1];
}
//------------------------------------------------------------------------------

//------------------------------------------------------------------------------
/// returns number of samples recorded to file
//------------------------------------------------------------------------------
int64_t  SDPInput::RecPosition()
{
   if (!m_prfRecordFile)
      return 0;
   return m_prfRecordFile->Size();
}
//------------------------------------------------------------------------------

//------------------------------------------------------------------------------
/// toggles save-to-file status
//------------------------------------------------------------------------------
void SDPInput::SaveToFile(bool bEnable)
{
   if (bEnable)
      Start();
   else
      Stop();
   m_bSaveToFile = bEnable;
}
//------------------------------------------------------------------------------

//------------------------------------------------------------------------------
/// returns current save-to-file status
//------------------------------------------------------------------------------
bool SDPInput::SaveToFile()
{
   return m_bSaveToFile;
}
//------------------------------------------------------------------------------

//------------------------------------------------------------------------------
/// sets filename of reord file
//------------------------------------------------------------------------------
void SDPInput::FileName(AnsiString sFileName)
{
   if (!m_prfRecordFile)
      return;
   // NEW FEATURE: change filename by closing/opening file. This is only allowed,
   // if file is disabled at the moment!
   if (m_prfRecordFile->IsOpen() && m_prfRecordFile->Enabled())
      throw Exception("setting record filename is not allowed if recording to file is active");

   if (m_prfRecordFile->IsOpen())
      Stop();

   m_prfRecordFile->FileName(sFileName);

   Start();
}
//------------------------------------------------------------------------------

//------------------------------------------------------------------------------
/// returns filename of reord file
//------------------------------------------------------------------------------
AnsiString SDPInput::FileName(void)
{
   if (!m_prfRecordFile)
      return "";
   return m_prfRecordFile->FileName();
}
//------------------------------------------------------------------------------

//------------------------------------------------------------------------------
/// returns true, if recording was ever started 
//------------------------------------------------------------------------------
bool SDPInput::Started()
{
   return m_bStarted;
}
//------------------------------------------------------------------------------

//------------------------------------------------------------------------------
/// returns input name
//------------------------------------------------------------------------------
AnsiString SDPInput::Name(void)
{
   return m_strName;
}
//------------------------------------------------------------------------------

//------------------------------------------------------------------------------
/// sets input name
//------------------------------------------------------------------------------
void SDPInput::Name(AnsiString strName)
{
   m_strName = strName;
}
//------------------------------------------------------------------------------


