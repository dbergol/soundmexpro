//------------------------------------------------------------------------------
/// \file SoundDllPro_SoundClassWdm.cpp
/// \author Berg
/// \brief Implementation of sound class for SoundMexPro. Inherits form
/// SoundClassBase. Implements all abstract functions for use with Wdm
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
#pragma hdrstop

#include "SoundDllPro_SoundClassWdm.h"
#include "SoundDllPro.h"
#include "SoundDllPro_Tools.h"

//---------------------------------------------------------------------------

#pragma package(smart_init)
#pragma warn -use
//------------------------------------------------------------------------------

//------------------------------------------------------------------------------
// contructor, initializes members, creates device class
//------------------------------------------------------------------------------
SoundClassWdm::SoundClassWdm() :
   SoundClassBase(),
   m_pwo(NULL),
   m_prb(NULL),
   m_bInitialized(false),
   m_pslDeviceNames(NULL),
   m_bStopping(false),
   m_nSamplesPlayed(0),
   m_nSampleStopPos(0)
{
   InitializeCriticalSection(&m_csProcess);
   ZeroMemory(&m_wfx, sizeof(m_wfx));
   // create dummy ringbuffer needed for getting device up and running in
   // endless loop playing zeros
   m_prb = new TMMRingBuffer(NULL);
   m_prb->NumBuffers    = 2;
   m_prb->LoopIfEmpty   = true;
   m_prb->QueueMode     = qmDuplex;


   m_pslDeviceNames     = new TStringList();

   CreateDevice();
}
//------------------------------------------------------------------------------

//------------------------------------------------------------------------------
// destructor, does cleanup
//------------------------------------------------------------------------------
SoundClassWdm::~SoundClassWdm()
{
   DeleteDevice();

   TRYDELETENULL(m_prb);
   TRYDELETENULL(m_pslDeviceNames);

   DeleteCriticalSection(&m_csProcess);
}
//------------------------------------------------------------------------------

//------------------------------------------------------------------------------
// destructor, does cleanup
//------------------------------------------------------------------------------
void SoundClassWdm::CreateDevice()
{
   DeleteDevice();

   m_pwo                  = new TMMWaveOut(NULL);
   m_pwo->OnBufferFilled  = WdmProcess;
   m_pwo->OnStart         = OnStart;
   m_pwo->OnStop          = OnStop;
   m_pwo->TimeFormat      = tfSample;
   m_pwo->CallBackMode    = cmThread;
   #if __BORLANDC__ == 1380
   m_pwo->OnXrun          = OnXRun;
   #endif

   // get number of devices
   int nNumDevs = SoundNumDrivers();

   // iteration through devices ...
   int nIndex;
   for (nIndex = 0; nIndex < nNumDevs; nIndex++)
      {
      m_pwo->DeviceID = nIndex;
      m_pslDeviceNames->Add(m_pwo->ProductName);
      }
   m_pwo->DeviceID = 0;


   m_pwo->NumBuffers    = 20;
   m_pwo->BufferSize    = 4096;

   // set ringbuffer properties
   m_prb->BufferSize    = m_pwo->BufferSize;
   m_prb->Output        = m_pwo;

   // set format tag to
   // - WAVE_FORMAT_PCM (1) for 16bit
   // - WAVE_FORMAT_IEEE_FLOAT (3) for 32bit.
   m_wfx.wFormatTag        = 3;
   m_wfx.wBitsPerSample    = 32;
   m_wfx.nSamplesPerSec    = 44100;
   m_wfx.nChannels         = 2;
   m_wfx.nBlockAlign       = (short int)(m_wfx.nChannels * m_wfx.wBitsPerSample/8);
   m_wfx.nAvgBytesPerSec   = m_wfx.nSamplesPerSec * m_wfx.nChannels * m_wfx.wBitsPerSample/8;
   m_wfx.cbSize            = 0; // NOTE: this is the size of 'additional info' which are _not_ present
   m_pwo->PWaveFormat      = &m_wfx;
}
//------------------------------------------------------------------------------

//------------------------------------------------------------------------------
// destructor, does cleanup
//------------------------------------------------------------------------------
void SoundClassWdm::DeleteDevice()
{
   m_pslDeviceNames->Clear();
   if (!m_pwo)
      return;
   // Stop device non-smooth
   try
      {
      SoundStop(false);
      }
   catch (...)
      {
      }

   // cleanup
   TRYDELETENULL(m_pwo);
   unsigned int n;
   for (n = 0; n < m_vvfBuffer.size(); n++)
      m_vvfBuffer[n].resize(0);
   ZeroMemory(&m_wfx, sizeof(m_wfx));
}
//------------------------------------------------------------------------------

