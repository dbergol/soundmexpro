//------------------------------------------------------------------------------
/// \file SoundDllPro_SoundClassMMDevice.cpp
/// \author Berg
/// \brief Implementation of sound class for SoundMexPro. Inherits form
/// SoundClassBase. Implements all abstract functions for use with MMDevice API
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
#pragma warn -8026
#pragma hdrstop

#include "SoundDllPro_SoundClassMMDevice.h"
#include <Avrt.h>
#include <math.h>
#include <initguid.h>
#include <functiondiscoveryapi.h>

#include "Audiopolicy.h"
#include "Audioclient.h"
#pragma warn +8026


#include "soundmexpro_defs.h"
#include "SoundDllPro_Tools.h"

//------------------------------------------------------------------------------

#pragma package(smart_init)
#pragma warn -use
//------------------------------------------------------------------------------
using namespace Asio;

//------------------------------------------------------------------------------
/// constants needed to intialize GUIDs of Com classes
//------------------------------------------------------------------------------
const CLSID CLSID_MMDeviceEnumerator   = __uuidof(MMDeviceEnumerator);
const IID IID_IMMDeviceEnumerator      = __uuidof(IMMDeviceEnumerator);
const IID IID_IAudioClient             = __uuidof(IAudioClient);
const IID IID_IAudioRenderClient       = __uuidof(IAudioRenderClient);
//------------------------------------------------------------------------------

//------------------------------------------------------------------------------
/// CHtMMDProcessingThread
/// Processing thread of class SoundClassMMDevice
//------------------------------------------------------------------------------
CHtMMDProcessingThread::CHtMMDProcessingThread(SoundClassMMDevice* phtmmd)
:  TThread(true),
   m_phtmmd(phtmmd)
{
   FreeOnTerminate = false;
   if (!m_phtmmd)
      throw Exception("invalid instance passed to processing thread");
}
//------------------------------------------------------------------------------

//------------------------------------------------------------------------------
/// Thread function. Calls Processing function of SoundClassMMDevice on buffer events
//------------------------------------------------------------------------------
void __fastcall CHtMMDProcessingThread::Execute()
{
   // Ask MMCSS to temporarily boost the thread priority
   // to reduce glitches while a low-latency stream plays.
   DWORD dwTaskIndex = 0;
   HANDLE hProcessTask = AvSetMmThreadCharacteristics(TEXT("Pro Audio"), &dwTaskIndex);
   if (!hProcessTask)
      throw Exception("error setting thread characteristics");

   try
      {
      while (!Terminated)
         {
         // Wait for next buffer or exit event to be signaled.
         DWORD dw = WaitForMultipleObjects(MMD_EVENT_NUMEVENTS, &m_phtmmd->m_hEvents[0], false, 2000);

         // call processing
         if (dw == WAIT_OBJECT_0 + MMD_EVENT_PROCESS)
            {
            m_phtmmd->Process();
            }
         // exit event
         else if (dw == WAIT_OBJECT_0 + MMD_EVENT_EXIT)
            {
            Terminate();
            }
         // timeout
         else if (dw == WAIT_TIMEOUT)
            {
            // if device is really running, then it's a hang!
            if (m_phtmmd->SoundIsRunning())
               m_phtmmd->SoundOnHang();
            }
         }
      }
   __finally
      {
      if (hProcessTask)
         {
         // Reset thread priority
         AvRevertMmThreadCharacteristics(hProcessTask);
         hProcessTask = NULL;
         }
      }
}
//------------------------------------------------------------------------------

//------------------------------------------------------------------------------
// contructor, initializes members, creates events and device thread
//------------------------------------------------------------------------------
SoundClassMMDevice::SoundClassMMDevice()
   :  SoundClassBase(),
      m_pssbSoftwareBuffer(NULL),
      m_nSampleRate(0),
      m_pmmdpt(NULL),
      m_nBufsizeSamples(0),
      m_nBufsizeBytes(0),
      m_nNumBuffers(0),
      m_bRunning(false),
      m_nDeviceIndex(0),
      m_pDevice(NULL),
      m_pAudioClient(NULL),
      m_pRenderClient(NULL),
      m_nSamplesPlayed(0),
      m_nSampleStopPos(0),
      m_bInitialized(false),
      m_bStopping(false)
{
   InitializeCriticalSection(&m_csProcess);

   CoInitialize(NULL);
   ListDevices();

   // Create events
   int n;
   for (n = 0; n < MMD_EVENT_NUMEVENTS; n++)
      {
      m_hEvents[n] = CreateEvent(NULL, FALSE, FALSE, NULL);
      if (m_hEvents[n] == NULL)
         throw Exception("error creating buffer event");
      }
   // create processing thread
   m_pmmdpt = new CHtMMDProcessingThread(this);
   m_pmmdpt->Start();
}
//------------------------------------------------------------------------------

//------------------------------------------------------------------------------
// destructor, does cleanup
//------------------------------------------------------------------------------
SoundClassMMDevice::~SoundClassMMDevice()
{
   Cleanup();
   DeleteCriticalSection(&m_csProcess);
   CoUninitialize();
}
//------------------------------------------------------------------------------

