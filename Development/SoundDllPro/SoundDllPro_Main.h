//------------------------------------------------------------------------------
/// \file SoundDllPro_Main.h
/// \author Berg
/// \brief Implementation of class SoundDllProMain. Main class for SoundMexPro
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
#ifndef SoundDllPro_MainH
#define SoundDllPro_MainH
//------------------------------------------------------------------------------
#include <vcl.h>
// avoid warnings from clang
#pragma clang diagnostic push
#pragma clang diagnostic ignored "-Wc++98-compat-pedantic"
#pragma clang diagnostic ignored "-Wreserved-id-macro"
#pragma clang diagnostic ignored "-Wextra-semi"
#pragma clang diagnostic ignored "-Wundef"
#include <asio.h>
#pragma clang diagnostic pop
#include "HanningWindow.h"
#include "SoundDllPro_InputChannel.h"
#include "SoundDllPro_OutputChannel.h"
#include "SoundDllPro_OutputTrack.h"
#include "SoundDllPro_MarkButtons.h"
#include "PerformanceCounter.h"
#include "SoundDllPro_Tools.h"
#include "VSTHost.h"
#include "SoundDllPro_SoundClassBase.h"
#include "SoundDllPro_Debug.h"
//------------------------------------------------------------------------------
#define SoundClass()             SoundDllProMain::Instance()
#ifdef PERFORMANCE_TEST
   #define NUM_TESTCOUNTER       10
#endif
//------------------------------------------------------------------------------
typedef void   (*LPFNEXTSOUNDPROC)(vvf & rvvf);
typedef void   (*LPFNEXTDATANOTIFY)(void);
typedef char*  (*LPFNEXTVSTRING)(void);

#define WM_USER_DRIVERSTATUS    (WM_USER+1)
extern const char* g_lpcszDriverStatus[Asio::RUNNING+1];

float ArrayAbsMax(const std::valarray<float>& rvaf);

//------------------------------------------------------------------------------
/// enumeration of peformance counters
//------------------------------------------------------------------------------
enum {
   PERF_COUNTER_DSP = 0,
   PERF_COUNTER_VSTREC,
   PERF_COUNTER_VSTTRACK,
   PERF_COUNTER_VSTMASTER,
   PERF_COUNTER_ML,
   PERF_COUNTER_REC,
   PERF_COUNTER_VIS,
   PERF_COUNTER_LAST
};
//------------------------------------------------------------------------------

//------------------------------------------------------------------------------
/// enumeration of driver models
//------------------------------------------------------------------------------
enum TSoundDriverModel {
   DRV_TYPE_ASIO = 0,
   DRV_TYPE_WDM
};
//------------------------------------------------------------------------------

//------------------------------------------------------------------------------
/// enumeration of recording threshold modes
//------------------------------------------------------------------------------
enum {
      SDA_THRSHLDMODE_OR = 0,
      SDA_THRSHLDMODE_AND
      };
//------------------------------------------------------------------------------

//------------------------------------------------------------------------------
/// enumeration of (channel= mute types)
//------------------------------------------------------------------------------
enum TChannelType {
	  CT_TRACK = 0,
	  CT_OUTPUT,
	  CT_INPUT
	  };
//------------------------------------------------------------------------------

//------------------------------------------------------------------------------
/// forward declarations
//------------------------------------------------------------------------------
class TAboutBox;
class TVisualForm;
class TTracksForm;
class TMixerForm;
class TPerformanceForm;
class TMPlugin;
//------------------------------------------------------------------------------

//------------------------------------------------------------------------------
/// \class SoundDllProMain. Description see SoundDllPro_Main.cpp
//------------------------------------------------------------------------------
class SoundDllProMain 
{
   public:
      SDPDebug*               m_sdpdDebug;
      void                    AddDebugString(UnicodeString us);
      SoundClassBase*         m_pscSoundClass;

      void                    VSTQueryPlugin(TStringList *psl);
      TVSTHost*               VSTGetHost(AnsiString strType);
      TVSTHostPlugin*         VSTGetPlugin(AnsiString strType, AnsiString strPosition, AnsiString strChannel);
      CPerformanceCounter     m_pcProcess[PERF_COUNTER_LAST];
      #ifdef PERFORMANCE_TEST
      CPerformanceCounter     m_pcTest[NUM_TESTCOUNTER];
      #endif