//------------------------------------------------------------------------------
// returns true if if Wdm is prepared or false otherwise
//------------------------------------------------------------------------------
bool SoundClassWdm::SoundInitialized()
{
   
   return m_bInitialized;
}
//------------------------------------------------------------------------------

//------------------------------------------------------------------------------
// loads Wdm driver by index
//------------------------------------------------------------------------------
void SoundClassWdm::SoundLoadDriverByIndex(size_t nIndex)
{
   if (nIndex >= SoundNumDrivers())
      throw Exception("driver index out of range");
   m_pwo->DeviceID = nIndex;
   if (m_lpfnOnStateChange)
      m_lpfnOnStateChange(Asio::LOADED);

}
//------------------------------------------------------------------------------

//------------------------------------------------------------------------------
// loads Wdm driver by name
//------------------------------------------------------------------------------
void SoundClassWdm::SoundLoadDriverByName(AnsiString strName)
{
   int nIndex = m_pslDeviceNames->IndexOf(strName);
   if (nIndex < 0)
      throw Exception("driver with requested name not found");
   SoundLoadDriverByIndex(nIndex);
   if (m_lpfnOnStateChange)
      m_lpfnOnStateChange(Asio::LOADED);

}
//------------------------------------------------------------------------------


//------------------------------------------------------------------------------
// unloads Wdm driver
//------------------------------------------------------------------------------
void SoundClassWdm::SoundUnloadDriver()
{
   m_bInitialized = false;
   if (m_lpfnOnStateChange)
      m_lpfnOnStateChange(Asio::FREE);


}
//------------------------------------------------------------------------------


//------------------------------------------------------------------------------
//  Reads configuration values from passed stringlist 
//------------------------------------------------------------------------------
void SoundClassWdm::SoundInit(TStringList* psl)
{
   if (SoundIsRunning())
      throw Exception("cannot reinitialize device while it is running!");

   try
      {
      AnsiString str = psl->Values[SOUNDDLLPRO_PAR_OUTPUT];
      // if empty, use first two output channels as default
      if (str.IsEmpty())
         str = "0,1";
      if (str != "0,1")
         throw Exception("invalid field in 'output': only first two channels supported for wdm driver model");
      str = psl->Values[SOUNDDLLPRO_PAR_INPUT];
      if (!str.IsEmpty())
         throw Exception("invalid field in 'input': no inputs supported for wdm driver model");

      // read numbufs...
      m_pwo->NumBuffers = (int)GetInt(psl, SOUNDDLLPRO_PAR_NUMBUFS, 20, VAL_POS_OR_ZERO);
      // and undocumented value 'bufsize' (in samples!!)
      int nBufsize = (int)GetInt(psl, SOUNDDLLPRO_PAR_BUFSIZE, 0, VAL_POS_OR_ZERO);
      if (nBufsize == 0)
         m_pwo->BufferSize = 4096;
      else
         m_pwo->BufferSize = nBufsize * m_wfx.nBlockAlign;

      m_pwo->Close();
      m_pwo->PWaveFormat      = &m_wfx;
      // open/close to force an error if wave format is not supported!
      m_pwo->Open();
      m_pwo->Close();

      // initialize buffers: NOTE: we only support stereo here!
      // Buffersize of device is specified in bytes. So in every buffer for the
      // device callback we expect
      //    m_pwo->BufferSize / nBytesPerSample / NumberOfChannels
      // values per channel in each buffer.
      // Set up internal float buffer with this size
      m_vvfBuffer.resize(2);
      m_vvfBuffer[0].resize(m_pwo->BufferSize/(m_wfx.wBitsPerSample/8*2));
      m_vvfBuffer[1].resize(m_vvfBuffer[0].size());

      m_bInitialized = true;
      if (m_lpfnOnStateChange)
         m_lpfnOnStateChange(Asio::INITIALIZED);

      }
   __finally
      {
      }
}
//------------------------------------------------------------------------------

//------------------------------------------------------------------------------
// returns number of Wdm drivers
//------------------------------------------------------------------------------
size_t SoundClassWdm::SoundNumDrivers()
{
   return waveOutGetNumDevs();
}
//------------------------------------------------------------------------------

//------------------------------------------------------------------------------
// returns name of Wdm driver by index
//------------------------------------------------------------------------------
AnsiString SoundClassWdm::SoundDriverName(unsigned int iIndex)
{

   if (iIndex >= SoundNumDrivers())
      throw Exception("driver index out of range");
   return m_pslDeviceNames->Strings[iIndex];
}
//------------------------------------------------------------------------------

