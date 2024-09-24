//------------------------------------------------------------------------------
/// \file SoundDllPro_SoundClassMMDevice.h
/// \author Berg
/// \brief Interface of sound class for SoundMexPro. Inherits form
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
#ifndef SoundDllPro_SoundClassMMDeviceH
#define SoundDllPro_SoundClassMMDeviceH
//---------------------------------------------------------------------------

#include <windows.h>
#include <mmreg.h>
#include "SoundDllPro_SoundClassBase.h"
#include "SoundSoftwareBuffer.h"
#include <casioEnums.h>
#include <mmdeviceapi.h>

/// define for nzmber of supported channels
#define MMDEVICE_NUM_CHANNELS    2

//------------------------------------------------------------------------------
/// structure containing info about a IMMDevice, prefix dp
//------------------------------------------------------------------------------
struct IMMDeviceProps
{
   UnicodeString  m_usId;              /// ID of device
   UnicodeString  m_usName;            /// description of device
   UnicodeString  m_usFriendlyName;    /// description of device
   UnicodeString  m_usInterface;       /// friendly name of device interface
};
//------------------------------------------------------------------------------

//------------------------------------------------------------------------------
/// enum for different supported device wave formats, prefix dwfmt
//------------------------------------------------------------------------------
enum THtMMDeviceFormat
{
   MMD_WFMT_NONE = 0,         /// dummy for NONE
   MMD_WFMT_PCMIEEEFLOAT,     /// 32bit IEEE float format (normalized)
   MMD_WFMT_PCM32INT,         /// 32bit integer PCM
   MMD_WFMT_PCM24INT,         /// 24bit integer PCM
   MMD_WFMT_PCM2432INT,       /// 24bit significant integer bits PCM in 32bit container
   MMD_WFMT_PCM16INT,         /// 16bit integer PCM
   MMD_WFMT_NUMFORMATS        /// dummy for last supported format
};
//------------------------------------------------------------------------------

//------------------------------------------------------------------------------
/// enum for module events
//------------------------------------------------------------------------------
enum THtMMDeviceEvents
{
   MMD_EVENT_PROCESS = 0,
   MMD_EVENT_EXIT,
   MMD_EVENT_NUMEVENTS
};
//------------------------------------------------------------------------------

/// forward declarations
class    SoundClassMMDevice;
class    IAudioClient;
class    IAudioRenderClient;

//------------------------------------------------------------------------------
/// \class CHtMMDProcessingThread, processing thread used by CHtMMDevice,
/// prefix mmdpt
//------------------------------------------------------------------------------
class CHtMMDProcessingThread : public TThread
{
   public:
      CHtMMDProcessingThread(SoundClassMMDevice* phtcbs);
   protected:
      SoundClassMMDevice*      m_phtmmd;        ///< owning CHtMMDevice instance
      void __fastcall   Execute();
};
//------------------------------------------------------------------------------

//------------------------------------------------------------------------------
/// \class SoundClassMMDevice. Inherits SoundClassBase.
/// Implements all abstract functions for use with Wdm
//------------------------------------------------------------------------------
class SoundClassMMDevice : public SoundClassBase
{
   friend class CHtMMDProcessingThread;
   friend class TSoundSoftwareBuffer;

   private:
      CRITICAL_SECTION           m_csProcess;         ///< critical section processing
      TSoundSoftwareBuffer*      m_pssbSoftwareBuffer;   ///< instance of software buffering class
      std::vector<IMMDeviceProps >  m_vdp;            ///< vector containing info on all MMDevices
      THtMMDeviceFormat          m_dwfmt;             ///< internal device format
      DWORD                      m_nSampleRate;       ///< current samplerate
      CHtMMDProcessingThread*    m_pmmdpt;            ///< processing thread instance
      UINT32                     m_nBufsizeSamples;   ///< buffer size in samples
      UINT32                     m_nBufsizeBytes;     ///< buffer size in bytes with respect to BlockAlign
      UINT32                     m_nNumBuffers;       ///< number of buffers
      bool                       m_bRunning;          ///< flag if sound is running
      vvf                        m_vvfDummy;          ///< internal float buffer
      vvf                        m_vvfBuffer;         ///< internal float buffer
      unsigned int               m_nDeviceIndex;      ///< current device index
      IMMDevice*                 m_pDevice;           ///< IMMDevice instance
      IAudioClient*              m_pAudioClient;      ///< IAudioClient instance
      IAudioRenderClient*        m_pRenderClient;     ///< IAudioRenderClient instance
      HANDLE                     m_hEvents[MMD_EVENT_NUMEVENTS];  ///< array with events for processing/stopping
      UnicodeString              m_strProcessingError;            ///< string for storing 'type' of processing error