//------------------------------------------------------------------------------
/// Does cleanup. Only to be called by destructor 
//------------------------------------------------------------------------------
void SoundClassMMDevice::Cleanup(void)
{
   // Stop device first to be sure device does not trigger thread function any more
   try
      {
      SoundStop(false);
      }
   catch(...)
      {
      }

   // if thread is still running, signal a break and wait for it
   if (m_pmmdpt && !m_pmmdpt->Suspended)
      {
      SetEvent(m_hEvents[MMD_EVENT_EXIT]);
      m_pmmdpt->WaitFor();
      }
   TRYDELETENULL(m_pmmdpt);

   SoundExit();

   // cleanup events
   int n;
   for (n = 0; n < MMD_EVENT_NUMEVENTS; n++)
      {
      if (m_hEvents[n])
         {
         CloseHandle(m_hEvents[n]);
         m_hEvents[n] = NULL;
         }
      }
   // cleanup SoftwareBuffer
   StopSoftwareBuffer();
}
//------------------------------------------------------------------------------

//------------------------------------------------------------------------------
/// cleans up software buffer
//------------------------------------------------------------------------------
void SoundClassMMDevice::StopSoftwareBuffer()
{
   if (m_pssbSoftwareBuffer && !m_pssbSoftwareBuffer->Suspended)
      {
      m_pssbSoftwareBuffer->Terminate();
      m_pssbSoftwareBuffer->WaitFor();
      }
   TRYDELETENULL(m_pssbSoftwareBuffer);
}
//------------------------------------------------------------------------------

//------------------------------------------------------------------------------
// returns true if mmdevice is initialized or false otherwise
//------------------------------------------------------------------------------
bool SoundClassMMDevice::SoundInitialized()
{
   return !!m_pRenderClient;
}
//------------------------------------------------------------------------------

//------------------------------------------------------------------------------
/// Stops device and resets members
//------------------------------------------------------------------------------
void SoundClassMMDevice::SoundExit()
{
   // Stop device non-smooth
   try
      {
      SoundStop(false);
      }
   catch(...)
      {
      }

   SAFE_RELEASE(m_pRenderClient);
   SAFE_RELEASE(m_pAudioClient);
   SAFE_RELEASE(m_pDevice);

   m_nBufsizeSamples = 0;
   m_nBufsizeBytes   = 0;
}
//------------------------------------------------------------------------------

//------------------------------------------------------------------------------
// loads mmdevide driver by index
//------------------------------------------------------------------------------
void SoundClassMMDevice::SoundLoadDriverByIndex(size_t nIndex)
{
   if (nIndex >= SoundNumDrivers())
      throw Exception("driver index out of range");

   SoundLoadDriverByName(SoundDriverName((unsigned int)nIndex));
}
//------------------------------------------------------------------------------

//------------------------------------------------------------------------------
// loads mmdevide driver by name
//------------------------------------------------------------------------------
void SoundClassMMDevice::SoundLoadDriverByName(AnsiString strName)
{
   // search for driver
   unsigned int nIndex;
   for (nIndex = 0; nIndex < SoundNumDrivers(); nIndex++)
      {
      if (m_vdp[nIndex].m_usFriendlyName == strName)
         break;
      }

   if (nIndex == SoundNumDrivers())
      throw Exception("driver with requested name not found");

   m_nDeviceIndex = nIndex;

   // now all values are valid - as far we know it now. Try to activate device
   IMMDeviceEnumerator *pDeviceEnumerator = NULL;
   HRESULT hr;
   try
      {
      try
         {
         // create device enumarator
         hr = CoCreateInstance(  __uuidof(MMDeviceEnumerator),
                                 NULL,
                                 CLSCTX_INPROC_SERVER,
                                 __uuidof(IMMDeviceEnumerator),
                                 (void**)&pDeviceEnumerator);
         if (FAILED(hr))
            throw Exception("error creating MMDeviceEnumerator");

         // get our device
         hr = pDeviceEnumerator->GetDevice(m_vdp[m_nDeviceIndex].m_usId.w_str(), &m_pDevice);
         if (FAILED(hr))
            throw Exception("error retrieving device");

         // avtivate it
         hr = m_pDevice->Activate(IID_IAudioClient, CLSCTX_ALL, NULL, (void**)&m_pAudioClient);
         if (FAILED(hr))
            throw Exception("error activating device");
         }
      catch (...)
         {
         SAFE_RELEASE(m_pDevice);
         SAFE_RELEASE(m_pAudioClient);
         throw;
         }
      }
   __finally
      {
      SAFE_RELEASE(pDeviceEnumerator);
      }

   if (m_lpfnOnStateChange)
      m_lpfnOnStateChange(Asio::LOADED);

}
//------------------------------------------------------------------------------

//------------------------------------------------------------------------------
// unloads mmdevide driver
//------------------------------------------------------------------------------
void SoundClassMMDevice::SoundUnloadDriver()
{
   SAFE_RELEASE(m_pDevice);
   SAFE_RELEASE(m_pAudioClient);
   m_bInitialized = false;
   if (m_lpfnOnStateChange)
      m_lpfnOnStateChange(Asio::FREE);
}
//------------------------------------------------------------------------------