//------------------------------------------------------------------------------
// returns index of current Wdm driver
//------------------------------------------------------------------------------
unsigned int SoundClassWdm::SoundCurrentDriverIndex(void)
{
   return m_pwo->DeviceID;
}
//------------------------------------------------------------------------------



//------------------------------------------------------------------------------
// returns number of input or ouput channels of current Wdm driver
//------------------------------------------------------------------------------
size_t SoundClassWdm::SoundChannels(Direction adDirection)
{
   // no input supported
   if (adDirection == Asio::INPUT)
      return 0;
   return 2;
}
//------------------------------------------------------------------------------

//------------------------------------------------------------------------------
// returns channel name of input or ouput channel of current Wdm driver
//------------------------------------------------------------------------------
#pragma argsused
AnsiString SoundClassWdm::SoundChannelName(unsigned int iChannelIndex, Direction adDirection)
{
   return "Channel_" + IntToStr((int)iChannelIndex);
}
//------------------------------------------------------------------------------

//------------------------------------------------------------------------------
// returns number of ACTIVE input or output channels of current Wdm driver
//------------------------------------------------------------------------------
size_t SoundClassWdm::SoundActiveChannels(Direction adDirection)
{
   return SoundChannels(adDirection);
}
//------------------------------------------------------------------------------

//------------------------------------------------------------------------------
// returns name of an ACTIVE input or output channel of current Wdm driver
//------------------------------------------------------------------------------
AnsiString SoundClassWdm::SoundGetActiveChannelName(unsigned int iChannelIndex, Direction adDirection)
{
   return SoundChannelName(iChannelIndex, adDirection);
}
//------------------------------------------------------------------------------


//------------------------------------------------------------------------------
// returns current buffer size of current Wdm driver
//------------------------------------------------------------------------------
long SoundClassWdm::SoundBufsizeCurrent()
{
   return m_pwo->BufferSize / m_wfx.nBlockAlign;
}
//------------------------------------------------------------------------------

//------------------------------------------------------------------------------
// returns best buffer size of current Wdm driver
//------------------------------------------------------------------------------
long SoundClassWdm::SoundBufsizeBest()
{
   return m_pwo->BufferSize / m_wfx.nBlockAlign;
}
//------------------------------------------------------------------------------

//------------------------------------------------------------------------------
// returns current number of buffers
//------------------------------------------------------------------------------
long SoundClassWdm::SoundNumBufOut()
{
   if (!m_pwo)
      return 0;
   return m_pwo->NumBuffers;
}
//------------------------------------------------------------------------------


//------------------------------------------------------------------------------
// starts Wdm device
//------------------------------------------------------------------------------
void SoundClassWdm::SoundStart()
{
   m_nProcessCalls = 0;
   m_pwo->Start();
}
//------------------------------------------------------------------------------

//------------------------------------------------------------------------------
// stops Wdm device with optional waiting for 'real-stop-is-done'
//------------------------------------------------------------------------------
void SoundClassWdm::SoundStop(bool bWaitForStopDone)
{
   m_bStopping = true;
   m_pwo->Stop();
   if (bWaitForStopDone)
      {
      DWORD dw = GetTickCount();
      while(1)
         {
         if (ElapsedSince(dw) > 1000)
            throw Exception("WDM-Stopping timeout");
         if (!SoundIsRunning())
            break;
         Application->ProcessMessages();
         Sleep(10);
         }

      }

   m_bStopping = false;
   m_nProcessCalls = 0;
}
//------------------------------------------------------------------------------

//------------------------------------------------------------------------------
// returns true if stae is RUNNING or false else
//------------------------------------------------------------------------------
bool SoundClassWdm::SoundIsRunning()
{
   return m_pwo->State.Contains(wosPlay) || m_pwo->State.Contains(wosPause);
}
//------------------------------------------------------------------------------

//------------------------------------------------------------------------------
// returns m_bStopping
//------------------------------------------------------------------------------
bool SoundClassWdm::SoundIsStopping()
{
   return m_bStopping;
}
//------------------------------------------------------------------------------

//------------------------------------------------------------------------------
// returns maximum value detected in an active channel
//------------------------------------------------------------------------------
#pragma argsused
float SoundClassWdm::SoundActiveChannelMaxValue(Direction adDirection, size_t nChannel)
{
   // in this implementation values are always between 1 and -1
   return 1.0f;
}
//------------------------------------------------------------------------------