      TAboutBox*              m_pfrmAbout;
      TTracksForm*            m_pfrmTracks;
      TMixerForm*             m_pfrmMixer;
      TPerformanceForm*       m_pfrmPerformance;
      std::vector<SDPInput*>  m_vInput;
      std::vector<SDPOutput*> m_vOutput;
      std::vector<SDPTrack*>  m_vTracks;
      std::vector<std::vector<int> >    m_vviIOMapping;

      // 'measured' volumes (maxima in channels/tracks)
      std::valarray<float>                m_vfInChannelLevel;
      std::valarray<float>                m_vfOutChannelLevel;
      std::vector<std::valarray<float> >  m_vvfTrackLevel;
      double                  m_dSampleRate;
      CRITICAL_SECTION        m_csProcess;
      CRITICAL_SECTION        m_csBufferDone;
      TMPlugin*               m_pMPlugin;
      bool                    m_bInitialized;
      bool                    m_bRecCompensateLatency;
      bool                    m_bTestCopyOutToIn;
      LPFNEXTDATANOTIFY       m_lpfnExtDataNotify;    /// function pointer for external notifying
      bool                    m_bInitDebug;
      bool                    m_bNoGUI;               /// flag, if tracks and mixer GUI to be disabled
      SoundDllProMain();
      virtual ~SoundDllProMain(void);
      static SoundDllProMain*    Instance();
      static void                SetDriverModel(TSoundDriverModel sdm);
      static TSoundDriverModel   GetDriverModel(void);
      int               About();
      bool              DeviceIsRunning();
      void              Initialize(TStringList *psl);
      void              Exit();
      void              StartMode(int64_t nLength, bool bPause);
      void              Start();
      void              ClearData(void);
      void              DoStop(void);
      void              Stop(void);
      void              Stop(bool bSmooth, bool bWaitForStopDone);
      void              Pause(bool bRamped = true);
      void              Unpause(void);
      bool              Paused(void);
      void              Mute(void);
      void              Unmute(void);
      bool              Muted(void);
      void              SetPosition(uint64_t nPosition);
      bool              IsPlaying();
      bool              IsPlaying(const std::vector<int> & viChannelsOut);
      void              CreateLocalBuffers(unsigned int nInChannels, unsigned int nOutChannels);
      uint64_t          GetSamplePosition();
      uint64_t          GetLoadPosition();
      uint64_t          GetBufferDonePosition();
      uint64_t          GetBufferPlayPosition();
      unsigned int      GetRecDownSampleFactor();
      void              SetRecDownSampleFactor(unsigned int n);
      AnsiString        GetVSTProperties();
      bool              AsyncError(AnsiString &str);
      void              ResetError();
      void              SetOutputGain(const std::vector<float> & vfGain);
      void              SetGain(const std::vector<float> & vfGain);
      void              SetGainDirect(const std::vector<float> & vfGain);
      std::vector<float> GetGain(void);
      void              SetTrackGain(const std::vector<float > & vfTrackGain, unsigned int nRampLength = 0, bool bUpdateSliders = true);
      const std::vector<float >& GetTrackGain();
      void              SetTrackGainDirect(const std::vector<float > & vfTrackGain);
      void              SetRecGain(const std::vector<float> & vfGain, bool bUpdateSliders = true);
      std::vector<float> GetRecGain(void);
      void              SetChannelMute(const std::valarray<bool > &vabMute, TChannelType ct, bool bUpdateMixer = true);
      const std::valarray<bool >& GetChannelMute(TChannelType ct);
      void              SetChannelSolo(const std::valarray<bool > &vabSolo, TChannelType ct, bool bUpdateMixer = true);
      const std::valarray<bool >& GetChannelSolo(TChannelType ct);
      AnsiString        GetChannelNames(TChannelType ct);
      void              GetChannelNames(TChannelType ct, TStringList *psl);
      std::vector<int>  ConvertSoundChannelArgument(  AnsiString     strChannels,
                                                      TChannelType   ct,
                                                      int            nFlags = 0);
      void              ClearTracks(std::vector<int> viTracks);
      void              SetStartThreshold(std::vector<int> viChannels, float fValue, int nMode);
      float             GetStartThreshold(int &nMode);
      void              SetRecordThreshold(std::vector<int> viChannels, float fValue, int nMode);
      float             GetRecordThreshold(int &nMode);
      void              AddButton(HWND hWindowHandle, int64_t nStart, int64_t nLength);
      void              AddButton(RECT &rc, AnsiString strCaption, int64_t nStart, int64_t nLength);
      double            SecondsPerBuffer();
      ASIOError         SetInputMonitor(ASIOInputMonitor& aim);
      bool              BufferedIO();
      void              OnHang();
      unsigned int      GetQueueNumBufs();
      std::vector<unsigned int>   GetTrackInputChannels(unsigned int nTrackIndex);
      AnsiString        GetSupportedSamplerateString();
      AnsiString        GetSoundFormatString();
      void              ShowPerformance();
      void              ResetLevels();
      virtual void         SoundLoadDriver(AnsiString strDriver);
      virtual void         SoundUnloadDriver(void);
      virtual size_t       SoundNumDrivers();
      virtual AnsiString   SoundDriverName(unsigned int iIndex);
      virtual unsigned int SoundCurrentDriverIndex(void);
      virtual double       SoundGetSampleRate(void);
      virtual long         SoundGetLatency(Asio::Direction adDirection);
      virtual long         SoundChannels(Asio::Direction adDirection);
      virtual AnsiString   SoundChannelName(unsigned int iChannelIndex, Asio::Direction adDirection);
      virtual size_t       SoundActiveChannels(Asio::Direction adDirection);
      virtual AnsiString   SoundGetActiveChannelName(unsigned int iChannelIndex, Asio::Direction adDirection);
      virtual void         SoundShowControlPanel(void);
      virtual long         SoundBufsizeSamples();
      int                  ThreadPriority();
      bool                 IsFile2File();
      bool                 IsWaitingForStartThreshold();
      void                 SetClipThreshold(const std::vector<float> & vfThreshold, Asio::Direction adDirection);
      std::vector<float>   GetClipThreshold(Asio::Direction adDirection);
      std::vector<unsigned int> GetClipCount(Asio::Direction adDirection);
      void                 ResetClipCount();
      const std::valarray<unsigned int>& GetTrackClipCount();
      std::valarray<unsigned int>& GetXruns();
      void              SetRampLength(float f);
      void              ApplyRamp(vvf & vvfBuffers, CHanningWindow &hw);
   protected:
      static SoundDllProMain*  sm_psda;
      AnsiString        m_strError;            ///< string used for error messages
      unsigned int      m_nHangsDetected;      ///< counter for OnHang occurrances
      uint64_t          m_nProcessedBuffers;   ///< counter for processed buffers since last 'start'
      uint64_t          m_nPlayedBuffers;      ///< counter for played buffers since last 'start'
      uint64_t          m_nDoneBuffers;        ///< counter for played buffers since last 'start'
      unsigned int      m_nStartTimeout;       ///< maximum time we wait after 'Start' for the first callback
      unsigned int      m_nStopTimeout;        ///< maximum time we wait after 'Stop' for the last callback
      double            m_dSecondsPerBuffer;   ///< Seconds per buffer
      bool              m_bRecFilesDisabled;   ///< global flag for sisabling recfiles
      void              Process(vvf& vvfBuffersIn, vvf& vvfBuffersOut, bool& bIsLast);
      void              DoSignalProcessing(vvf &vvfIn, vvf &vvfOut);
      void              OnBufferDone(vvf& vvfBuffersIn, vvf& vvfBuffersOut, bool& bIsLast);
      void              OnBufferPlay(vvf& vvfBuffer);
      bool              WaitForThreshold(vvf& vvfBuffersIn, bool bStart);
      void              OnStopComplete();
      void              OnError();
      void              OnStateChange(Asio::State asState);
      void              OnXrun(Asio::XrunType xtXrunType);
      void              WaitForRampState(CHanningWindow &hw, WindowState rsValue, float fRampLength = -1.0f);
   private:
      static TSoundDriverModel   sm_sdmDriverModel;
      float                m_fRampLength;          ///< ramp length in milliseconds
      CHanningWindow       m_hwPause;              ///< hanning ramp for pause
      CHanningWindow       m_hwMute;               ///< hanning ramp for mute
      // Gains
      CHanningWindow       m_hwGain;               ///< hanning ramp for gain
      std::vector<float>   m_vfGain;               ///< vector containing gain for each output channel
      std::vector<float>   m_vfPendingGain;        ///< vector containing pending gains for each output channel
      CHanningWindow       m_hwTrackGain;             /// hanning ramp for track volumes
      std::vector<float >  m_vfTrackGain;          /// vector containing volume for each output track
      std::vector<float >  m_vfPendingTrackGain;   /// vector containing pending volume for each output track
      std::vector<float >  m_vfInputGain;          /// vector containing volume for each input channel