//------------------------------------------------------------------------------
/// Reads configuration values from passed stringlist and initializes
//  m_pAudioClient
//------------------------------------------------------------------------------
void SoundClassMMDevice::SoundInit(TStringList* psl)
{
   if (SoundIsRunning())
      throw Exception("cannot reinitialize device while it is running!");

   if (!m_pAudioClient)
      throw Exception("cannot init device before driver loaded");

   StopSoftwareBuffer();
   // read number of software buffers
   m_nNumBuffers = (unsigned int)GetInt(psl, SOUNDDLLPRO_PAR_NUMBUFS, 20, VAL_POS_OR_ZERO);
   // mmdevice buffers (not SoftwareBuffers!!)
   int nWdmNumBuffers = (int)GetInt(psl, SOUNDDLLPRO_PAR_WDMNUMBUFS, 10, VAL_POS_OR_ZERO);
   if (nWdmNumBuffers < 6)
      nWdmNumBuffers = 6;

   HRESULT hr;
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


      // find out, which WAVEFORMATEXTENSIBLE to be used
      WAVEFORMATEXTENSIBLE wfx;
      ZeroMemory(&wfx, sizeof(WAVEFORMATEXTENSIBLE));
      // do a loop through supported formats: we use the first that is fine (they
      // are sorted in enum to have 'the best' format listed first)
      m_dwfmt = MMD_WFMT_NONE;
      int nFormat;
      for(nFormat = MMD_WFMT_NONE+1; nFormat < MMD_WFMT_NUMFORMATS; nFormat++)
         {
         HtMMDeviceFormat2WaveFormatExtensible(wfx, (THtMMDeviceFormat)nFormat, m_nSampleRate);
         // ask AudioClient if it suports the format
         hr = m_pAudioClient->IsFormatSupported(AUDCLNT_SHAREMODE_EXCLUSIVE, (WAVEFORMATEX*)&wfx, NULL);
         if (SUCCEEDED(hr))
            {
            m_dwfmt = (THtMMDeviceFormat)nFormat;
            break;
            }
         }
      if (m_dwfmt == MMD_WFMT_NONE)
         throw Exception("device does not support any of the sound formats supported by module");

      REFERENCE_TIME hnsRequestedDuration = 0;
      // retrieve minimum supported buffer size
      hr = m_pAudioClient->GetDevicePeriod(NULL, &hnsRequestedDuration);
      if (FAILED(hr))
         throw Exception("error retrieving device period");
      // apply factor from presets
      hnsRequestedDuration *= nWdmNumBuffers;

      hr = m_pAudioClient->Initialize(
                              AUDCLNT_SHAREMODE_EXCLUSIVE,
                              AUDCLNT_STREAMFLAGS_EVENTCALLBACK | AUDCLNT_STREAMFLAGS_NOPERSIST | AUDCLNT_SESSIONFLAGS_DISPLAY_HIDE,
                              hnsRequestedDuration,
                              hnsRequestedDuration,
                              (WAVEFORMATEX*)&wfx,
                              NULL);

      // Alignment dance according to Microsoft documentation of IAudioClient::Initialize
      //
      // "Starting with Windows 7, Initialize can return AUDCLNT_E_BUFFER_SIZE_NOT_ALIGNED for a render
      // or a capture device. This indicates that the buffer size, specified by the caller in the
      // hnsBufferDuration parameter, is not aligned. This error code is returned only if the caller
      // requested an exclusive-mode stream (AUDCLNT_SHAREMODE_EXCLUSIVE) and event-driven buffering
      // (AUDCLNT_STREAMFLAGS_EVENTCALLBACK).
      // If Initialize returns AUDCLNT_E_BUFFER_SIZE_NOT_ALIGNED, the caller must call Initialize again
      // and specify the aligned buffer size. Use the following steps:
      // 1. Call IAudioClient::GetBufferSize and receive the next-highest-aligned buffer size (in frames).
      // 2. Call IAudioClient::Release to release the audio client used in the previous call that returned AUDCLNT_E_BUFFER_SIZE_NOT_ALIGNED.
      // 3. Calculate the aligned buffer size in 100-nansecond units (hns). The buffer size is
      //    (REFERENCE_TIME)((10000.0 * 1000 / WAVEFORMATEX.nSamplesPerSecond * nFrames) + 0.5).
      //    In this formula, nFrames is the buffer size retrieved by GetBufferSize.
      // 4. Call the IMMDevice::Activate method with parameter iid set to REFIID IID_IAudioClient to create a new audio client.
      // 5. Call Initialize again on the created audio client and specify the new buffer size and periodicity."
      // (citation end)
      if (hr == AUDCLNT_E_BUFFER_SIZE_NOT_ALIGNED)
         {
         hr = m_pAudioClient->GetBufferSize(&m_nBufsizeSamples);
         if (FAILED(hr))
            throw Exception("error 2 retrieving device buffer size");
         SAFE_RELEASE(m_pAudioClient);
         hnsRequestedDuration = (REFERENCE_TIME)  ((10000.0 * 1000 / wfx.Format.nSamplesPerSec * m_nBufsizeSamples) + 0.5);
         hr = m_pDevice->Activate( IID_IAudioClient, CLSCTX_ALL, NULL, (void**)&m_pAudioClient);
         if (FAILED(hr))
            throw Exception("error 2 activating device");
         hr = m_pAudioClient->Initialize(
                              AUDCLNT_SHAREMODE_EXCLUSIVE,
                              AUDCLNT_STREAMFLAGS_EVENTCALLBACK | AUDCLNT_STREAMFLAGS_NOPERSIST | AUDCLNT_SESSIONFLAGS_DISPLAY_HIDE,
                              hnsRequestedDuration,
                              hnsRequestedDuration,
                              (WAVEFORMATEX*)&wfx,
                              NULL);
         if (FAILED(hr))
            throw Exception("error 2 initializing device");
         }
      if (FAILED(hr))
         throw Exception("error initializing device");


      // register event for buffer-event notifications
      hr = m_pAudioClient->SetEventHandle(m_hEvents[MMD_EVENT_PROCESS]);
      if (FAILED(hr))
         throw Exception("error setting event handle");

      // Get the current buffe size
      hr = m_pAudioClient->GetBufferSize(&m_nBufsizeSamples);
      if (FAILED(hr))
         throw Exception("error retrieving device buffer size");
      // calculate buffer size of shared memory (needed in ::ProcessInternal)
      m_nBufsizeBytes = m_nBufsizeSamples * wfx.Format.nBlockAlign;

      // finally retrieve the audio render (playback) client service
      hr = m_pAudioClient->GetService(IID_IAudioRenderClient, (void**)&m_pRenderClient);
      if (FAILED(hr))
         throw Exception("error getting render client service");

      // finally initialize internal buffer
      unsigned int nOutChannels = (unsigned int)SoundChannels(OUTPUT);
      m_vvfBuffer.resize(nOutChannels);
      unsigned int n;
      for (n = 0; n < nOutChannels; n++)
         {
         m_vvfBuffer[n].resize(m_nBufsizeSamples);
         m_vvfBuffer[n] = 0.0f;
         }

      m_bInitialized = true;
      if (m_lpfnOnStateChange)
         m_lpfnOnStateChange(Asio::INITIALIZED);
      }
   catch(...)
      {
      SoundExit();
      throw;
      }
}
//------------------------------------------------------------------------------

