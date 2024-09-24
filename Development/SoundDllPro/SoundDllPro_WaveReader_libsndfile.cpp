//------------------------------------------------------------------------------
/// \file SoundDllPro_WaveReader_libsndfile.cpp
/// \author Berg
/// \brief Implementation of class SDPWaveReader. Encapsulates buffered reading
/// of a single channel of a PCM wave file using a thread for non-blocking reading.
/// Uses libsndfile
///
/// Project SoundMexPro
/// Module  SoundDllPro.dll
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
#include <vcl.h>
#include <windowsx.h>
#pragma hdrstop

#include "SoundDllPro_WaveReader_libsndfile.h"
#include "SoundDllPro_Tools.h"
#pragma package(smart_init)
//---------------------------------------------------------------------------


//------------------------------------------------------------------------------
/// size in samples used for file buffering (64kB)
//------------------------------------------------------------------------------
unsigned int SDPWaveReader::sm_nWaveReaderBufSize = WAVREAD_DEFAULTBUFSIZE;

//------------------------------------------------------------------------------
/// constructor. Initializes members, opens file, checks file format, starts
/// thread and waits until first data buffer is read.
//------------------------------------------------------------------------------
__fastcall SDPWaveReader::SDPWaveReader(  SDPOD_AUDIO       &rsdpodFile,
                                          unsigned int      nDeviceSampleRate,
                                          unsigned int      nMinBufsize,
                                          int               nThreadPriority)
   : TThread(true),
     m_strFileName(rsdpodFile.strName),
     m_pSndFile(NULL),
     m_nBufferSize(nMinBufsize),
     m_nFileChannel(rsdpodFile.nChannelIndex),
     m_nNumFileChannels(0),
     m_nReadPos(0),
     m_nSamplesRead(0),
     m_nSamplesReadFromFile(0),
     m_nFilePos(0),
     m_bDone(false),
     m_nStartPos(0),
     m_nFileSize(0),
     m_nLength(0),
     m_nReadBufIndex(0),
     m_nFileOffset(0),
     m_nFileLastSample(0),
     m_bStarted(false),
     m_nCrossfadeOffset(0),
     m_nTotalLength(0)
{
   Priority = nThreadPriority == 3 ? tpTimeCritical : tpHighest;

   m_hEvents[0] = NULL;
   m_hEvents[1] = NULL;
   // create named events non-signaled and auto-resetting
   for (int i = 0; i < 2; i++)
      {
      m_hEvents[i]    = CreateEvent(NULL, FALSE, FALSE, NULL);
      if (!m_hEvents[i])
         throw Exception("error creating file reader events");
      }

   //WriteDebugString("start");
   FreeOnTerminate = false;
   if (!FileExists(m_strFileName))
      throw Exception("file not found");

   try
      {
      InitializeCriticalSection(&m_csFile);

      SF_INFO sfi;
      ZeroMemory(&sfi, sizeof(sfi));

      m_pSndFile = sf_open(m_strFileName.c_str(), SFM_READ, &sfi);

      if (!m_pSndFile)
         throw Exception("error opening file");

      // check, that channel is within range
      m_nNumFileChannels = (unsigned int)sfi.channels;
      if (m_nFileChannel >= m_nNumFileChannels)
         throw Exception("requested channel is out of range (requested: "
                        + IntToStr((int)m_nFileChannel)
                        + ", available: "
                        + IntToStr((int)m_nNumFileChannels)
                        + ")");


      // samplerate identical to device samplerate?
      if (nDeviceSampleRate != (unsigned int)sfi.samplerate)
         throw Exception(  "samplerate is "
                        + IntToStr((int)sfi.samplerate)
                        + ", but device was initialized with samplerate "
                        + IntToStr((int)nDeviceSampleRate));

      if (!sfi.seekable)
         throw Exception("sound file cannot be used: not seekable");

      m_nFileSize    = (uint64_t)sfi.frames;
      if (!m_nFileSize)
         throw Exception("file does not contain samples");

      // first check length parameter
      if (rsdpodFile.nNumSamples > m_nFileSize)
         throw Exception("specified length exceeds file length");
      m_nLength = rsdpodFile.nNumSamples;

      // then file offset: this is the offset to be used for generating a snippet.
      // if offset is specified and length is 0, then we use length until end
      if ((uint64_t)rsdpodFile.nFileOffset >= m_nFileSize)
         throw Exception("file offset exceeds file length");
      m_nFileOffset  = (uint64_t)rsdpodFile.nFileOffset;
      if (!!m_nFileOffset && m_nLength == 0)
         m_nLength = m_nFileSize - m_nFileOffset;
      if (m_nLength > m_nFileSize - m_nFileOffset)
         throw Exception("specified length exceeds file length - fileoffset");

      m_nStartPos = (uint64_t)rsdpodFile.nStartOffset;

      // caculate total length of object including offsets and looping
      // length == 0 means: use complete file
      if (m_nLength == 0)
         {
         m_nTotalLength = m_nFileSize * rsdpodFile.nLoopCount  - m_nStartPos;
         if (!rsdpodFile.nLoopCount)
            m_nTotalLength = 0;
         }
      else
         {
         // calculate last sample to use within file
         m_nFileLastSample = m_nFileOffset + m_nLength;
         // check for looped snippet
         if (m_nFileLastSample > m_nFileSize)
            m_nFileLastSample -= m_nFileSize;
         if (rsdpodFile.nLoopCount == 1)
            m_nTotalLength = m_nLength;
         else
            m_nTotalLength = m_nLength * rsdpodFile.nLoopCount - m_nStartPos;
         }


      // id crossfade is done, then we have to store crossfade length and adjust
      // total length member
      if (rsdpodFile.nLoopRampLenght && rsdpodFile.bLoopCrossfade)
         {
         m_nCrossfadeOffset = rsdpodFile.nLoopRampLenght;
         m_nTotalLength -= ((rsdpodFile.nLoopCount-1) * rsdpodFile.nLoopRampLenght);
         }


      // use passed buffer size, but at least 640 kB totally for performance reasons
      if (m_nNumFileChannels*m_nBufferSize < sm_nWaveReaderBufSize)
         m_nBufferSize = sm_nWaveReaderBufSize/m_nNumFileChannels;

      // create two float buffers for ping-pong filling
      for (int i = 0; i < 2; i++)
         {
         // NOTE: we read all channels
         m_sdpWB[i].m_vafBuffer.resize(m_nNumFileChannels*m_nBufferSize);
         m_sdpWB[i].m_wrbStatus = SDP_WAVEREADERBUFFERSTATUS_DONE;
         }
      }
   catch (Exception &e)
      {
      // tell thread to stop
      if (!!m_hEvents[SDP_WAVEREADEREVENT_STOP])
         {
         if (!SetEvent(m_hEvents[SDP_WAVEREADEREVENT_STOP]))
            throw Exception("error setting stop event");
         }

      if (m_pSndFile)
         sf_close(m_pSndFile);
      m_pSndFile = NULL;
      AnsiString str = "error loading file '" + ExpandFileName(m_strFileName) + "': " + e.Message;
      DeleteCriticalSection(&m_csFile);
      throw Exception(str);
      }
}
//------------------------------------------------------------------------------