      void                       Cleanup(void);
      void                       ProcessInternal(vvf & vvfBuffer);
      void                       Process(void);
      void                       InternalFloatBufferToDeviceBuffer(BYTE *pData);
      void                       HtMMDeviceFormat2WaveFormatExtensible(WAVEFORMATEXTENSIBLE &rwfx, THtMMDeviceFormat dwfmt, DWORD nSampleRate);
      bool                       ListDevices();
      void                       StopSoftwareBuffer();


      unsigned int               m_nSamplesPlayed;
      unsigned int               m_nSampleStopPos;
      // device members
      bool                       m_bInitialized;
      bool                       m_bStopping;



      void     InitByIndex(int nDeviceId,
                           int nSampleRate,
                           int nBitsPerSample,
                           int nBufferSizeBytes,
                           int nNumBuffers
                           );
      void     InitByName( AnsiString strDeviceName,
                           int nSampleRate,
                           int nBitsPerSample,
                           int nBufferSizeBytes,
                           int nNumBuffers
                           );
      void     SoundExit();
      void     OnXRun(void);
      void     OnError(void);
   public:
      SoundClassMMDevice();
      ~SoundClassMMDevice();
      virtual void         SoundInit(TStringList* psl);
      virtual bool         SoundInitialized();
      virtual void         SoundLoadDriverByIndex(size_t nIndex);
      virtual void         SoundLoadDriverByName(AnsiString strName);
      virtual void         SoundUnloadDriver();
      virtual size_t       SoundNumDrivers();
      virtual AnsiString   SoundDriverName(unsigned int iIndex);
      virtual unsigned int SoundCurrentDriverIndex(void);
      virtual long         SoundChannels(Asio::Direction adDirection);
      virtual AnsiString   SoundChannelName(unsigned int iChannelIndex, Asio::Direction adDirection);
      virtual void         SoundStart();
      virtual void         SoundStop(bool bWaitForStopDone);
      virtual bool         SoundIsRunning();
      virtual bool         SoundIsStopping();
      virtual size_t       SoundActiveChannels(Asio::Direction adDirection);
      virtual AnsiString   SoundGetActiveChannelName(unsigned int iChannelIndex, Asio::Direction adDirection);
      virtual long         SoundBufsizeCurrent();
      virtual long         SoundBufsizeBest();
      virtual long         SoundNumBufOut();
      virtual void         SoundSetSampleRate(double dSampleRate);
      virtual bool         SoundCanSampleRate(double dSampleRate);
      virtual double       SoundGetSampleRate(void);
      virtual float        SoundActiveChannelMaxValue(Asio::Direction adDirection, size_t nChannel);
      virtual float        SoundActiveChannelMinValue(Asio::Direction adDirection, size_t nChannel);
      void                 SoundOnHang();
      virtual AnsiString   SoundFormatString();
      static UnicodeString GetFormatDescription(THtMMDeviceFormat dwfmt);

};
//------------------------------------------------------------------------------

//------------------------------------------------------------------------------
/// helper define to safley release a MMDevice-API object reference
/// \param[in] pointer to object to release
//------------------------------------------------------------------------------
#define SAFE_RELEASE(p)  \
   {\
   if (p != NULL)\
      {\
      try\
         {\
         p->Release();\
         }\
      catch(...)\
         {\
         }\
      p = NULL;\
      }\
   }\
//------------------------------------------------------------------------------

//------------------------------------------------------------------------------
/// helper define to safley call CoTaskMemFree on an object
/// \param[in] pointer to object to call CoTaskMemFree for
//------------------------------------------------------------------------------
#define SAFE_COTASKMEMFREE(p) \
   {\
   if (p != NULL)\
      {\
      try\
         {\
         CoTaskMemFree(p);\
         }\
      catch(...)\
         {\
         }\
      p = NULL;\
      }\
   }\
//------------------------------------------------------------------------------

#endif