//------------------------------------------------------------------------------
// returns number of MMDevice drivers
//------------------------------------------------------------------------------
size_t SoundClassMMDevice::SoundNumDrivers()
{
   return m_vdp.size();
}
//------------------------------------------------------------------------------

//------------------------------------------------------------------------------
// returns name of MMDevice driver by index
//------------------------------------------------------------------------------
AnsiString SoundClassMMDevice::SoundDriverName(unsigned int iIndex)
{
   if (iIndex >= SoundNumDrivers())
      throw Exception("driver index out of range");
   return m_vdp[iIndex].m_usFriendlyName;
}
//------------------------------------------------------------------------------

//------------------------------------------------------------------------------
// returns index of current MMDevice driver
//------------------------------------------------------------------------------
unsigned int SoundClassMMDevice::SoundCurrentDriverIndex(void)
{
   return m_nDeviceIndex;
}
//------------------------------------------------------------------------------

//------------------------------------------------------------------------------
// returns number of input or ouput channels of current MMDevice driver
//------------------------------------------------------------------------------
long SoundClassMMDevice::SoundChannels(Asio::Direction adDirection)
{
   // no input supported
   if (adDirection == Asio::INPUT)
      return 0;
   return MMDEVICE_NUM_CHANNELS;
}
//------------------------------------------------------------------------------

//------------------------------------------------------------------------------
// returns channel name of input or ouput channel of current MMDevice driver
//------------------------------------------------------------------------------
#pragma argsused
AnsiString SoundClassMMDevice::SoundChannelName(unsigned int iChannelIndex, Direction adDirection)
{
   return "Channel_" + IntToStr((int)iChannelIndex);
}
//------------------------------------------------------------------------------

//------------------------------------------------------------------------------
// returns number of ACTIVE input or output channels of current MMDevice driver
//------------------------------------------------------------------------------
size_t SoundClassMMDevice::SoundActiveChannels(Asio::Direction adDirection)
{
   return (size_t)SoundChannels(adDirection);
}
//------------------------------------------------------------------------------

//------------------------------------------------------------------------------
// returns name of an ACTIVE input or output channel of current MMDevice driver
//------------------------------------------------------------------------------
AnsiString SoundClassMMDevice::SoundGetActiveChannelName(unsigned int iChannelIndex, Direction adDirection)
{
   return SoundChannelName(iChannelIndex, adDirection);
}
//------------------------------------------------------------------------------

//------------------------------------------------------------------------------
// returns current buffer size of current MMDevice driver
//------------------------------------------------------------------------------
long SoundClassMMDevice::SoundBufsizeCurrent()
{
   return (long)m_nBufsizeSamples;
}
//------------------------------------------------------------------------------

//------------------------------------------------------------------------------
// returns best buffer size of current MMDevice driver
//------------------------------------------------------------------------------
long SoundClassMMDevice::SoundBufsizeBest()
{
   return (long)m_nBufsizeSamples;
}
//------------------------------------------------------------------------------

//------------------------------------------------------------------------------
// returns current number of buffers
//------------------------------------------------------------------------------
long SoundClassMMDevice::SoundNumBufOut()
{
   return (long)m_nNumBuffers;
}
//------------------------------------------------------------------------------

//------------------------------------------------------------------------------
// starts MMDevice device
//------------------------------------------------------------------------------
void SoundClassMMDevice::SoundStart()
{
   m_nProcessCalls = 0;
   m_nSamplesPlayed = 0;
   m_nSampleStopPos = 0;

   if (!SoundInitialized())
      throw Exception("device is not initialized");

   StopSoftwareBuffer();
   if (m_nNumBuffers)
      {
      m_pssbSoftwareBuffer = new TSoundSoftwareBuffer(this, (int)m_nNumBuffers, SoundChannels(OUTPUT), (int)m_nBufsizeSamples);
      m_pssbSoftwareBuffer->Start();
      }

   BYTE  *pData;
   // load first buffer with zeroes
   HRESULT hr = m_pRenderClient->GetBuffer(m_nBufsizeSamples, &pData);
   if (FAILED(hr))
      throw Exception("error getting device buffer");
   // NOTE: flag AUDCLNT_BUFFERFLAGS_SILENT always plays zeroes and ignores data in pData
   hr = m_pRenderClient->ReleaseBuffer(m_nBufsizeSamples, AUDCLNT_BUFFERFLAGS_SILENT);
   if (FAILED(hr))
      throw Exception("error releasing device buffer");

   hr = m_pAudioClient->Start();
   if (FAILED(hr))
      throw Exception("error starting device");

   if (m_lpfnOnStateChange)
      m_lpfnOnStateChange(Asio::RUNNING);

   m_bRunning = true;
}
//------------------------------------------------------------------------------