//------------------------------------------------------------------------------
/// destructor. Stops thread and does cleanup
//------------------------------------------------------------------------------
__fastcall SDPWaveReader::~SDPWaveReader()
{
   // tell thread to stop: NOTE: no exception here to happen in destructor:
   // thus ignore any error....
   bool bStopEventSuccess = SetEvent(m_hEvents[SDP_WAVEREADEREVENT_STOP]);

   Terminate();            
   //wait only on 'regular' stopping
   if (bStopEventSuccess)
      WaitFor();

   if (m_pSndFile)
      sf_close(m_pSndFile);
   m_pSndFile = NULL;

   for (int i = 0; i < 2; i++)
      {
      if (m_hEvents[i] != NULL)
         {
         CloseHandle(m_hEvents[i]);
         m_hEvents[i] = NULL;
         }
      }
   DeleteCriticalSection(&m_csFile);
}
//------------------------------------------------------------------------------

//------------------------------------------------------------------------------
/// thread function. Reads data (if necessary)
//------------------------------------------------------------------------------
void __fastcall SDPWaveReader::Execute()
{
   try
      {
      bool bBreak = false;
      while (1)
         {
         // wait for 'load' or 'stop' request. To avoid dead locks we
         // have a timeout of 1 second: in that case we simply re-loop,
         // but if termination was requested meanwhile or file is closed
         // we break the loop
         DWORD nWaitResult = WaitForMultipleObjects(2, m_hEvents, false, 1000);
         switch (nWaitResult)
            {
            // first is 'load':
            case (WAIT_OBJECT_0 + SDP_WAVEREADEREVENT_LOAD):
               ReadData();
               break;
            // second is stop
            case (WAIT_OBJECT_0 + SDP_WAVEREADEREVENT_STOP):
               bBreak = true; break;
            case (WAIT_TIMEOUT):
               break;
            default:
               throw Exception("unexpected event");
            }

         if (bBreak || Terminated || !m_pSndFile)
            break;
         }
      }
   catch (Exception &e)
      {
      m_strThreadError = e.Message;
      OutputDebugString(("Thread error: " + m_strThreadError).c_str());
      Terminate();
      }
}
//---------------------------------------------------------------------------

