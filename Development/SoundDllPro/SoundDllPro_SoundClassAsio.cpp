//------------------------------------------------------------------------------
/// \file SoundDllPro_SoundClassAsio.cpp
/// \author Berg
/// \brief Implementation of ASIO sound class for SoundMexPro. Inherits from CAsio
/// AND SoundClassBase. Implements all abstract functions for use with ASIO and
/// overloads all necessary functions of CAsio class
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
#include "SoundDllPro_SoundClassAsio.h"
#include "soundmexpro_defs.h"
#include "SoundDllPro_Tools.h"
#include "SoundData.h"
#include "SoundDataExchanger.h"
//------------------------------------------------------------------------------
#pragma package(smart_init)
//------------------------------------------------------------------------------

//------------------------------------------------------------------------------
/// contructor, calls both base classes
//------------------------------------------------------------------------------
SoundClassAsio::SoundClassAsio() :
   SoundClassBase(),
   CAsio(),
   m_nNumProcBufs(0),
   m_bCaptureDoneProcessed(false),
   m_bFreezeSampleRateOnStart(false)
{
}
//------------------------------------------------------------------------------

//------------------------------------------------------------------------------
/// tell sound class to pass processed data to done-queue.
//------------------------------------------------------------------------------
void SoundClassAsio::SoundSetSaveProcessedCaptureData(bool b)
{
   m_bCaptureDoneProcessed = b;
}
//------------------------------------------------------------------------------

//------------------------------------------------------------------------------
/// returns true if if ASIO is prepared or false otherwise
//------------------------------------------------------------------------------
bool SoundClassAsio::SoundInitialized()
{
   return (GetState() >= Asio::PREPARED);
}
//------------------------------------------------------------------------------

//------------------------------------------------------------------------------
/// loads ASIO driver by index
//------------------------------------------------------------------------------
void SoundClassAsio::SoundLoadDriverByIndex(size_t nIndex)
{
   LoadDriverByIndex(nIndex, ASIODRIVER_SYSREF);
}
//------------------------------------------------------------------------------

//------------------------------------------------------------------------------
/// loads ASIO driver by name
//------------------------------------------------------------------------------
void SoundClassAsio::SoundLoadDriverByName(AnsiString strName)
{
   LoadDriverByName(strName, ASIODRIVER_SYSREF);
}
//------------------------------------------------------------------------------

//------------------------------------------------------------------------------
/// unloads ASIO driver
//------------------------------------------------------------------------------
void SoundClassAsio::SoundUnloadDriver()
{
   UnloadDriver();
   m_nNumProcBufs = 0;
}
//------------------------------------------------------------------------------