//------------------------------------------------------------------------------
// stops MMDevice device with optional waiting for 'real-stop-is-done'
//------------------------------------------------------------------------------
#pragma argsused
void SoundClassMMDevice::SoundStop(bool bWaitForStopDone)
{
   if (!SoundIsRunning() || !m_pAudioClient)
      return;

   EnterCriticalSection(&m_csProcess);
   try
      {
      m_bStopping = true;

      HRESULT hr = m_pAudioClient->Stop();
      if (FAILED(hr))
         throw Exception("error stopping device");
      hr = m_pAudioClient->Reset();
      Application->ProcessMessages();
      if (FAILED(hr))
         throw Exception("error resetting device");

      m_bRunning = false;

      StopSoftwareBuffer();

      m_bStopping = false;
      m_nProcessCalls = 0;

      if (m_lpfnOnStopComplete)
         m_lpfnOnStopComplete();

      if (m_lpfnOnStateChange)
         m_lpfnOnStateChange(Asio::PREPARED);
      }
   __finally
      {
      LeaveCriticalSection(&m_csProcess);
      }
}
//------------------------------------------------------------------------------

//------------------------------------------------------------------------------
// returns true if stae is RUNNING or false else
//------------------------------------------------------------------------------
bool SoundClassMMDevice::SoundIsRunning()
{
   return m_bRunning;
}
//------------------------------------------------------------------------------

//------------------------------------------------------------------------------
// returns m_bStopping
//------------------------------------------------------------------------------
bool SoundClassMMDevice::SoundIsStopping()
{
   return m_bStopping;
}
//------------------------------------------------------------------------------

//------------------------------------------------------------------------------
// returns maximum value detected in an active channel
//------------------------------------------------------------------------------
#pragma argsused
float SoundClassMMDevice::SoundActiveChannelMaxValue(Asio::Direction adDirection, size_t nChannel)
{
   // in this implementation values are always between 1 and -1
   return 1.0f;
}
//------------------------------------------------------------------------------

//------------------------------------------------------------------------------
// returns minimum value detected in an active channel
//------------------------------------------------------------------------------
#pragma argsused
float SoundClassMMDevice::SoundActiveChannelMinValue(Asio::Direction adDirection, size_t nChannel)
{
   // in this implementation values are always between 1 and -1
   return -1.0f;
}
//------------------------------------------------------------------------------

//------------------------------------------------------------------------------
// sets sample rate of current device
//------------------------------------------------------------------------------
void SoundClassMMDevice::SoundSetSampleRate(double dSampleRate)
{
   if (SoundIsRunning())
      throw Exception("cannot reinitialize device while it is running!");
   if (!SoundCanSampleRate(dSampleRate))
      throw Exception("samplerate not supported");
   m_nSampleRate = (DWORD)dSampleRate;
}
//------------------------------------------------------------------------------

//------------------------------------------------------------------------------
// checks, if a particular samplerate is supported by device
//------------------------------------------------------------------------------
bool SoundClassMMDevice::SoundCanSampleRate(double dSampleRate)
{
   if (SoundIsRunning())
      throw Exception("cannot check sample rate check on running device");

   if (!m_pAudioClient)
      throw Exception("cannot check sample rate check on unloaded driver");

   // find one if the supported waveformats running with passed samplerate
   DWORD nSampleRate = (DWORD)dSampleRate;
   WAVEFORMATEXTENSIBLE wfx;
   ZeroMemory(&wfx, sizeof(WAVEFORMATEXTENSIBLE));
   // do a loop through supported formats: we use the first that is fine (they
   // are sorted in enum to have 'the best' format listed first)
   int nFormat;
   HRESULT hr;
   for(nFormat = MMD_WFMT_NONE+1; nFormat < MMD_WFMT_NUMFORMATS; nFormat++)
      {
      HtMMDeviceFormat2WaveFormatExtensible(wfx, (THtMMDeviceFormat)nFormat, nSampleRate);
      // ask AudioClient if it suports the format
      hr = m_pAudioClient->IsFormatSupported(AUDCLNT_SHAREMODE_EXCLUSIVE, (WAVEFORMATEX*)&wfx, NULL);
      if (SUCCEEDED(hr))
         return true;
      }
   return false;
}
//------------------------------------------------------------------------------

//------------------------------------------------------------------------------
// returns current samplerate
//------------------------------------------------------------------------------
double SoundClassMMDevice::SoundGetSampleRate(void)
{
   return (double)m_nSampleRate;
}
//------------------------------------------------------------------------------

//------------------------------------------------------------------------------
/// Overloaded callback called on hangs, calls user defined callback
//------------------------------------------------------------------------------
void SoundClassMMDevice::SoundOnHang()
{
   if (m_lpfnOnHang)
      m_lpfnOnHang();
}
//------------------------------------------------------------------------------

//------------------------------------------------------------------------------
/// calls user defined XRun-function
//------------------------------------------------------------------------------
void SoundClassMMDevice::OnXRun(void)
{
   if (m_lpfnOnXrun)
      m_lpfnOnXrun(XR_PROC);
}
//------------------------------------------------------------------------------

//------------------------------------------------------------------------------
/// calls user defined Error-function
/// Calls user defined callback
//------------------------------------------------------------------------------
void SoundClassMMDevice::OnError()
{
   if (m_lpfnOnError)
      m_lpfnOnError();
}
//---------------------------------------------------------------------------