//------------------------------------------------------------------------------
/// Loads data from file to buffers (if status of buffer(s) is 'done')
//------------------------------------------------------------------------------
void SDPWaveReader::ReadData()
{
   if (!m_pSndFile)
      throw Exception("try to read file samples from closed file");

   // protect file access that is used also by 'SetPosition'
   EnterCriticalSection(&m_csFile);
   try
      {
      if (m_nSamplesReadFromFile >= (m_nTotalLength) && !!m_nTotalLength)
         return;

      for (int i = 0; i < 2; i++)
         {
         if (m_sdpWB[i].m_wrbStatus != SDP_WAVEREADERBUFFERSTATUS_DONE)
            continue;

         m_sdpWB[i].m_vafBuffer = 0.0f;

         unsigned int nValuesToRead = m_nBufferSize * m_nNumFileChannels;
         unsigned int nValuesRead   = 0;
         int64_t nValuesToReadInThisRun;
         uint64_t  nNewFilePos;
         int64_t n;
         bool bZeroSamplesReadOnLastRun = false;
         // loop until enough samples read (or file closed, because reading is done)
         while (nValuesRead < nValuesToRead && !!m_pSndFile)
            {
            // 1. full file used: 'regular' reading
            if (m_nLength == 0)
               {
               n = sf_read_float(m_pSndFile, &m_sdpWB[i].m_vafBuffer[nValuesRead], nValuesToRead - nValuesRead);
               }
            // 2. snippet used: read with respect to file offset and last sample to use
            else
               {
               // case 2a: 'last sample' behind 'file offset': regular snippet used
               // (snippet itself is no looped part of file)
               if (m_nFileLastSample > m_nFileOffset)
                  {
                  nValuesToReadInThisRun = nValuesToRead - nValuesRead;
                  if (m_nFilePos + (uint64_t)nValuesToReadInThisRun/m_nNumFileChannels >= m_nFileLastSample)
                     nValuesToReadInThisRun = (int64_t)((m_nFileLastSample - m_nFilePos) * m_nNumFileChannels);
                  n = sf_read_float(m_pSndFile, &m_sdpWB[i].m_vafBuffer[nValuesRead], nValuesToReadInThisRun);
                  }
               // case 2b: 'last sample' before 'file offset': looped snippet used.
               else
                  {
                  // case 2b1: we are already behind 'file offset': try to regular reading until file end
                  if (m_nFilePos >= m_nFileOffset)
                     {
                     n = sf_read_float(m_pSndFile, &m_sdpWB[i].m_vafBuffer[nValuesRead], nValuesToRead - nValuesRead);
                     }
                  // case 2b2: we are between beginning of file and 'last sample'
                  else if (m_nFilePos < m_nFileLastSample)
                     {
                     nValuesToReadInThisRun = nValuesToRead - nValuesRead;
                     if (m_nFilePos + (uint64_t)nValuesToReadInThisRun/m_nNumFileChannels >= m_nFileLastSample)
                        nValuesToReadInThisRun = (int64_t)(m_nFileLastSample - m_nFilePos) * m_nNumFileChannels;
                     n = sf_read_float(m_pSndFile, &m_sdpWB[i].m_vafBuffer[nValuesRead], nValuesToReadInThisRun);
                     }
                  // case 2b3: we are in 'forbidden region': must never happen
                  else
                     throw Exception("fatal file snippet positioning error 1");
                  }
               }

            // should never happen!
            if (n < 0)
               throw Exception("cannot read data from file (error 1)");

            // adjust internal file position
            m_nFilePos += (uint64_t)n/m_nNumFileChannels;
            nValuesRead += (unsigned int)n;
            m_nSamplesReadFromFile += (uint64_t)n/m_nNumFileChannels;
            // not enough read? Then we have to loop the file/snippet
            if (nValuesRead < nValuesToRead)
               {
               // leave here if enough samples read
               if (m_nSamplesReadFromFile >= m_nTotalLength && !!m_nTotalLength)
                  {
                  break;
                  }
               // otherwise reset file position
               else
                  {
                  // usually we simply have to loop here if no single sample was read.
                  // For safety reasons (to avoid dead lock) we do not allow
                  // to subsequent 'zero sample read' occurrances
                  if (n == 0)
                     {
                     if (bZeroSamplesReadOnLastRun)
                        throw Exception("cannot read data from file (error 2)");
                     bZeroSamplesReadOnLastRun = true;
                     }
                  else
                     bZeroSamplesReadOnLastRun = false;


                  // Check different cases
                  // - default value: set position to 0
                  nNewFilePos    = 0;
                  // 1. full file used: reset to file start and increase loop count:
                  // - nothing to do: default values are fine
                  if (m_nLength == 0)
                     {
                     }
                  // 2. snippet used
                  else
                     {
                     // 2a: 'last sample' behind 'file offset': regular snippet used
                     if (m_nFileLastSample > m_nFileOffset)
                        nNewFilePos = m_nFileOffset;
                     // 2b: looped snippet
                     else
                        {
                        // here we have to distinguish, if we
                        // - loop the snippet, i.e. we are at stop pos now...
                        if (m_nFilePos == m_nFileLastSample)
                           nNewFilePos = m_nFileOffset;
                        // ... or loop the file to build the full snippet: move
                        // to file start (default, i.e. do nothing)
                        }
                     }
                  //OutputDebugStringW(IntToStr((int)nNewFilePos).c_str());
                  nNewFilePos     +=  m_nCrossfadeOffset;
                  // OutputDebugStringW(IntToStr((int)nNewFilePos).c_str());
                  sf_seek(m_pSndFile, (int64_t)nNewFilePos, SEEK_SET);
                  m_nFilePos     =  nNewFilePos;
                  }
               }
            }

         // set buffer status to 'filled'
         m_sdpWB[i].m_wrbStatus = SDP_WAVEREADERBUFFERSTATUS_FILLED;
         }
      }
   __finally
      {
      LeaveCriticalSection(&m_csFile);
      }
}
//------------------------------------------------------------------------------