//------------------------------------------------------------------------------
// returns minimum value detected in an active channel
//------------------------------------------------------------------------------
#pragma argsused
float SoundClassWdm::SoundActiveChannelMinValue(Direction adDirection, size_t nChannel)
{
   // in this implementation values are always between 1 and -1
   return -1.0f;
}
//------------------------------------------------------------------------------

//------------------------------------------------------------------------------
// sets sample rate of current device
//------------------------------------------------------------------------------
void SoundClassWdm::SoundSetSampleRate(double dSampleRate)
{
   if (SoundIsRunning())
      throw Exception("cannot reinitialize device while it is running!");
   if (!SoundCanSampleRate(dSampleRate))
      throw Exception("samplerate not supported");
   m_wfx.nSamplesPerSec    = (DWORD)dSampleRate;
   m_pwo->PWaveFormat      = &m_wfx;

}
//------------------------------------------------------------------------------

//------------------------------------------------------------------------------
// checks, if a particular samplerate is supported by device
//------------------------------------------------------------------------------
bool SoundClassWdm::SoundCanSampleRate(double dSampleRate)
{
   if (SoundIsRunning())
      throw Exception("cannot check sample rate check on running device");
   try
      {
      int nSampleRate = m_wfx.nSamplesPerSec;
      try
         {
         m_wfx.nSamplesPerSec    = (DWORD)dSampleRate;
         m_pwo->PWaveFormat      = &m_wfx;
         // open/close to force an error if wave format is not supported!
         m_pwo->Open();
         m_pwo->Close();
         }
      __finally
         {
         m_wfx.nSamplesPerSec = nSampleRate;
         m_pwo->PWaveFormat      = &m_wfx;
         }
      }
   catch (...)
      {
      return false;
      }
   return true;
}
//------------------------------------------------------------------------------


//------------------------------------------------------------------------------
// returns current samplerate
//------------------------------------------------------------------------------
double SoundClassWdm::SoundGetSampleRate(void)
{
   return m_wfx.nSamplesPerSec;
}
//------------------------------------------------------------------------------

//------------------------------------------------------------------------------
/// OnStop callback of WDM device
/// Calls user defined callbacks and closes device
//------------------------------------------------------------------------------
#pragma argsused
void  __fastcall SoundClassWdm::OnStop(TObject *Sender)
{
   Application->ProcessMessages();
   if (m_lpfnOnStopComplete)
      m_lpfnOnStopComplete();
   if (m_lpfnOnStateChange)
      m_lpfnOnStateChange(Asio::PREPARED);
   m_pwo->Close();
}
//------------------------------------------------------------------------------

//------------------------------------------------------------------------------
/// OnStart callback of WDM device
/// Calls user defined callback
//------------------------------------------------------------------------------
#pragma argsused
void  __fastcall SoundClassWdm::OnStart(TObject *Sender)
{
   if (m_lpfnOnStateChange)
      m_lpfnOnStateChange(Asio::RUNNING);
   m_nSamplesPlayed = 0;
   m_nSampleStopPos = 0;
}
//------------------------------------------------------------------------------

//------------------------------------------------------------------------------
/// Overloaded CAsio callback called on xruns
/// Calls user defined callback
//------------------------------------------------------------------------------
#pragma argsused
void __fastcall SoundClassWdm::OnXRun(TObject *Sender)
{
   if (m_lpfnOnXrun)
      m_lpfnOnXrun(XR_PROC);
}

//------------------------------------------------------------------------------
/// Overloaded CWdm callback called on fatal errors
/// Calls user defined callback
//------------------------------------------------------------------------------
#pragma argsused
void __fastcall SoundClassWdm::OnError(TObject *Sender)
{
   if (m_lpfnOnError)
      m_lpfnOnError();
}
//---------------------------------------------------------------------------