//------------------------------------------------------------------------------
///  Reads configuration values from passed stringlist and calls CASio::CreateBuffers
//------------------------------------------------------------------------------
void SoundClassAsio::SoundInit(TStringList* psl)
{                     
   if (GetState() < Asio::INITIALIZED)
      throw Exception("cannot initialize ASIO class if driver is not loaded");
   TStringList *pslTmp = new TStringList();
   AnsiString str;
   try
      {
      pslTmp->Delimiter = ',';
      // check, which channels to allocate
      std::vector<bool> vbOut, vbIn;
      int nChannels = SoundChannels(Asio::OUTPUT);
      int nChannelIndex, nLastDevice;
      str = psl->Values[SOUNDDLLPRO_PAR_OUTPUT];
      pslTmp->Clear();
      // if empty, use first two output channels as default
      if (str.IsEmpty())
         str = "0,1";
      // all: add all cahnnels
      if (!strcmpi(str.c_str(), SOUNDDLLPRO_PAR_ALL))
         {
         for (nChannelIndex = 0; nChannelIndex < nChannels; nChannelIndex++)
            pslTmp->Add(nChannelIndex);
         }
      // add it to string list. Don't do it, if value is -1: then disable all
      else if (str != "-1")
         pslTmp->DelimitedText = str;

      #ifdef TEST_INIT_DEBUG
      WriteDebugString("Initialize 6");
      #endif
      // same value for all channels if '-1'(false)
      nLastDevice = 0;
      for (int i = 0; i < pslTmp->Count; i++)
         {
         if (!TryStrToInt(pslTmp->Strings[i], nChannelIndex))
            throw Exception("invalid field in 'output': not an integer");
         if (nChannelIndex >= nChannels || nChannelIndex < 0)
            throw Exception("invalid field in 'output': out of range");
         if (i && nChannelIndex <= nLastDevice)
            throw Exception("invalid field in 'output': must be sorted ascending");
         nLastDevice = nChannelIndex;
         }
      for (nChannelIndex = 0; nChannelIndex < nChannels; nChannelIndex++)
         vbOut.push_back(pslTmp->IndexOf(IntToStr(nChannelIndex)) != -1);
      pslTmp->Clear();
      nChannels = SoundChannels(Asio::INPUT);
      str = psl->Values[SOUNDDLLPRO_PAR_INPUT];
      // if empty, use no input channels as default
      if (str.IsEmpty())
         str = "-1";
      if (!strcmpi(str.c_str(), SOUNDDLLPRO_PAR_ALL))
         {
         for (nChannelIndex = 0; nChannelIndex < nChannels; nChannelIndex++)
            pslTmp->Add(nChannelIndex);
         }
      // add it to string list. Don't do it, if value is -1: then disable all
      else if (str != "-1")
         pslTmp->DelimitedText = str;
      nLastDevice = 0;
      for (int i = 0; i < pslTmp->Count; i++)
         {
         if (!TryStrToInt(pslTmp->Strings[i], nChannelIndex))
            throw Exception("invalid field in 'input': not an integer");
         if (nChannelIndex >= nChannels || nChannelIndex < 0)
            throw Exception("invalid field in 'input': out of range");
         if (i && nChannelIndex <= nLastDevice)
            throw Exception("invalid field in 'input': must be sorted ascending");
         nLastDevice = nChannelIndex;
         }
      for (nChannelIndex = 0; nChannelIndex < nChannels; nChannelIndex++)
         vbIn.push_back(pslTmp->IndexOf(IntToStr(nChannelIndex)) != -1);
      #ifdef TEST_INIT_DEBUG
      WriteDebugString("Initialize 7");
      #endif

      #ifdef TEST_INIT_DEBUG
      WriteDebugString("Initialize 7.4");
      #endif
      // ... bufsize (with default BufsizeBest)
      int nBufSize = (int)GetInt(psl, SOUNDDLLPRO_PAR_BUFSIZE, SoundBufsizeBest(), VAL_POS);
      #ifdef TEST_INIT_DEBUG
      WriteDebugString("Initialize 7.4.1");
      #endif
      #ifdef TEST_INIT_DEBUG
      WriteDebugString("Initialize 7.5");
      #endif
      // ... and number of software buffers
      int nNumProcBufs = (int)GetInt(psl, SOUNDDLLPRO_PAR_NUMBUFS, 10, VAL_POS_OR_ZERO);
      #ifdef TEST_INIT_DEBUG
      WriteDebugString("Initialize 7.6");
      #endif
      m_bFreezeSampleRateOnStart = GetInt(psl, SOUNDDLLPRO_PAR_FREEZESRATE, 0, VAL_POS_OR_ZERO) > 0;
      // create buffers. NOTE: for 'visualization' callback we always use a
      // 'reasonable' large queue, here: 16384 samples
      // CHANGE ON 20.10.2008: larger visualization queue: this queue has a lower priority
      // than processing queue , so we use 10*nNumbufs, with a minimum of 16384 samples
      // and minimum of 50 buffers
      int nNumBufsVis = 10*nNumProcBufs;
      if (nNumBufsVis < (16384 / nBufSize))
         nNumBufsVis =  16384 / nBufSize;
      if (nNumBufsVis < 50)
         nNumBufsVis = 50;
      #ifdef TEST_INIT_DEBUG
      WriteDebugString("Initialize 8");
      #endif
      m_nNumProcBufs = (unsigned int)nNumProcBufs;
      CreateBuffers(vbIn, vbOut, nBufSize, nNumProcBufs, nNumBufsVis);
      GetSoundDataExchanger()->m_bCaptureDoneProcessed = m_bCaptureDoneProcessed;
      }
   __finally
      {
      TRYDELETENULL(pslTmp);
      }
}
//------------------------------------------------------------------------------