//------------------------------------------------------------------------------
/// sets 'total' position within file object with respect to loop count,
/// start offset and file snippet
/// IMPORTANT NOTE: must nevber be called, while 'really' in use, i.e. if
/// GetSample is currently called asynchronously!!!
//------------------------------------------------------------------------------
void SDPWaveReader::SetPosition(uint64_t nPosition)
{

//OutputDebugString(__FUNC__);
   AnsiString strError;
   try
      {
      // protect file access that is used also by 'ReadData'
      EnterCriticalSection(&m_csFile);
      try
         {
         // check position vs. length (only if not endless loop!)
         if (!!TotalLength() && nPosition >= TotalLength())
            throw Exception("position exceeds total length of file object");

         strError = "set file position";
         int64_t nFilePosition;
         // NOTE: the first m_nCrossfadeOffset samples are
         // only used in the very first loop (later the crossfade buffer
         // is used for adding up in SDPOutputData lass)).
         // Thus we have to check, if we are within the first loop
         if (nPosition < UsedLength() - m_nStartPos)
            nFilePosition = (int64_t)(m_nStartPos + nPosition + m_nFileOffset);
         else
            {
            // then check position within file with respect to used length.
            // - First we subtract 'incomplete' first loop (where m_nStartPos was used) to have
            //   correct 'looplength' for 'modulo' which is UsedLength() -  m_nCrossfadeOffset
            // - finally add m_nCrossfadeOffset (first m_nCrossfadeOffset samples are in crossfade-buffer)
            //   and the file offset
            nFilePosition = (int64_t)(((nPosition - (UsedLength() - m_nStartPos)) % (UsedLength() -  m_nCrossfadeOffset))
                           + m_nCrossfadeOffset + m_nFileOffset);
            }

         if (nFilePosition >= (int64_t)m_nFileSize)
            nFilePosition -= m_nFileSize;

         int64_t n = sf_seek(m_pSndFile, nFilePosition, SEEK_SET);
         if (n != nFilePosition)
            throw Exception("Error getting file sample position");
         m_bDone = false;
         m_nFilePos = (uint64_t)nFilePosition;
         // set numbers of samples already read.
         m_nSamplesRead          = nPosition;
         m_nSamplesReadFromFile  = m_nSamplesRead;


         strError = "reset buffers";
         // reset buffers
         m_nReadBufIndex = 0;
         m_nReadPos      = 0;
         }
      __finally
         {
         // leave critical section before setting signals for thread to allow
         // ReadData (called by thread function) to enter the critical section
         LeaveCriticalSection(&m_csFile);
         }

      m_sdpWB[0].m_wrbStatus = SDP_WAVEREADERBUFFERSTATUS_DONE;
      m_sdpWB[1].m_wrbStatus = SDP_WAVEREADERBUFFERSTATUS_DONE;

      strError = "resume thread";
      if (Suspended)
          Start();

      strError = "set thread re-start events";
      // tell thread to read data
      if (!SetEvent(m_hEvents[SDP_WAVEREADEREVENT_LOAD]))
         throw Exception("error setting load event");

      strError = "wait for filled buffers";
      // wait until thread is really running, i.e. first buffers are filled!
      DWORD dw = GetTickCount();
      while (  m_sdpWB[0].m_wrbStatus != SDP_WAVEREADERBUFFERSTATUS_FILLED
            || m_sdpWB[1].m_wrbStatus != SDP_WAVEREADERBUFFERSTATUS_FILLED
            )
         {
         Sleep(1);
         Application->ProcessMessages();
         if (ElapsedSince(dw) > 2000)
            {
            if (Suspended)
               throw Exception("error resuming file reader thread");
            throw Exception("error reading file samples");
            }
         if (Terminated)
            break;
         }

      // check, if thread func itself had an error and terminated the thread
      if (!m_strThreadError.IsEmpty())
         throw Exception(m_strThreadError);
      }
   catch (Exception &e)
      {
      // tell thread to stop
      if (!!m_hEvents[SDP_WAVEREADEREVENT_STOP])
         {
         if (!SetEvent(m_hEvents[SDP_WAVEREADEREVENT_STOP]))
            throw Exception("error setting stop event");
         }

      if (m_pSndFile)
         sf_close(m_pSndFile);
      m_pSndFile = NULL;
      AnsiString str = "error setting file position of file '" + ExpandFileName(m_strFileName) + "': " + e.Message;
      if (!strError.IsEmpty())
         str += " (" + strError + ")";
      throw Exception(str);
      }
}
//------------------------------------------------------------------------------