//------------------------------------------------------------------------------
/// Internal processing callback called either by ::Process(), or if Software
/// Buffers are used by TSoundSoftwareBuffer::Execute()
//------------------------------------------------------------------------------
void SoundClassMMDevice::ProcessInternal(vvf & vvfBuffer)
{
   m_nProcessCalls++;
   UINT32 nChannel;
   UINT32 nNumChannels = (UINT32)SoundChannels(OUTPUT);
   bool bIsLast = false;
   try
      {
      // to be sure: check sizes
      if (nNumChannels != vvfBuffer.size())
         throw Exception("fatal sizing error 1");
      for (nChannel = 0; nChannel < nNumChannels; nChannel++)
         {
         if (vvfBuffer[nChannel].size() != m_nBufsizeSamples)
            throw Exception("fatal sizing error 2");
         vvfBuffer[nChannel] = 0.0f;
         }

      if (m_lpfnOnProcess)
         m_lpfnOnProcess(m_vvfDummy, vvfBuffer, bIsLast);
      }
   catch (Exception &e)
      {
      m_strProcessingError = e.Message;
      try
         {
         SoundStop(false);
         }
      catch (...)
         {
         }
      OnError();
      }

   // store load-position, where last buffer was loaded
   if (bIsLast && !m_nSampleStopPos)
      m_nSampleStopPos = m_nProcessCalls * m_nBufsizeSamples;
}
//------------------------------------------------------------------------------

//------------------------------------------------------------------------------
/// Processing callback called in processing thread by CHtMMDProcessingThread::Execute
//------------------------------------------------------------------------------
void SoundClassMMDevice::Process(void)
{
   BYTE* pData;
   UINT32 nChannel;
   UINT32 nNumChannels = (UINT32)SoundChannels(OUTPUT);
   bool bIsLast = false;
   EnterCriticalSection(&m_csProcess);
   try
      {
      try
         {
         for (nChannel = 0; nChannel < nNumChannels; nChannel++)
            m_vvfBuffer[nChannel] = 0.0f;

         if (m_pssbSoftwareBuffer)
            {
            if (!m_pssbSoftwareBuffer->GetBuffer(m_vvfBuffer))
               OnXRun();
            }
         else
            ProcessInternal(m_vvfBuffer);

         if (m_lpfnOnBufferPlay)
            m_lpfnOnBufferPlay(m_vvfBuffer);

         if (m_lpfnOnBufferDone)
            m_lpfnOnBufferDone(m_vvfDummy, m_vvfBuffer, bIsLast);

         // Get buffer memory
         HRESULT hr = m_pRenderClient->GetBuffer(m_nBufsizeSamples, &pData);
         if (FAILED(hr))
            throw Exception("error getting device buffer");



         // convert data
         InternalFloatBufferToDeviceBuffer(pData);


         // tell device that we're done with this buffer
         hr = m_pRenderClient->ReleaseBuffer(m_nBufsizeSamples, 0);
         if (FAILED(hr))
            throw Exception("error releasing device buffer");

         m_nSamplesPlayed += m_nBufsizeSamples;
         }
      __finally
         {
         LeaveCriticalSection(&m_csProcess);
         }
      }
   catch (Exception &e)
      {
      m_strProcessingError = e.Message;
      try
         {
         SoundStop(false);
         }
      catch (...)
         {
         }
      OnError();
      }
   // if m_nSampleStopPos was set in ::ProcessInternal then we do the
   // automatic stop, if this positionn was reached in playback
   if (m_nSampleStopPos > 0 && m_nSamplesPlayed >= m_nSampleStopPos)
      {
      try
         {
         m_nSampleStopPos = 0;
         SoundStop(true);
         }
      catch (...)
         {
         }
      }
}
//------------------------------------------------------------------------------