//------------------------------------------------------------------------------
/// returns number of ASIO drivers
//------------------------------------------------------------------------------
size_t SoundClassAsio::SoundNumDrivers()
{
   return NumDrivers();
}
//------------------------------------------------------------------------------

//------------------------------------------------------------------------------
/// returns name of ASIO driver by index
//------------------------------------------------------------------------------
AnsiString SoundClassAsio::SoundDriverName(unsigned int iIndex)
{
   return DriverNameAtIndex(iIndex);
}
//------------------------------------------------------------------------------

//------------------------------------------------------------------------------
/// returns index of current ASIO driver
//------------------------------------------------------------------------------
unsigned int SoundClassAsio::SoundCurrentDriverIndex(void)
{
   return (unsigned int)CurrentDriverIndex();
}
//------------------------------------------------------------------------------

//------------------------------------------------------------------------------
/// returns number of input or ouput channels of current ASIO driver
//------------------------------------------------------------------------------
long SoundClassAsio::SoundChannels(Direction adDirection)
{
   return HardwareChannels(adDirection);
}
//------------------------------------------------------------------------------

//------------------------------------------------------------------------------
/// returns channel name of input or ouput channel of current ASIO driver
//------------------------------------------------------------------------------
AnsiString SoundClassAsio::SoundChannelName(unsigned int iChannelIndex, Direction adDirection)
{
   return ChannelName((int)iChannelIndex, adDirection);
}
//------------------------------------------------------------------------------

//------------------------------------------------------------------------------
/// returns number of ACTIVE input or output channels of current ASIO driver
//------------------------------------------------------------------------------
size_t SoundClassAsio::SoundActiveChannels(Direction adDirection)
{
   return ActiveChannels(adDirection);
}
//------------------------------------------------------------------------------

//------------------------------------------------------------------------------
/// returns name of an ACTIVE input or output channel of current ASIO driver
//------------------------------------------------------------------------------
AnsiString SoundClassAsio::SoundGetActiveChannelName(unsigned int iChannelIndex, Direction adDirection)
{
   return m_vvsActiveChNames[adDirection][iChannelIndex];
}
//------------------------------------------------------------------------------

//------------------------------------------------------------------------------
/// returns current buffer size of current ASIO driver
//------------------------------------------------------------------------------
long SoundClassAsio::SoundBufsizeCurrent()
{
   return BufsizeCurrent();
}
//------------------------------------------------------------------------------

//------------------------------------------------------------------------------
/// returns best buffer size of current ASIO driver
//------------------------------------------------------------------------------
long SoundClassAsio::SoundBufsizeBest()
{
   return BufsizeBest();
}
//------------------------------------------------------------------------------

//------------------------------------------------------------------------------
/// returns number of processing buffers
//------------------------------------------------------------------------------
long SoundClassAsio::SoundNumBufOut()
{
   return (long)m_nNumProcBufs;
}
//------------------------------------------------------------------------------

//------------------------------------------------------------------------------
/// shows control panel of current ASIO driver
//------------------------------------------------------------------------------
void SoundClassAsio::SoundShowControlPanel(void)
{
   ShowControlPanel();
}
//------------------------------------------------------------------------------

//------------------------------------------------------------------------------
/// starts ASIO device
//------------------------------------------------------------------------------
void SoundClassAsio::SoundStart()
{
   m_nProcessCalls = 0;
   // update sample rate with current one BEFORE start to have it 'up-to-date'
   // for "m_bFreezeSampleRateOnStart == true" mode
   m_dSampleRate = SoundGetSampleRate();
   Start();
}
//------------------------------------------------------------------------------