//------------------------------------------------------------------------------
/// returns one sample
//------------------------------------------------------------------------------
float SDPWaveReader::GetFileSample()
{
   m_bStarted = true;
   float fReturn = 0.0f;
   if (m_bDone)
      throw Exception("try to get file samples from done file");

   if (m_nReadPos < m_nBufferSize)
      {
      fReturn     = m_sdpWB[m_nReadBufIndex].m_vafBuffer[m_nReadPos++*m_nNumFileChannels + m_nFileChannel];
      m_nSamplesRead++;

      // completely done?
      if (m_nSamplesRead == m_nTotalLength && !!m_nTotalLength)
         m_bDone = true;
      // end of buffer reached after increment, but not completely done?
      //    -> switch data buffer
      else if (m_nReadPos == m_nBufferSize)
         {
         // set actual to done
         m_sdpWB[m_nReadBufIndex].m_wrbStatus = SDP_WAVEREADERBUFFERSTATUS_DONE;
         // tell thread to read data
         if (!SetEvent(m_hEvents[SDP_WAVEREADEREVENT_LOAD]))
            throw Exception("error setting load event");
         // switch
         m_nReadBufIndex = (int)(!(bool)m_nReadBufIndex);

         // check, if we are allowed to read the other buffer!
         if (m_sdpWB[m_nReadBufIndex].m_wrbStatus != SDP_WAVEREADERBUFFERSTATUS_FILLED)
            {
            throw Exception("unexpected wave file read error (no filled data buffer): "
            + IntToStr((int)m_nSamplesRead) + ", " + IntToStr((int)m_nTotalLength) + ", "
            + IntToStr((int)m_nTotalLength-(int)m_nSamplesRead)
            );
            }
         m_nReadPos = 0;
         }
      }
   else
      throw Exception("unexpected wave file read error 1 (" + IntToStr((int)m_nReadPos) + ":" + IntToStr((int)m_nBufferSize) + ")");

   return fReturn;
}
//------------------------------------------------------------------------------

//------------------------------------------------------------------------------
/// returns file size in samples
//------------------------------------------------------------------------------
uint64_t SDPWaveReader::FileSize()
{
   return m_nFileSize;
}
//------------------------------------------------------------------------------