//------------------------------------------------------------------------------
/// stores info about all MMDevices in internal vector
/// \retval true if function suceeds, false else
//------------------------------------------------------------------------------
bool SoundClassMMDevice::ListDevices()
{
   m_vdp.clear();
   HRESULT hr;
   IMMDeviceEnumerator *pEnumerator = NULL;
   IMMDeviceCollection *pCollection = NULL;
   IMMDevice *pDevice = NULL;
   IPropertyStore *pProps = NULL;
   LPWSTR pwszID = NULL;
   IMMDeviceProps dp;
   try
      {
      // create instance of COM object
      hr = CoCreateInstance(__uuidof(MMDeviceEnumerator), NULL, CLSCTX_ALL,
                           __uuidof(IMMDeviceEnumerator), (void**) &pEnumerator);
      if (FAILED(hr))
         return false;

      hr = pEnumerator->EnumAudioEndpoints(eRender, DEVICE_STATE_ACTIVE, &pCollection);
      if (FAILED(hr))
         return false;

      unsigned int uNumDevs, u;
      hr = pCollection->GetCount(&uNumDevs);
      if (FAILED(hr))
         return false;
      // no devices present is not an error!
      if (uNumDevs == 0)
         return true;

      // iteration through devices with skipping of devices with errors
      for (u = 0; u < uNumDevs; u++)
         {
         // Get pointer to device number i.
         hr = pCollection->Item(u, &pDevice);
         if (FAILED(hr))
            continue;

         // Get the device's ID string.
         hr = pDevice->GetId(&pwszID);
         if (SUCCEEDED(hr))
            {
            // open it's property store to retrieve a readable name
            hr = pDevice->OpenPropertyStore(STGM_READ, &pProps);
            if (SUCCEEDED(hr))
               {
               // Initialize container for property value.
               PROPVARIANT varName, varFriendlyName, varInterface;
               PropVariantInit(&varName);
               PropVariantInit(&varFriendlyName);
               PropVariantInit(&varInterface);
               // retrieve description of device. MSDN help describes as follows:
               // "The device description of the endpoint device (for example, "Speakers")."
               // This string is ususally language dependant.
               hr = pProps->GetValue(PKEY_Device_DeviceDesc, &varName);
               if (SUCCEEDED(hr))
                  {
                  // retrieve friendly name of device interface. MSDN help describes as follows:
                  // "The friendly name of the audio adapter to which the endpoint device
                  // is attached (for example, "XYZ Audio Adapter")."
                  // This string is ususally language independant. According to MSDN this
                  // name is identical to the device name returned for DirectSound devices.
                  hr = pProps->GetValue(PKEY_DeviceInterface_FriendlyName, &varInterface);
                  // on complete success add it to internal vector
                  if (SUCCEEDED(hr))
                     {
                     // retrieve friendly name of device. MSDN help describes as follows:
                     // The friendly name of the endpoint device (for example, "Speakers (XYZ Audio Adapter)")
                     // This string is ususally language dependant.
                     hr = pProps->GetValue(PKEY_Device_FriendlyName, &varFriendlyName);
                     // on complete success add it to internal vector
                     if (SUCCEEDED(hr))
                        {
                        dp.m_usId            = pwszID;
                        dp.m_usName          = varName.pwszVal;
                        dp.m_usFriendlyName  = varFriendlyName.pwszVal;
                        dp.m_usInterface     = varInterface.pwszVal;
                        m_vdp.push_back(dp);
                        }
                     }
                  }
               PropVariantClear(&varName);
               PropVariantClear(&varFriendlyName);
               PropVariantClear(&varInterface);
               }
            }
         // free data for this loop.
         // NOTE: no need call in special order: releasing objects higher in
         // hierarchy before releasing 'children' is allowed!
         SAFE_COTASKMEMFREE(pwszID);
         pwszID = NULL;
         SAFE_RELEASE(pProps);
         SAFE_RELEASE(pDevice);
         }
      return true;
      }
   __finally
      {
      // cleanup
      // NOTE: no need call in special order: releasing objects higher in
      // hierarchy before releasing 'children' is allowed!
      SAFE_COTASKMEMFREE(pwszID)
      SAFE_RELEASE(pEnumerator);
      SAFE_RELEASE(pCollection);
      SAFE_RELEASE(pDevice);
      SAFE_RELEASE(pProps);
      }
   return false;
}
//------------------------------------------------------------------------------

//------------------------------------------------------------------------------
/// fills passed WAVEFORMATEXTENSIBLE to fit to passed THtMMDeviceFormat
/// \param[out] rwfx reference to WAVEFORMATEEXTENSIBLE struct to be filled
/// \param[in] dwfmt THtMMDeviceFormat to use
/// \param[in] nSampleRate samplerate to set ing rwfx
/// \exception Exception if dwfmt is not supported
//------------------------------------------------------------------------------
void SoundClassMMDevice::HtMMDeviceFormat2WaveFormatExtensible(WAVEFORMATEXTENSIBLE &rwfx,
                                                               THtMMDeviceFormat    dwfmt,
                                                               DWORD                nSampleRate)
{
   ZeroMemory(&rwfx, sizeof(rwfx));
   rwfx.Format.wFormatTag       = WAVE_FORMAT_EXTENSIBLE;
   rwfx.Format.nChannels        = (WORD)SoundChannels(OUTPUT);
   rwfx.Format.nSamplesPerSec   = nSampleRate;
   rwfx.Format.cbSize           = sizeof(WAVEFORMATEXTENSIBLE) - sizeof(WAVEFORMATEX);
   rwfx.dwChannelMask           = SPEAKER_FRONT_LEFT | SPEAKER_FRONT_RIGHT;
   #pragma clang diagnostic push
   #pragma clang diagnostic ignored "-Wdisabled-macro-expansion"
   rwfx.SubFormat               = KSDATAFORMAT_SUBTYPE_PCM;
   #pragma clang diagnostic pop

   if (dwfmt == MMD_WFMT_PCM16INT)
      {
      rwfx.Format.wBitsPerSample = 16;
      rwfx.Samples.wValidBitsPerSample = 16;
      }
   else if (dwfmt == MMD_WFMT_PCM24INT)
      {
      rwfx.Format.wBitsPerSample = 24;
      rwfx.Samples.wValidBitsPerSample = 24;
      }
   else if (dwfmt == MMD_WFMT_PCM2432INT)
      {
      rwfx.Format.wBitsPerSample = 32;
      rwfx.Samples.wValidBitsPerSample = 24;
      }
   else if (dwfmt == MMD_WFMT_PCM32INT)
      {
      rwfx.Format.wBitsPerSample = 32;
      rwfx.Samples.wValidBitsPerSample = 32;
      }
   else if (dwfmt == MMD_WFMT_PCMIEEEFLOAT)
      {               
      #pragma clang diagnostic push
      #pragma clang diagnostic ignored "-Wdisabled-macro-expansion"
      rwfx.SubFormat = KSDATAFORMAT_SUBTYPE_IEEE_FLOAT;
      #pragma clang diagnostic pop
      rwfx.Format.wBitsPerSample = 32;
      rwfx.Samples.wValidBitsPerSample = 32;
      }
   else
      throw Exception("unsupported wave format");

   rwfx.Format.nBlockAlign       = (WORD)(rwfx.Format.nChannels*rwfx.Format.wBitsPerSample/8);
   rwfx.Format.nAvgBytesPerSec   = rwfx.Format.nBlockAlign*rwfx.Format.nSamplesPerSec;
}
//------------------------------------------------------------------------------