//------------------------------------------------------------------------------
/// stops ASIO device with optional waiting for 'real-stop-is-done'
//------------------------------------------------------------------------------
void SoundClassAsio::SoundStop(bool bWaitForStopDone)
{
   Stop();
   // wait for stop to be really done
   if (bWaitForStopDone)
      {
      if (WAIT_TIMEOUT == WaitForSingleObject(m_hStoppedEvent, 3000))
         {
         if (SoundIsRunning())
            throw Exception("timeout occurred on stopping device");
         }
      }
   m_nProcessCalls = 0;
}
//------------------------------------------------------------------------------

//------------------------------------------------------------------------------
/// returns true if stae is RUNNING or false else
//------------------------------------------------------------------------------
bool SoundClassAsio::SoundIsRunning()
{
   return (GetState() == Asio::RUNNING);
}
//------------------------------------------------------------------------------

//------------------------------------------------------------------------------
/// returns m_bStopping
//------------------------------------------------------------------------------
bool SoundClassAsio::SoundIsStopping()
{
   return m_bStopping;
}
//------------------------------------------------------------------------------

//------------------------------------------------------------------------------
/// returns maximum value detected in an active channel
//------------------------------------------------------------------------------
float SoundClassAsio::SoundActiveChannelMaxValue(Direction adDirection, size_t nChannel)
{
   return m_vvfAciveChMaxValues[adDirection][nChannel];
}
//------------------------------------------------------------------------------

//------------------------------------------------------------------------------
/// returns minimum value detected in an active channel
//------------------------------------------------------------------------------
float SoundClassAsio::SoundActiveChannelMinValue(Direction adDirection, size_t nChannel)
{
   return m_vvfAciveChMinValues[adDirection][nChannel];
}
//------------------------------------------------------------------------------

//------------------------------------------------------------------------------
/// sets sample rate of current device
//------------------------------------------------------------------------------
void SoundClassAsio::SoundSetSampleRate(double dSampleRate)
{
   SampleRate(dSampleRate);
}
//------------------------------------------------------------------------------

//------------------------------------------------------------------------------
/// checks, if a particular samplerate is supported by device
//------------------------------------------------------------------------------
bool SoundClassAsio::SoundCanSampleRate(double dSampleRate)
{
   return CanSampleRate(dSampleRate);
}
//------------------------------------------------------------------------------

//------------------------------------------------------------------------------
/// returns current samplerate
//------------------------------------------------------------------------------
double SoundClassAsio::SoundGetSampleRate(void)
{
   if (m_bFreezeSampleRateOnStart && GetState() == RUNNING)
      return m_dSampleRate;
   else
      return SampleRate();
}
//------------------------------------------------------------------------------

//------------------------------------------------------------------------------
/// returns latency
//------------------------------------------------------------------------------
long SoundClassAsio::SoundGetLatency(Direction adDirection)
{
   return Latency(adDirection);
}
//------------------------------------------------------------------------------

//------------------------------------------------------------------------------
/// returns m_nWatchdogTimeout
//------------------------------------------------------------------------------
unsigned int SoundClassAsio::SoundGetWatchdogTimeout(void)
{
   return m_nWatchdogTimeout;
}
//------------------------------------------------------------------------------

//------------------------------------------------------------------------------
/// Callback for external buffer size change: forbidden. Stops device and sets error
//------------------------------------------------------------------------------
void SoundClassAsio::OnBufferSizeChangeChange()
{
   m_strFatalError = "the buffersize of the driver was changed externally.";
   Stop();
}
//------------------------------------------------------------------------------

//------------------------------------------------------------------------------
/// Callback for external reset request: forbidden. Stops device and sets error
//------------------------------------------------------------------------------
void SoundClassAsio::OnResetRequest()
{
   m_strFatalError = "an external driver reset was requested. Most probably driver settings were changed externally.";
   Stop();
}
//------------------------------------------------------------------------------