//------------------------------------------------------------------------------
/// returns total lenght to play in samples with respect to start position and
/// looping
//------------------------------------------------------------------------------
uint64_t SDPWaveReader::TotalLength()
{
   return m_nTotalLength;
}
//------------------------------------------------------------------------------

//------------------------------------------------------------------------------
/// returns true, if file is looped endlessly, false else
//------------------------------------------------------------------------------
bool SDPWaveReader::IsEndlessLoop()
{
   return (m_nTotalLength == 0);
}
//------------------------------------------------------------------------------

//------------------------------------------------------------------------------
/// returns, if file is done completely
//------------------------------------------------------------------------------
bool SDPWaveReader::Done()
{
   return m_bDone;
}
//------------------------------------------------------------------------------

//------------------------------------------------------------------------------
/// returns filename
//------------------------------------------------------------------------------
AnsiString SDPWaveReader::GetFileName()
{
   return m_strFileName;
}
//------------------------------------------------------------------------------

//------------------------------------------------------------------------------
/// returns filechannel
//------------------------------------------------------------------------------
unsigned int SDPWaveReader::GetFileChannel()
{
   return m_nFileChannel;
}
//------------------------------------------------------------------------------

//------------------------------------------------------------------------------
/// returns file offset (here: startpos)
//------------------------------------------------------------------------------
uint64_t SDPWaveReader::GetStartOffset()
{
   return m_nStartPos;
}
//------------------------------------------------------------------------------

//------------------------------------------------------------------------------
/// returns file offset
//------------------------------------------------------------------------------
uint64_t SDPWaveReader::GetFileOffset()
{
   return m_nFileOffset;
}
//------------------------------------------------------------------------------

//------------------------------------------------------------------------------
/// returns m_nSamplesRead
//------------------------------------------------------------------------------
uint64_t  SDPWaveReader::GetSamplesRead()
{
   return m_nSamplesRead;
}
//------------------------------------------------------------------------------

//------------------------------------------------------------------------------
/// returns start status
//------------------------------------------------------------------------------
bool SDPWaveReader::Started()
{
   return m_bStarted;
}
//------------------------------------------------------------------------------

//------------------------------------------------------------------------------
/// returns used length or 0, if complete file used
//------------------------------------------------------------------------------
uint64_t SDPWaveReader::UsedLength()
{
   if (!m_nLength)
      return m_nFileSize;
   return  m_nLength;
}
//------------------------------------------------------------------------------

//------------------------------------------------------------------------------
/// retrieves properties of a file (static function)
//------------------------------------------------------------------------------
void SDPWaveReader::WaveFileProperties(AnsiString strFileName,
                                       unsigned int &nNumChannels,
                                       unsigned int &nNumSamples,
                                       double       &dSampleRate
                                       )
{
   SNDFILE* pSndFile = NULL;
   try
      {
      SF_INFO sfi;
      ZeroMemory(&sfi, sizeof(sfi));


      pSndFile = sf_open(strFileName.c_str(), SFM_READ, &sfi);

      if (!pSndFile)
         throw Exception("cannot open file");

      nNumChannels = (unsigned int)sfi.channels;
      dSampleRate  = sfi.samplerate;
      nNumSamples  = (unsigned int)sfi.frames;
      }
   __finally
      {
      if (pSndFile)
         sf_close(pSndFile);
      }
}
//------------------------------------------------------------------------------

//------------------------------------------------------------------------------
/// returns psition within loop with respect to start offset
//------------------------------------------------------------------------------
uint64_t  SDPWaveReader::GetLoopPosition()
{
   // - in first loop the position is 'simply' GetSamplesRead - StartOffset
   uint64_t  nReturn;
   if (m_nSamplesRead < (UsedLength() - m_nStartPos))
      nReturn = m_nSamplesRead + m_nStartPos;
   else
      {
      // subtract first incomplete loop WITHOUT subtracting m_nCrossfadeOffset
      nReturn = m_nSamplesRead - (UsedLength()-m_nStartPos);
      // calculate number of available in m_nSamplesRead, where loops (beginning
      // with second loop) only have UsedLength()-m_nCrossfadeOffset samples!
      uint64_t nNumLoops = nReturn / (UsedLength()-m_nCrossfadeOffset) + 1;
      // add up NumLoops * m_nCrossfadeOffset (as if it were 'full' loops)
      nReturn += nNumLoops*m_nCrossfadeOffset;
      //
      nReturn = nReturn % (UsedLength());
      }

   return nReturn;
}
//------------------------------------------------------------------------------