      std::valarray<unsigned int> m_vanXrunCounter; ///< counters for xruns
      std::vector<std::vector<unsigned int> >  m_vvnClipCount;     ///< vector containing clipcounts
      std::vector<std::vector<float> > m_vvfClipThreshold;        ///< vector containing normalized clip threshold
      TVSTHost*         m_pVSTHostTrack;  // VST-Host for track plugins
      TVSTHost*         m_pVSTHostMaster; // VST-Host for master plugins
      TVSTHost*         m_pVSTHostFinal; // VST-Host for 'final' master plugins
      TVSTHost*         m_pVSTHostRecord; // VST-Host for masterecording plugins
      unsigned int      m_nHangsForError;      ///< number of hangs that yield an error
      int               m_nThreadPriority;
      bool              m_bFile2File;
      bool              m_bFile2FileDone;
      bool              m_bRealtime;
      HANDLE            m_hOnVisualizeDoneEvent;   /// event for disk writing sync
      AnsiString        m_strFatalError;
      bool              m_bStopOnEmpty;
      bool              m_bPauseOnAutoStop;
      int64_t           m_nRunLength;
      uint64_t          m_nLoadPosition;
      uint64_t          m_nAutoPausePosition;
      uint64_t          m_nBufferPlayPosition;
      uint64_t          m_nBufferDonePosition;
      uint64_t          m_nLastCursorPosition;
      int               m_nBufSizeBest;
      unsigned int      m_nRecDownSampleFactor; ///< factor for downsampling recording data
      vvf               m_vvfDownSample;        ///< internal buffer for downsampling
      LPFNEXTSOUNDPROC  m_lpfnExtPreVSTProc;    /// function pointer for external processing BEFORE VST
      LPFNEXTSOUNDPROC  m_lpfnExtPostVSTProc;   /// function pointer for external processing AFTER VST and Gain
      LPFNEXTSOUNDPROC  m_lpfnExtPreRecVSTProc;    /// function pointer for external processing of record data BEFORE VST
      LPFNEXTSOUNDPROC  m_lpfnExtPostRecVSTProc;  /// function pointer for external processing of record data AFTER VST
      LPFNEXTSOUNDPROC  m_lpfnExtDoneProc;      /// function pointer for external done processing
      void CalculateAppliedMuteStatus(TChannelType ct);
      std::vector<std::valarray<bool> >   m_vvabChannelMute; /// vector of valarrays containing mute status of channels
      std::vector<std::valarray<bool> >   m_vvabChannelSolo; /// vector of valarrays containing solo status of channels
      std::vector<std::valarray<bool> >   m_vvabAppliedChannelMute;  /// vector of valarrays containing applied mute
                                                                     /// status of channels calculated from m_vvabChannelMute
                                                                     /// and m_vvabChannelSolo
      std::valarray<unsigned int>  m_vanTrackClipCount;     ///< calarray containing track clipcounts

      float             m_fStartThreshold;
      std::vector<int>  m_viStartThresholdChannels;
      int               m_nStartThresholdMode;
      float             m_fRecordThreshold;
      std::vector<int>  m_viRecordThresholdChannels;
      int               m_nThresholdMode;
      SDPMarkButtons    m_mbMarkButtons;
      bool              m_bAutoCleanup;
      unsigned int      m_iProcQueueBuffers;
      int               m_nNumTrackLevels;
      // variables for file2file operation
      vvf               m_vvfBuffersOutFile2File;
      vvf               m_vvfBuffersInFile2File;
      int               m_nBufsizeFile2File;
      int               m_nOutChannelsFile2File;

      std::valarray<float> m_vafTrackGainRamp;     /// buffer for calculating ramp values for track gains
      std::vector<std::valarray<float> >  m_vvafTrackBuffers;
      void              InitializeMPlugin(TStringList *psl);
      void              DoButtonMarking(int64_t nSamplePosition);
};
//------------------------------------------------------------------------------
#endif