//------------------------------------------------------------------------------
/// \brief called by base class on change of sampling frequency.
/// \param[in] dSrate sampling frequency
/// \exception Asio::EAsioError if device is running and change is not allowed
//------------------------------------------------------------------------------
#pragma argsused
void SoundClassAsio::OnRateChange(double dSrate)
{
   OutputDebugString("rate change");
   m_strFatalError = "sample rate change detected on running device";
   Stop();
}
//------------------------------------------------------------------------------

//------------------------------------------------------------------------------
/// Overloaded CAsio callback called if 'done-loop' is really finished
/// Calls user defined callback
//------------------------------------------------------------------------------
void SoundClassAsio::OnDoneLoopStopped()
{
   if (m_lpfnOnStopComplete)
	  m_lpfnOnStopComplete();
}
//------------------------------------------------------------------------------

//------------------------------------------------------------------------------
/// Overloaded CAsio callback called OnHang
/// Calls user defined callback
//------------------------------------------------------------------------------
void SoundClassAsio::OnHang()
{
   if (m_lpfnOnHang)
	  m_lpfnOnHang();
}
//------------------------------------------------------------------------------

//------------------------------------------------------------------------------
/// Overloaded CAsio callback called on fatal errors
/// Calls user defined callback
//------------------------------------------------------------------------------
void SoundClassAsio::OnFatalError()
{
   if (m_lpfnOnError)
	  m_lpfnOnError();
}
//------------------------------------------------------------------------------

//------------------------------------------------------------------------------
/// Overloaded CAsio callback called on state changes
/// Calls user defined callback
//------------------------------------------------------------------------------
#pragma argsused
void SoundClassAsio::OnStateChange(State asState)
{
   if (m_lpfnOnStateChange)
      m_lpfnOnStateChange(GetState());
}
//------------------------------------------------------------------------------

//------------------------------------------------------------------------------
/// Overloaded CAsio callback called on xruns
/// Calls user defined callback
//------------------------------------------------------------------------------
void SoundClassAsio::OnXrun(XrunType xtXrunType)
{
   if (m_lpfnOnXrun)
	  m_lpfnOnXrun(xtXrunType);
}
//------------------------------------------------------------------------------

//------------------------------------------------------------------------------
/// Overloaded CAsio processing callback
/// Calls user defined callback
//------------------------------------------------------------------------------
#pragma argsused
void SoundClassAsio::Process( SoundData & sdBuffersIn,
                              SoundData & sdBuffersOut,
                              unsigned nBuffersInWaiting,
                              bool bPreloading)
{
   m_nProcessCalls++;
   if (m_lpfnOnProcess)
      m_lpfnOnProcess(sdBuffersIn.m_vvfData, sdBuffersOut.m_vvfData, sdBuffersOut.m_bIsLast);
}
//------------------------------------------------------------------------------

//------------------------------------------------------------------------------
/// Overloaded CAsio OnBufferPlay callback
/// Calls user defined callback
//------------------------------------------------------------------------------
void SoundClassAsio::OnBufferPlay(SoundData & sdBuffersOut)
{
   if (m_lpfnOnBufferPlay)
	  m_lpfnOnBufferPlay(sdBuffersOut.m_vvfData);
}
//------------------------------------------------------------------------------

//------------------------------------------------------------------------------
/// Overloaded CAsio OnBufferDone callback
/// Calls user defined callback
//------------------------------------------------------------------------------
#pragma argsused
void SoundClassAsio::OnBufferDone(  SoundData & sdBuffersIn,
                                    SoundData & sdBuffersOut,
                                    long nBuffersWaiting)
{
   if (m_lpfnOnBufferDone)
      {
      m_lpfnOnBufferDone(sdBuffersIn.m_vvfData, sdBuffersOut.m_vvfData, sdBuffersOut.m_bIsLast);
      }
}
//------------------------------------------------------------------------------

//------------------------------------------------------------------------------
// returns fix asio format for soundmexpro
//------------------------------------------------------------------------------
AnsiString SoundClassAsio::SoundFormatString()
{
   return "32-bit IEEE float";
}
//------------------------------------------------------------------------------