//------------------------------------------------------------------------------
/// writes internal data from m_vvfBuffer to passed memory with respect to current
/// audio format including hardclipping
/// NOTE: this part is implemented in separate function and NOT in ::Process
/// for device independant unit testing!
/// \param[out] pData pointer to destination memory
//------------------------------------------------------------------------------
void SoundClassMMDevice::InternalFloatBufferToDeviceBuffer(BYTE *pData)
{
   UINT32 nChannel, nFrame, nByte;
   UINT32 nNumChannels = (UINT32)SoundActiveChannels(OUTPUT);
   float fScale;
   // write zeroes to be sure that silence is played on any error
   ZeroMemory(pData, m_nBufsizeBytes);
   #pragma clang diagnostic push
   #pragma clang diagnostic ignored "-Wcast-align"
   // 32-bit bormalized float: thats exactly the internal format
   if (m_dwfmt == MMD_WFMT_PCMIEEEFLOAT)
      {
      float fValue;
      float* lp = (float*)pData;
      for (nFrame = 0; nFrame < m_nBufsizeSamples; nFrame++)
         {
         for (nChannel = 0; nChannel < nNumChannels; nChannel++)
            {
            fValue = m_vvfBuffer[nChannel][nFrame];
            if (fValue > 1.0f)
               fValue = 1.0f;
            else if (fValue < -1.0f)
               fValue = -1.0f;

            *lp++ = fValue;
            }
         }
      }
   // 16-bit integer
   else if (m_dwfmt == MMD_WFMT_PCM16INT)
      {
      int nValue;
      fScale = (float)(SHRT_MAX);
      short int* lp = (short int*)pData;
      for (nFrame = 0; nFrame < m_nBufsizeSamples; nFrame++)
         {
         for (nChannel = 0; nChannel < nNumChannels; nChannel++)
            {
            nValue = (int)floor(m_vvfBuffer[nChannel][nFrame]*fScale);
            if (nValue > SHORT_MAX)
               nValue = SHORT_MAX;
            else if (nValue < SHORT_MIN)
               nValue = SHORT_MIN;

            *lp++ = (short int)nValue;
            }
         }
      }
   // 32bit or 24bit in 32bit container
   else if (m_dwfmt == MMD_WFMT_PCM32INT || m_dwfmt == MMD_WFMT_PCM2432INT)
      {
      int64_t nValue;
      fScale = (float)(INT_MAX);
      int* lp = (int*)pData;
      for (nFrame = 0; nFrame < m_nBufsizeSamples; nFrame++)
         {
         for (nChannel = 0; nChannel < nNumChannels; nChannel++)
            {
            nValue = (int64_t)floor(m_vvfBuffer[nChannel][nFrame]*fScale);
            if (nValue > INT_MAX)
               nValue = INT_MAX;
            else if (nValue < INT_MIN)
               nValue = INT_MIN;

            *lp++ = (int)nValue;
            }
         }
      }
   // 24bit: Big Endian packed 24 Bits
   else if (m_dwfmt == MMD_WFMT_PCM24INT)
      {
      int64_t nValue;
      int nValue32;
      fScale = (float)(INT_MAX);
      signed char* lp = (signed char*)pData;
      signed char* lpVal;
      for (nFrame = 0; nFrame < m_nBufsizeSamples; nFrame++)
         {
         for (nChannel = 0; nChannel < nNumChannels; nChannel++)
            {
            nValue = (int64_t)floor(m_vvfBuffer[nChannel][nFrame]*fScale);
            if (nValue > INT_MAX)
               nValue = INT_MAX;
            else if (nValue < INT_MIN)
               nValue = INT_MIN;
            // calculate value as int32 ...
            nValue32 = (int)nValue;

            // ... and write 3 MSBs
            lpVal = (signed char*)&nValue32;
            for (nByte = 0; nByte < 3; nByte++)
               *lp++ = lpVal[nByte+1];
            }
         }
      }
   else
      throw Exception("unsupported wave format");
   #pragma clang diagnostic pop

}
//------------------------------------------------------------------------------

//------------------------------------------------------------------------------
/// returns a readable description of passed THtMMDeviceFormat
/// \param[in] dwfmt THtMMDeviceFormat to get description for
/// \retval string containing description
//------------------------------------------------------------------------------
UnicodeString SoundClassMMDevice::GetFormatDescription(THtMMDeviceFormat dwfmt)
{
   UnicodeString str = "unknown format";
   // MMD_WFMT_NUMFORMATS not handled by purpose
   #pragma clang diagnostic push
   #pragma clang diagnostic ignored "-Wswitch"
   switch (dwfmt)
      {
      case MMD_WFMT_PCM16INT:       str = "16-bit integer"; break;
      case MMD_WFMT_PCM24INT:       str = "24-bit integer"; break;
      case MMD_WFMT_PCM2432INT:     str = "24-bit integer in 32-bit container"; break;
      case MMD_WFMT_PCM32INT:       str = "32-bit integer"; break;
      case MMD_WFMT_PCMIEEEFLOAT:   str = "32-bit IEEE float"; break;
      }
   #pragma clang diagnostic pop
   return str;
}
//------------------------------------------------------------------------------

//------------------------------------------------------------------------------
// returns 'GetFormatDescription' of currently used format
//------------------------------------------------------------------------------
AnsiString SoundClassMMDevice::SoundFormatString()
{
   return GetFormatDescription(m_dwfmt);
}
//------------------------------------------------------------------------------