//------------------------------------------------------------------------------
/// \brief VCL callback attached to devices 'OnBufferFilled' property. Calls user
/// defined processing callback (if not paused), calls PostProcess and converts
/// data back to device format.
/// \param[in] Sender: not used
/// \param[in|out] lpWaveHdr pointer to WAVEHDR struct.
/// \exception all caught within function
/// NOTE: Xrun detection implemented in MMTools classes TMMDSWaveOut and TMMWaveOut
//------------------------------------------------------------------------------
#pragma argsused
void  __fastcall SoundClassWdm::WdmProcess(TObject *Sender, PWaveHdr lpWaveHdr)
{
   #if __BORLANDC__ != 1380
   if (m_pwo->BufferCounter < 3)
      OnXRun(NULL);
   #endif
   m_nProcessCalls++;
   bool bIsLast = false;
   try
      {
      // write zeros to buffer initially
      ZeroMemory(lpWaveHdr->lpData, lpWaveHdr->dwBytesRecorded);
      // play a few buffers zeroes before passing data: otherwise data are clipped
      // in the beginning!
      if (m_nProcessCalls < (unsigned int)m_pwo->NumBuffers)
         return;
      EnterCriticalSection(&m_csProcess);
      try
         {
         // clear internal buffer internal buffer
         m_vvfBuffer[0] = 0.0f;
         m_vvfBuffer[1] = 0.0f;


         if (m_lpfnOnProcess)
            m_lpfnOnProcess(m_vvfDummy, m_vvfBuffer, bIsLast);

         if (m_lpfnOnBufferPlay)
            m_lpfnOnBufferPlay(m_vvfBuffer);

         if (m_lpfnOnBufferDone)
            m_lpfnOnBufferDone(m_vvfDummy, m_vvfBuffer, bIsLast);


         // convert float data to 16bit or 32bit integer respectively
         unsigned int nSamples, nSampleIndex;
         // format 16bit int
         if (m_pwo->PWaveFormat->wBitsPerSample == 16)
            {
            float fScale = -(float)(SHRT_MIN);
            // NOTE: here dwBytesRecorded is used, because MMTools use always
            // to store bytes to use (might be < dwBufferSize on stopping of device)
            nSamples = lpWaveHdr->dwBytesRecorded / (2*sizeof(short int));
            short int *lpi = (short int*)lpWaveHdr->lpData;
            // check for correct size: error must never happen, but we cannot
            // be sure here, what user callback did...
            if (m_vvfBuffer.size() != 2)
               throw Exception("sizing error 1");
            if (m_vvfBuffer[0].size() != nSamples)
               throw Exception("sizing error 2");
            if (m_vvfBuffer[1].size() != nSamples)
               throw Exception("sizing error 3");
            // scale both channels by SHRT_MAX
            for (nSampleIndex = 0; nSampleIndex < nSamples; nSampleIndex++)
               {
               *lpi++ = (short int)(m_vvfBuffer[0][nSampleIndex]*fScale);
               *lpi++ = (short int)(m_vvfBuffer[1][nSampleIndex]*fScale);
               }
         m_nSamplesPlayed+= nSamples;
            }
         // format 32bit float
         else
            {
            nSamples = lpWaveHdr->dwBytesRecorded / (2*sizeof(float));
            float *lpf = (float*)lpWaveHdr->lpData;
            if (m_vvfBuffer.size() != 2)
               throw Exception("sizing error 1");
            if (m_vvfBuffer[0].size() != nSamples)
               throw Exception("sizing error 2");
            if (m_vvfBuffer[1].size() != nSamples)
               throw Exception("sizing error 3");
            // no scaling required
            for (nSampleIndex = 0; nSampleIndex < nSamples; nSampleIndex++)
               {
               *lpf++ = (float)(m_vvfBuffer[0][nSampleIndex]);
               *lpf++ = (float)(m_vvfBuffer[1][nSampleIndex]);
               }
            m_nSamplesPlayed+= nSamples;
            }
         }
      __finally
         {
         LeaveCriticalSection(&m_csProcess);
         }
      }
   // on exceptions in processing thread stop device and call SoundOnFatalError
   catch (Exception &e)
      {
      try
         {
         m_pwo->Stop();
         }
      catch (...)
         {
         }
      if (m_lpfnOnError)
         m_lpfnOnError();
      }

   // HIER: StopperThread implementieren
   if (bIsLast && !m_nSampleStopPos)
      {
      // OutputDebugString( "INIT STOP");
      m_nSampleStopPos = m_nSamplesPlayed;
      }
   if (m_nSampleStopPos > 0 && m_nSamplesPlayed > m_nSampleStopPos + m_pwo->NumBuffers*SoundBufsizeCurrent())
      {
      try
         {
         m_nSampleStopPos = 0;
//         OutputDebugString("stop HIER");
         m_pwo->Stop();
         }
      catch (...)
         {
         }
      }
}
//------------------------------------------------------------------------------

//------------------------------------------------------------------------------
// returns device format
//------------------------------------------------------------------------------
AnsiString SoundClassWdm::SoundFormatString()
{
   if (!m_pwo || !m_pwo->PWaveFormat)
      return "unknown";
   return IntToStr(m_pwo->PWaveFormat->wBitsPerSample) + "-bit integer";
}
//------------------------------------------------------------------------------


