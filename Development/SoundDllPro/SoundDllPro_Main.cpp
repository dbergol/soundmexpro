//------------------------------------------------------------------------------
/// \file SoundDllPro_Main.cpp
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
#pragma hdrstop
#include <math.h>
#include <stdio.h>
#include "SoundDllPro_Main.h"
#include "MPlugin.h"
#include "formTracks.h"
#include "formAbout.h"
#include "formMixer.h"
#include "formPerformance.h"
#include "SoundDllPro_RecFile.h"
#include "SoundDllPro_SoundClassAsio.h"
#include "SoundDllPro_WaveReader_libsndfile.h"
#ifdef NOMMDEVICE
   #include "SoundDllPro_SoundClassWdm.h"
#else
   #include "SoundDllPro_SoundClassMMDevice.h"
#endif
//------------------------------------------------------------------------------
#pragma warn -aus
#pragma warn -use

//------------------------------------------------------------------------------
/// human readable strings for showing driver status
//------------------------------------------------------------------------------

const char* g_lpcszDriverStatus[RUNNING+1] =
   {
   "driver not initialized",
   "driver loaded",
   "driver initialized",
   "device stopped",
   "device running"
   };
//------------------------------------------------------------------------------

//------------------------------------------------------------------------------
/// Pointer to global instance (set in constructor).
//------------------------------------------------------------------------------
SoundDllProMain*  SoundDllProMain::sm_psda = NULL;
/// static driver model variable
TSoundDriverModel SoundDllProMain::sm_sdmDriverModel = DRV_TYPE_ASIO;
//------------------------------------------------------------------------------

//------------------------------------------------------------------------------
/// array with sample rates to test if they are supported
//------------------------------------------------------------------------------
static const long anTestRates[] = {
    8000,
    11025,
    16000,
    22050,
    32000,
    44100,
    48000,
    88200,
    96000,
    176400,
    192000,
    352800,
    384000
};
//------------------------------------------------------------------------------

//------------------------------------------------------------------------------
/// returns absolute maximum of a float valarray
//------------------------------------------------------------------------------
float ArrayAbsMax(const std::valarray<float>& rvaf)
{
   float fMin = (float)fabs(rvaf.min());
   float fMax = (float)rvaf.max();
   if (fMax > fMin)
      return fMax;
   else
      return fMin;
}
//------------------------------------------------------------------------------

//------------------------------------------------------------------------------
/// constructor. Initializes members sets SoundDllProMain::sm_psda
//------------------------------------------------------------------------------
SoundDllProMain::SoundDllProMain()
   :  m_sdpdDebug(NULL),
      m_pscSoundClass(NULL),
      m_pfrmAbout(NULL),
      m_pfrmTracks(NULL),
      m_pfrmMixer(NULL),
      m_pfrmPerformance(NULL),
      m_pMPlugin(NULL),
      m_bInitialized(false),
      m_bRecCompensateLatency(false),
      m_bTestCopyOutToIn(false),
      m_lpfnExtDataNotify(NULL),
      m_bInitDebug(false),
      m_nHangsDetected(0),
      m_nProcessedBuffers(0),
      m_nPlayedBuffers(0),
      m_nDoneBuffers(0),
      m_nStartTimeout(6000),
      m_nStopTimeout(1000),
      m_dSecondsPerBuffer(1.0),
      m_bRecFilesDisabled(false),
      m_fRampLength(1.0f),
      m_pVSTHostTrack(NULL),
      m_pVSTHostMaster(NULL),
      m_pVSTHostFinal(NULL),
      m_pVSTHostRecord(NULL),
      m_nHangsForError(1),
      m_nThreadPriority(2), // corresponds to tpHighest!
      m_bFile2File(false),
      m_bFile2FileDone(false),
      m_bRealtime(false),
      m_bStopOnEmpty(true),
      m_bPauseOnAutoStop(false),
      m_nRunLength(-1),
      m_nLoadPosition(0),
      m_nAutoPausePosition(0),
      m_nBufferPlayPosition(0),
      m_nBufferDonePosition(0),
      m_nLastCursorPosition(0),
      m_nRecDownSampleFactor(1),
      m_lpfnExtPreVSTProc(NULL),
      m_lpfnExtPostVSTProc(NULL),
      m_lpfnExtPreRecVSTProc(NULL),
      m_lpfnExtPostRecVSTProc(NULL),
      m_lpfnExtDoneProc(NULL),
      m_fStartThreshold(0.0f),
      m_nStartThresholdMode(SDA_THRSHLDMODE_OR),
      m_fRecordThreshold(0.0f),
      m_nThresholdMode(SDA_THRSHLDMODE_OR),
      m_bAutoCleanup(false),
      m_nBufsizeFile2File(1024),
      m_nOutChannelsFile2File(2)

{
   OutputDebugString(__FUNC__);
   if (!IsAudioSpike())
      SetStyle();
      
   randomize();
   try
      {
      m_vanXrunCounter.resize(3);
      m_vanXrunCounter = 0;
      m_vvnClipCount.resize(2);
      m_vvfClipThreshold.resize(2);
      m_hOnVisualizeDoneEvent = CreateEvent(NULL, TRUE, FALSE, NULL);
      InitializeCriticalSection(&m_csProcess);
      InitializeCriticalSection(&m_csBufferDone);
      m_pfrmAbout  = new TAboutBox(NULL);
      m_pfrmTracks = new TTracksForm(NULL);
      m_pfrmMixer  = new TMixerForm(NULL);
      if (GetProfileStringDefault("Debug", 0) == 1)
         {
         m_sdpdDebug = new SDPDebug();
         m_sdpdDebug->Add("Debug initialized");
         }
      if (GetProfileStringDefault("ShowPerformance", 0) == 1)
         {
         m_pfrmPerformance = new TPerformanceForm(NULL);
         m_pfrmPerformance->Show();
         }
      // create sound class
      // NOTE: later we have to create particluar class here depending on DriverModel!
      if (GetDriverModel() == DRV_TYPE_WDM)
         #ifdef NOMMDEVICE
         m_pscSoundClass = (SoundClassBase*)(new SoundClassWdm());
         #else
         m_pscSoundClass = (SoundClassBase*)(new SoundClassMMDevice());
         #endif
      else
         m_pscSoundClass = (SoundClassBase*)(new SoundClassAsio());
      // attach all callbacks
      m_pscSoundClass->SetOnHang(OnHang);
      m_pscSoundClass->SetOnStopComplete(OnStopComplete);
      m_pscSoundClass->SetOnError(OnError);
      m_pscSoundClass->SetOnStateChange(OnStateChange);
      m_pscSoundClass->SetOnXrun(OnXrun);
      m_pscSoundClass->SetOnBufferPlay(OnBufferPlay);
      m_pscSoundClass->SetOnBufferDone(OnBufferDone);
      m_pscSoundClass->SetOnProcess(Process);
      if (!sm_psda)
         sm_psda = this;
      }
   catch (...)
      {
      DeleteCriticalSection(&m_csProcess);
      DeleteCriticalSection(&m_csBufferDone);
      CloseHandle(m_hOnVisualizeDoneEvent);
      TRYDELETENULL(m_pfrmTracks);
      TRYDELETENULL(m_pfrmMixer);
      TRYDELETENULL(m_pfrmAbout);
      TRYDELETENULL(m_pfrmPerformance);
      TRYDELETENULL(m_pscSoundClass);
      if (this == sm_psda)
         sm_psda = NULL;
      throw;
      }
}
//------------------------------------------------------------------------------

//------------------------------------------------------------------------------
/// destructor. Stops output and does cleanup and resets static member
/// SoundDllProMain::sm_psda
//------------------------------------------------------------------------------
SoundDllProMain::~SoundDllProMain()
{
   try{Stop(false, false);}catch(...){}
   try{Exit();}catch(...){}
   TRYDELETENULL(m_pfrmTracks);
   TRYDELETENULL(m_pfrmAbout);
   TRYDELETENULL(m_pfrmPerformance);
   TRYDELETENULL(m_pfrmMixer);
   TRYDELETENULL(m_pMPlugin);
   DeleteCriticalSection(&m_csProcess);
   DeleteCriticalSection(&m_csBufferDone);
   TRYDELETENULL(m_pVSTHostTrack);
   TRYDELETENULL(m_pVSTHostMaster);
   TRYDELETENULL(m_pVSTHostFinal);
   TRYDELETENULL(m_pVSTHostRecord);
   CloseHandle(m_hOnVisualizeDoneEvent);
   TRYDELETENULL(m_pscSoundClass);
   if (m_sdpdDebug)
      {
      try
         {
         m_sdpdDebug->m_psl->SaveToFile(g_strBinPath + "SDP_DEBUG.TXT");
         }
      catch (...)
         {
         }
      }
   TRYDELETENULL(m_sdpdDebug);
   if (this == sm_psda)
      sm_psda = NULL;
}
//------------------------------------------------------------------------------

//------------------------------------------------------------------------------
/// returns static member SoundDllProMain::sm_psda
//------------------------------------------------------------------------------
SoundDllProMain*  SoundDllProMain::Instance()
{
   return sm_psda;
}
//------------------------------------------------------------------------------

//------------------------------------------------------------------------------
/// adds a debug string to member debug class
//------------------------------------------------------------------------------
void SoundDllProMain::AddDebugString(UnicodeString us)
{
   if (m_sdpdDebug)
      m_sdpdDebug->Add(us);
}
//------------------------------------------------------------------------------

//------------------------------------------------------------------------------
/// Sets static driver model variable
//------------------------------------------------------------------------------
void SoundDllProMain::SetDriverModel(TSoundDriverModel sdm)
{
   if (!!Instance())
      throw Exception("cannot set driver model in initialized module");
   SoundDllProMain::sm_sdmDriverModel = sdm;
}
//------------------------------------------------------------------------------

//------------------------------------------------------------------------------
/// returns value of static driver model variable
//------------------------------------------------------------------------------
TSoundDriverModel SoundDllProMain::GetDriverModel(void)
{
   return SoundDllProMain::sm_sdmDriverModel;
}
//------------------------------------------------------------------------------

//------------------------------------------------------------------------------
/// Main initialization. Reads from passed TStringList like from an INI-file
/// and initializes all members. Main features:
/// - Checks license
/// - Loads driver by name or index
/// - Inits sound class
/// - calls CreateLocalBuffers 
/// - Creates virtual tracks
/// - Initialises MATLAB script plugin (calls InitializeMPlugin)
/// - Initialises VST-plugin-Hosts
//------------------------------------------------------------------------------
void SoundDllProMain::Initialize(TStringList *psl)
{
   int nNumProcBufs = 10;
   if (m_bInitDebug)
      WriteDebugString("Initialize 1", g_strBinPath + "init.log");
   if (!psl)
      throw Exception("invalid string list passed to Initialize");
   TStringList *pslLic = new TStringList();
   try
      {
      try
         {
         m_bInitDebug = GetProfileStringDefault("InitDebug", "0") == "1";
         if (m_bInitDebug)
            WriteDebugString("Initialize 2", g_strBinPath + "init.log");
         pslLic->Values[SOUNDDLLPRO_PAR_MATLABEXE] = LowerCase(psl->Values[SOUNDDLLPRO_PAR_MATLABEXE]);
         pslLic->Values[SOUNDDLLPRO_PAR_FORCELIC] = psl->Values[SOUNDDLLPRO_PAR_FORCELIC];
         pslLic->Values["Ed"] = "4";
         pslLic->Values["Type"] = "VST+";
         pslLic->Values[SOUNDDLLPRO_STR_Version] = VString();

         unsigned int nWaveReadBufSize = (unsigned int)GetInt(psl, SOUNDDLLPRO_PAR_FILEREADBUFSIZE, WAVREAD_DEFAULTBUFSIZE, VAL_POS);
         if (nWaveReadBufSize < WAVREAD_MINBUFSIZE)
            nWaveReadBufSize = WAVREAD_MINBUFSIZE;
         SDPWaveReader::sm_nWaveReaderBufSize = nWaveReadBufSize;

         int nPriority = HIGH_PRIORITY_CLASS;
         if (!_wcsicmp(psl->Values[SOUNDDLLPRO_PAR_PRIORITY].c_str(), L"normal"))
            nPriority = NORMAL_PRIORITY_CLASS;
         SetPriorityClass(GetCurrentProcess(), (unsigned int)nPriority);

         m_bRecCompensateLatency = GetInt(psl, SOUNDDLLPRO_PAR_RECCOMPLATENCY, 2, VAL_ALL) == 1;
         m_bFile2File = GetInt(psl, SOUNDDLLPRO_PAR_FILE2FILE, 2, VAL_ALL) == 1;
         unsigned int nBufSize = 0;
         unsigned int nOutChannels = 0;
         unsigned int nInChannels = 0;
         // ----------------file2file-operation requested! -----------------------
         if (m_bFile2File)
            {
            // output here is pure number of channels
            nOutChannels = (unsigned int)GetInt(psl, SOUNDDLLPRO_PAR_OUTPUT, 2, VAL_POS);
            m_nOutChannelsFile2File = (int)nOutChannels;
            // optional buffersize to use
            nBufSize = (unsigned int)GetInt(psl, SOUNDDLLPRO_PAR_F2FBUFSIZE, SoundBufsizeSamples(), VAL_POS);
            m_nBufsizeFile2File = (int)nBufSize;
            m_vvfBuffersOutFile2File.resize(nOutChannels);
            for (unsigned int i = 0; i < nOutChannels; i++)
               m_vvfBuffersOutFile2File[i].resize((unsigned int)SoundBufsizeSamples());
            m_vfOutChannelLevel.resize(nOutChannels);
            // sample rate: needed for writing wav files (and maybe for user defined plugins)
            m_dSampleRate = (double)GetInt(psl, SOUNDDLLPRO_PAR_SAMPLERATE, 44100, VAL_POS);
            // create output channels (for 'regular' action created in CreateBuffers) 
            for (unsigned int nChannelIndex = 0; nChannelIndex < nOutChannels; nChannelIndex++)
              {
              m_vOutput.push_back(new SDPOutput(nChannelIndex, SoundGetSampleRate()));
              m_vOutput.back()->DebugFileName("f2f_" + IntToStr((int)nChannelIndex) + ".wav");
              m_vOutput.back()->DebugSave(1);
              }
            CreateLocalBuffers(0, nOutChannels);
            }
         // ------------------regular action (with Sound-device)-----------------
         else
            {
            if (!SoundNumDrivers())
               throw Exception("no sound drivers found on the system");
            if (m_bInitDebug)
               WriteDebugString("Initialize 3", g_strBinPath + "init.log");
            // load driver (default is driver with index 0)
            AnsiString str = psl->Values[SOUNDDLLPRO_PAR_DRIVER];
            if (str.IsEmpty())
               str = "0"; // default
            SoundLoadDriver(str);
            m_pscSoundClass->SoundSetSaveProcessedCaptureData((GetInt(psl, SOUNDDLLPRO_PAR_RECPROCDATA, 0, VAL_POS_OR_ZERO) == 1));
            if (m_bInitDebug)
               WriteDebugString("Initialize 4", g_strBinPath + "init.log");
            // check if any channels at all (some drivers load successfully, even if device
            // is not present at all!
            if (  !SoundChannels(Asio::OUTPUT)
               && !SoundChannels(Asio::INPUT)
               )
               throw Exception("No input and no output channels found for driver (device not present?)");
            // ... and number of software buffers
            nNumProcBufs = (int)GetInt(psl, SOUNDDLLPRO_PAR_NUMBUFS, 10, VAL_POS_OR_ZERO);
            // check optional values:
            // auto cleanup ...
            m_bAutoCleanup = (GetInt(psl, SOUNDDLLPRO_PAR_AUTOCLEARDATA, 0, VAL_POS_OR_ZERO) == 1);
            if (m_bInitDebug)
               WriteDebugString("Initialize 5.1", g_strBinPath + "init.log");
            // ... samplerate ...
            m_dSampleRate = (double)GetInt(psl, SOUNDDLLPRO_PAR_SAMPLERATE, 44100, VAL_POS);
            if (!m_pscSoundClass->SoundCanSampleRate(m_dSampleRate))
               throw Exception("samplerate " + FloatToStr((long double)m_dSampleRate) + " not supported by device");
            if (m_bInitDebug)
               WriteDebugString("Initialize 5.2", g_strBinPath + "init.log");
            m_pscSoundClass->SoundSetSampleRate(m_dSampleRate);
            if (m_bInitDebug)
               WriteDebugString("Initialize 5.3", g_strBinPath + "init.log");
            #pragma clang diagnostic push
            #pragma clang diagnostic ignored "-Wfloat-equal"
            if (m_dSampleRate != SoundGetSampleRate())
               throw Exception("error setting samplerate to " + FloatToStr((long double)m_dSampleRate) + "Hz. Device supports samplerate, but switching failed. Maybe samplerate is blocked by driver settings.");
            #pragma clang diagnostic pop
            if (m_bInitDebug)
               WriteDebugString("Initialize 5.4", g_strBinPath + "init.log");

            m_pscSoundClass->SoundInit(psl);

            if (m_bInitDebug)
               WriteDebugString("Initialize 5.5", g_strBinPath + "init.log");

            m_iProcQueueBuffers = (unsigned int)nNumProcBufs;
            SetRecDownSampleFactor((unsigned int)GetInt(psl, SOUNDDLLPRO_PAR_RECDOWNSAMPLEFACTOR, 1, VAL_POS));
            m_bRecFilesDisabled = GetInt(psl, SOUNDDLLPRO_PAR_RECFILEDISABLE, 0) == 1;
            m_bTestCopyOutToIn  = GetInt(psl, SOUNDDLLPRO_PAR_COPYOUT2IN, 0) == 1;
            nOutChannels = (unsigned int)SoundActiveChannels(Asio::OUTPUT);
            nInChannels  = (unsigned int)SoundActiveChannels(Asio::INPUT);
            CreateLocalBuffers(nInChannels, nOutChannels);

            nBufSize = (unsigned int)m_pscSoundClass->SoundBufsizeCurrent();
            m_bRealtime = (nNumProcBufs == 0) && (GetDriverModel() == DRV_TYPE_ASIO);
            m_nBufSizeBest = m_pscSoundClass->SoundBufsizeBest();

            m_vfOutChannelLevel.resize(nOutChannels);
            m_vfInChannelLevel.resize(nInChannels);
            m_vfInputGain.resize(nInChannels, 1.0f);
            } // regular mode
         // --------------- end of file2file/soundcard-mode differences---------
         ResetLevels();
         if (m_bInitDebug)
            WriteDebugString("Initialize 6", g_strBinPath + "init.log");
         // create ramp buffer
         m_vafTrackGainRamp.resize(nBufSize);
         // create tracks
         unsigned int nTracks = 0;
         if (nOutChannels > 0)
            nTracks = (unsigned int)GetInt(psl, SOUNDDLLPRO_PAR_TRACK, nOutChannels, VAL_POS_OR_ZERO);
         if (nTracks < nOutChannels)
            throw Exception("number of tracks must be >= number of output channels");

         // create buffers for track calculations in ::Process
         m_vvafTrackBuffers.resize(nTracks);
         for (unsigned int i = 0; i < nTracks; i++)
            {
            m_vTracks.push_back(new SDPTrack(i, i % nOutChannels, m_bAutoCleanup));
            // create buffers for track calculations in ::Process
            m_vvafTrackBuffers[i].resize(nBufSize);
            m_vfTrackGain.push_back(1.0f);
            m_vfPendingTrackGain.push_back(1.0f);
            }
         m_vanTrackClipCount.resize(nTracks);
         m_vanTrackClipCount = 0;
         // initialize mute, solo and applied mute vectors of valarrays
         m_vvabChannelMute.resize(3);
         m_vvabChannelSolo.resize(3);
         m_vvabAppliedChannelMute.resize(3);
         for (unsigned int n = CT_TRACK; n <= CT_INPUT; n++)
            {
            unsigned int nSize = nTracks;
            if (n == CT_OUTPUT)
               nSize = (unsigned int)SoundActiveChannels(Asio::OUTPUT);
            else if (n == CT_INPUT)
               nSize = (unsigned int)SoundActiveChannels(Asio::INPUT);
            m_vvabChannelMute[n].resize(nSize);
            m_vvabChannelSolo[n].resize(nSize);
            m_vvabAppliedChannelMute[n].resize(nSize);
            m_vvabChannelMute[n] = false;
            m_vvabChannelSolo[n] = false;
            m_vvabAppliedChannelMute[n] = false;
            }
         if (m_bInitDebug)
            WriteDebugString("Initialize 7.1", g_strBinPath + "init.log");

         // we need multiple buffers for storing track volumes, because they are stored
         // in Process (software buffered), but should be shown at 'audible position'
         // (delayed by software buffering!). But we need 1 level at least!
         m_nNumTrackLevels = nNumProcBufs;
         if (m_nNumTrackLevels == 0)
            m_nNumTrackLevels = 1;
         m_vvfTrackLevel.resize((unsigned int)m_nNumTrackLevels);
         for (unsigned int i = 0; i < (unsigned int)m_nNumTrackLevels; i++)
            m_vvfTrackLevel[i].resize((unsigned int)nTracks);
         // afterwards initialize mixer (tracks must have been created prior to this)
         m_pfrmMixer->Init();
         if (m_bInitDebug)
            WriteDebugString("Initialize 7.2", g_strBinPath + "init.log");
         // set ramp length: default is 10 ms
         float fRampLength = 1000.0f * GetFloat(psl, SOUNDDLLPRO_PAR_RAMPLEN, ((float)SoundGetSampleRate() / 100.0f), VAL_POS_OR_ZERO) / (float)SoundGetSampleRate();
         SetRampLength(fRampLength);
         if (m_bInitDebug)
            WriteDebugString("Initialize 7.3", g_strBinPath + "init.log");

         m_nStartTimeout   = (unsigned int)GetInt(psl, SOUNDDLLPRO_PAR_STARTTIMEOUT, 6000, VAL_POS);
         m_nStopTimeout    = (unsigned int)GetInt(psl, SOUNDDLLPRO_PAR_STOPTIMEOUT, 1000, VAL_POS);
         // get thread priority. NOTE: must be read BEFORE VST init: we need it for MPlugin as
         // well !!
         // NOTE: must be between tpNormal and tpTimeCritical, i.e.
         //  3 <= n <= 6
         // we translate these numbers to 0..3
         int nThreadPriority = (int)GetInt(psl, SOUNDDLLPRO_PAR_VSTTP, 2); // default is tpHighest
         if (nThreadPriority < 0 || nThreadPriority > 3)
            throw Exception("invalid field in 'vstthreadpriority': must be between 0 and 3");
         m_nThreadPriority = nThreadPriority;
         if (m_bInitDebug)
            WriteDebugString("Initialize 8", g_strBinPath + "init.log");
         InitializeMPlugin(psl);
         if (m_bInitDebug)
            WriteDebugString("Initialize 9", g_strBinPath + "init.log");

         // get threading type
         TThreadingType tt = TT_PLUGIN;
         int n = (int)GetInt(psl, SOUNDDLLPRO_PAR_VSTMT, 1);
         if (n == 0)
            tt = TT_SINGLE;
         else if (n == 2)
            tt = TT_FIXNUM;
         // NOTE: Changed on 28.01.2011: formely used SoundBufsizeCurrent(),
         // but should be identical!!
         if (nTracks)
            {
            m_pVSTHostTrack   = new TVSTHost("VST-Trackhost", this);
            m_pVSTHostTrack->Init((unsigned int)nTracks, (unsigned int)nBufSize, tt, nThreadPriority);
            }
         if (nOutChannels)
            {
            m_pVSTHostMaster  = new TVSTHost("VST-Masterhost", this);
            m_pVSTHostMaster->Init((unsigned int)nOutChannels, (unsigned int)nBufSize, tt, nThreadPriority);
            m_pVSTHostFinal  = new TVSTHost("VST-Finalhost", this);
            m_pVSTHostFinal->Init((unsigned int)nOutChannels, (unsigned int)nBufSize, tt, nThreadPriority);
            }
         if (nInChannels)
            {
            m_pVSTHostRecord  = new TVSTHost("VST-Recordhost", this);
            m_pVSTHostRecord->Init(nInChannels, nBufSize, tt, nThreadPriority);
            }

         // finally attach external processing callbacks
         m_lpfnExtPreVSTProc        = (LPFNEXTSOUNDPROC)((NativeInt)GetInt(psl, SOUNDDLLPRO_PAR_EXTPREVSTPROC, 0, VAL_POS_OR_ZERO));
         m_lpfnExtPostVSTProc       = (LPFNEXTSOUNDPROC)((NativeInt)GetInt(psl, SOUNDDLLPRO_PAR_EXTPOSTVSTPROC, 0, VAL_POS_OR_ZERO));
         m_lpfnExtPreRecVSTProc     = (LPFNEXTSOUNDPROC)((NativeInt)GetInt(psl, SOUNDDLLPRO_PAR_EXTRECPREVSTPROC, 0, VAL_POS_OR_ZERO));
         m_lpfnExtPostRecVSTProc    = (LPFNEXTSOUNDPROC)((NativeInt)GetInt(psl, SOUNDDLLPRO_PAR_EXTRECPOSTVSTPROC, 0, VAL_POS_OR_ZERO));
         m_lpfnExtDoneProc          = (LPFNEXTSOUNDPROC)((NativeInt)GetInt(psl, SOUNDDLLPRO_PAR_EXTDONEPROC, 0, VAL_POS_OR_ZERO));

         m_lpfnExtDataNotify  = (LPFNEXTDATANOTIFY)((NativeInt)GetInt(psl, SOUNDDLLPRO_PAR_EXTDATANOTIFY, 0, VAL_POS_OR_ZERO));
         if (!!m_lpfnExtDataNotify)
            {
            unsigned int nDataNotifyTrack = (unsigned int)GetInt(psl, SOUNDDLLPRO_PAR_DATANOTIFYTRACK, 0, VAL_POS_OR_ZERO);
            if (!!m_lpfnExtDataNotify && nDataNotifyTrack < m_vTracks.size())
               m_vTracks[nDataNotifyTrack]->m_bNotify = true;
            }

         if (m_bInitDebug)
            WriteDebugString("Initialize 10", g_strBinPath + "init.log");
         m_bInitialized = true;
         }
      __finally
         {
         psl->Text = pslLic->Text;
         TRYDELETENULL(pslLic);
         }
      }
   catch (...)
      {
      if (m_bInitDebug)
         WriteDebugString("Initialize EXCEPTION", g_strBinPath + "init.log");
      Exit();
      TRYDELETENULL(m_pMPlugin);
      TRYDELETENULL(m_pVSTHostTrack);
      TRYDELETENULL(m_pVSTHostMaster);
      TRYDELETENULL(m_pVSTHostFinal);
      TRYDELETENULL(m_pVSTHostRecord);
      throw;
      }
}
//------------------------------------------------------------------------------

//------------------------------------------------------------------------------
/// Shows an about box
//------------------------------------------------------------------------------
int SoundDllProMain::About()
{
   return m_pfrmAbout->About();
}
//------------------------------------------------------------------------------

//------------------------------------------------------------------------------
/// Initialises MATLAB script plugin (if requested). 
//------------------------------------------------------------------------------
void SoundDllProMain::InitializeMPlugin(TStringList *psl)
{
   try
      {
      TRYDELETENULL(m_pMPlugin);
      // plugin requested at all (i.e. name of process command)?
      AnsiString strProc = Trim(psl->Values[SOUNDDLLPRO_PAR_PLUGIN_PROC]);
      if (strProc.IsEmpty())
         return;
      // check, if PLUGINEXE was passed
      AnsiString strMatlab = LowerCase(psl->Values[SOUNDDLLPRO_PAR_PLUGIN_EXE]);
      if (strMatlab.IsEmpty())
         {
         // otherwise check if MATLAB-exe was passed
         strMatlab = LowerCase(psl->Values[SOUNDDLLPRO_PAR_MATLABEXE]);
         if (strMatlab.IsEmpty())
               {
               char c[2*MAX_PATH];
               ZeroMemory(c, 2*MAX_PATH);
               if (!GetModuleFileName(GetModuleHandle(NULL), c, 2*MAX_PATH-1))
                  throw Exception("error retrieving MATLAB module name");
               strMatlab = LowerCase(c);
               }
         // change on 25.2.2010: if 'matlab.exe' AND octave is not present at all in
         // module name, then 'somebody else' (e.g. SMPIPC.EXE) was the loader. In
         // that case the only chance to call matlab is to use 'matlab.exe' without
         // any path and 'hope' that the path is contained in DOS search path.
         if (strMatlab.Pos("matlab.exe") == 0 && strMatlab.Pos("octave") == 0)
            strMatlab = "matlab.exe";
         }
      MPluginStruct     mps;
      mps.strMatlab     = strMatlab;
      mps.strPluginPath = psl->Values[SOUNDDLLPRO_PAR_PLUGINPATH];
      mps.strInitCmd    = psl->Values[SOUNDDLLPRO_PAR_PLUGIN_START];
      mps.strProcCmd    = strProc;
      mps.nInChannels   = (unsigned int)SoundActiveChannels(Asio::INPUT);
      mps.nOutChannels  = (unsigned int)SoundActiveChannels(Asio::OUTPUT);
      mps.nSamples      = (unsigned int)SoundBufsizeSamples();
      mps.nUserDataSize = (unsigned int)GetInt(psl, SOUNDDLLPRO_PAR_PLUGIN_USERDATASIZE, 100, VAL_POS);
      mps.bShowProcess  = (psl->Values[SOUNDDLLPRO_PAR_PLUGIN_SHOW] == "1");
      mps.bTerminateProcess = (psl->Values[SOUNDDLLPRO_PAR_PLUGIN_KILL] != "0");
      mps.bJVM          = (psl->Values[SOUNDDLLPRO_PAR_PLUGIN_FORCEJVM] == "1");
      mps.nStartTimeout = (unsigned int)GetInt(psl, SOUNDDLLPRO_PAR_PLUGIN_TIMEOUT, 10000, VAL_POS);

      m_pMPlugin = new TMPlugin(mps);
      m_pMPlugin->Init(m_nThreadPriority == 3); // if it's 3 then it's realtime priority!!
      }
   catch (Exception &)
      {
      TRYDELETENULL(m_pMPlugin);
      throw;
      }
}
//------------------------------------------------------------------------------

//------------------------------------------------------------------------------
/// Creates local buffers (sizing of vectors for gain ...)
//------------------------------------------------------------------------------
void SoundDllProMain::CreateLocalBuffers(unsigned int nInChannels, unsigned int nOutChannels)
{
   m_vvfClipThreshold[Asio::INPUT].clear();
   m_vvfClipThreshold[Asio::OUTPUT].clear();

   m_vfGain.clear();
   m_vfPendingGain.clear();
   // add number of allocated channels elements to vectors
   m_vfGain.resize(nOutChannels);
   m_vfPendingGain.resize(nOutChannels);
   m_vvnClipCount[Asio::OUTPUT].resize(nOutChannels);
   m_vvfClipThreshold[Asio::OUTPUT].resize(nOutChannels);
   // set default values:
   // 1 for Gain (linear) (real and pending)
   // 0 for ClipCount
   unsigned int i;
   for (i = 0; i < nOutChannels; i++)
      {
      m_vfGain[i]                = 1.0f;
      m_vfPendingGain[i]         = 1.0f;
      m_vvnClipCount[Asio::OUTPUT][i]  = 0;
      m_vvfClipThreshold[Asio::OUTPUT][i] = 1.0f;
      }
   m_vvnClipCount[Asio::INPUT].resize(nInChannels);
   m_vvfClipThreshold[Asio::INPUT].resize(nInChannels);
   for (i = 0; i < nInChannels; i++)
      {
      m_vvnClipCount[Asio::INPUT][i]  = 0;
      m_vvfClipThreshold[Asio::INPUT][i] = 1.0f;
      }
   m_hwGain.SetState(WINDOWSTATE_UP);
   // calculate Seconds available per buffer
   m_dSecondsPerBuffer = (double)SoundBufsizeSamples() / SoundGetSampleRate();
   m_vvfDownSample.resize(nInChannels);
   unsigned int nChannelIndex;
   for (nChannelIndex = 0; nChannelIndex < nInChannels; nChannelIndex++)
     {
     m_vInput.push_back(new SDPInput(nChannelIndex, m_bRecFilesDisabled, m_nRecDownSampleFactor));
     m_vvfDownSample[nChannelIndex].resize((unsigned long)m_pscSoundClass->SoundBufsizeCurrent() / m_nRecDownSampleFactor);
     }
   for (nChannelIndex = 0; nChannelIndex < nOutChannels; nChannelIndex++)
     {
     m_vOutput.push_back(new SDPOutput(nChannelIndex, SoundGetSampleRate()));
     }
   // create mapping vector of valarrays. It contains a mapping for each input, i.e.
   // to which output channels the input data should be added. Each input channel
   // ma be added to multiple output channels, therefore m_vviIOMapping is a vector
   // of valarrays
   m_vviIOMapping.resize(nInChannels);

   // give GUI a chance to update
   Application->ProcessMessages();
}
//------------------------------------------------------------------------------

//------------------------------------------------------------------------------
/// Does local cleanup: hides visualisations, deletes instances of SDPInput and
/// SDPOutput for all channels, cleans uo tracks and track buffers.
//------------------------------------------------------------------------------
void SoundDllProMain::Exit()
{
   EnterCriticalSection(&m_csBufferDone);
   try
      {
      if (m_pfrmMixer)
         {
         m_pfrmMixer->Hide();
         m_pfrmMixer->Exit();
         }
      unsigned int nChannelIndex;
      for (nChannelIndex = 0; nChannelIndex < m_vOutput.size(); nChannelIndex++)
         {
         TRYDELETENULL(m_vOutput[nChannelIndex]);
         }
      m_vOutput.clear();
      for (nChannelIndex = 0; nChannelIndex < m_vInput.size(); nChannelIndex++)
         {
         TRYDELETENULL(m_vInput[nChannelIndex]);
         }
      m_vInput.clear();
      for (nChannelIndex = 0; nChannelIndex < m_vTracks.size(); nChannelIndex++)
         {
         TRYDELETENULL(m_vTracks[nChannelIndex]);
         }
      m_vTracks.clear();
      m_vvafTrackBuffers.clear();
      }
   __finally
      {
      LeaveCriticalSection(&m_csBufferDone);
      }
}
//------------------------------------------------------------------------------

//------------------------------------------------------------------------------
/// Starts SDPInput and SDPOutput for all channels, VSThosts, starts
/// all tracks,
// In regular mode calls SoundStart and finnally re-checks samplerate after start
// In file2file mode calls processing callbacks until operation is done
//------------------------------------------------------------------------------
void SoundDllProMain::Start()
{
   m_nHangsForError = 1;
   if (!DeviceIsRunning())
      {
      ResetError();
      for (int i = 0; i < PERF_COUNTER_LAST; i++)
         m_pcProcess[i].Reset();
      m_vanTrackClipCount  = 0;
//      m_nLoadPosition      = 0;
//      m_nBufferDonePosition = 0;
//      m_nBufferPlayPosition = 0;
      m_nLastCursorPosition = 0;
      m_nAutoPausePosition  = 0;
      if (m_pVSTHostTrack)
         m_pVSTHostTrack->Start();
      if (m_pVSTHostMaster)
         m_pVSTHostMaster->Start();
      if (m_pVSTHostFinal)
         m_pVSTHostFinal->Start();
      if (m_pVSTHostRecord)
         m_pVSTHostRecord->Start();
      unsigned int nChannelIndex;
      for (nChannelIndex = 0; nChannelIndex < m_vOutput.size(); nChannelIndex++)
         m_vOutput[nChannelIndex]->Start();
      for (nChannelIndex = 0; nChannelIndex < m_vInput.size(); nChannelIndex++)
         m_vInput[nChannelIndex]->Start();
      for (nChannelIndex = 0; nChannelIndex < m_vTracks.size(); nChannelIndex++)
         m_vTracks[nChannelIndex]->Start();
      m_hwTrackGain.SetState(WINDOWSTATE_UP);
      }
   m_nHangsForError = m_nStartTimeout / m_pscSoundClass->SoundGetWatchdogTimeout();

   // ---- file2file operation -----
   if (m_bFile2File)
      {
      // first check for endless loop (then file2file is not allowed) and
      // determine the total length to play
      uint64_t nTotalLength = 0;
      uint64_t nTrackLength;
      unsigned int nTracks = (unsigned int)m_vTracks.size();
      for (unsigned int i = 0; i < nTracks; i++)
         {
         if (m_vTracks[i]->EndlessLoop())
            throw Exception("cannot start file2file operation because track " + IntToStr((int)i) + " runs an endless data loop");
         nTrackLength = (uint64_t)m_vTracks[i]->RemainingDataLength();
         if (nTrackLength > nTotalLength)
            nTotalLength = nTrackLength;
         }
      unsigned int nChannels = (unsigned int)m_vvfBuffersOutFile2File.size();
      bool bEmergencyStop;
      while (1)
         {
         // reset internal dummy data buffer to be passed to processing callbacks
         m_bFile2FileDone = false;
         for (unsigned int i = 0; i < nChannels; i++)
            m_vvfBuffersOutFile2File[i] = 0.0f;
         // call all three processing callbacks subsequently
         Process(m_vvfBuffersInFile2File, m_vvfBuffersOutFile2File, m_bFile2FileDone);
         OnBufferPlay(m_vvfBuffersOutFile2File);
         OnBufferDone(m_vvfBuffersInFile2File, m_vvfBuffersOutFile2File, m_bFile2FileDone);
         Application->ProcessMessages();
         // check if we're done (or have to do emergency break)
         if (IsAudioSpike())
            bEmergencyStop = false;
         else
            bEmergencyStop = m_nBufferDonePosition > nTotalLength + 1000000;
         // FOR TESTING EMERGENCY BREAK: bEmergencyStop = m_nBufferDonePosition > nTotalLength/2;
         if (m_bFile2FileDone || bEmergencyStop)
            {
            Stop(false, false);
            unsigned int nChannelIndex;
            for (nChannelIndex = 0; nChannelIndex < m_vOutput.size(); nChannelIndex++)
               m_vOutput[nChannelIndex]->Stop();
            ClearData();
            if (bEmergencyStop)
               throw Exception("internal error: file2file operation exceeds data length. Operation aborted");
            break;
            }
         }
      }
   // ----- regular device driven operation (_not_ file2file)
   else
      {
      Unmute();
      Unpause();
      m_vanXrunCounter     = 0;
      m_nProcessedBuffers  = 0;
      m_nDoneBuffers       = 0;
      m_nPlayedBuffers     = 0;
      m_nHangsDetected     = 0;
      // reset clip counts
      ResetClipCount();
      #pragma clang diagnostic push
      #pragma clang diagnostic ignored "-Wfloat-equal"
      if (m_dSampleRate != SoundGetSampleRate())
         {
         Stop(false, false);
         throw Exception("setting samplerate of device to " + FloatToStr((long double)m_dSampleRate) + " not successful");
         }
      #pragma clang diagnostic pop
                        
      // set initial gain and ramp status
      m_vfGain = m_vfPendingGain;                    
      unsigned int hwLen = (unsigned int)floor((double)m_fRampLength/1000.0 * SoundGetSampleRate());
      m_hwMute.SetLength(hwLen);
      // adjust ramp length's directly _after_ start! Samplerate may have changed on start!
      m_hwPause.SetLength(hwLen);
      m_hwMute.SetLength(hwLen);
      m_hwGain.SetLength(hwLen);
      // mute ramp here to have silence directly!
      m_hwMute.SetState(WINDOWSTATE_DOWN);
      m_nHangsDetected = 0;
      m_pscSoundClass->SoundStart();
      m_nHangsDetected = 0;
      // NEW on 01.08.08: wait a bit before calling WaitForRampState to
      // have threads up and running
      // NOTE: removed again on 11.9.08: causes missing data on output in the beginning!
      // Not necessary any longer anyway, because of new waiting feature below!
      // Sleep(50);
      // adjust ramp length's directly _after_ start! Samplerate may have changed on start!
      m_hwPause.SetLength(hwLen);
      m_hwMute.SetLength(hwLen);
      m_hwGain.SetLength(hwLen);
      // NOTE: we have to re-set ramps to their desired state again _after_
      // setting ramp length!
      m_hwGain.SetState(WINDOWSTATE_UP);
      m_hwMute.SetState(WINDOWSTATE_DOWN);
      m_hwPause.SetState(WINDOWSTATE_UP);

      // calculate number of 'OnHangs' that correspond to m_nStartTimeout
      unsigned int nHangsForError = m_nStartTimeout / m_pscSoundClass->SoundGetWatchdogTimeout() - 1;
      // wait for a callback to arrive
      DWORD dw = GetTickCount();
      while (m_pscSoundClass->ProcessCalls() == 0)
         {
         Application->ProcessMessages();
         if (m_nHangsDetected >= nHangsForError)
            throw Exception("error starting device: driver does not want data!");
         if (!DeviceIsRunning())
            throw Exception("error starting device: device has stopped again (maybe it's in use)!");
         if (ElapsedSince(dw) > 10000)
            throw Exception("error starting device: timout occurred: driver does not want data!");
         }
      // use mute ramp to do a smooth start, but only if NOT waiting for threshold, otherwise
      // we will get a timeout immediately!!
      if (!IsWaitingForStartThreshold())
         WaitForRampState(m_hwMute, WINDOWSTATE_UP);
      else
         m_hwMute.SetState(WINDOWSTATE_UP);
      // reset
      m_nHangsDetected = 0;
      m_nHangsForError = 1;
      }
}
//------------------------------------------------------------------------------

//------------------------------------------------------------------------------
/// Clears all data loaded to tracks and resets positions
/// counter
//------------------------------------------------------------------------------
void SoundDllProMain::ClearData(void)
{
   EnterCriticalSection(&m_csProcess);
   try
      {
      for (unsigned int nTrackIndex = 0; nTrackIndex < m_vTracks.size(); nTrackIndex++)
         m_vTracks[nTrackIndex]->Cleanup(true);
      m_pfrmTracks->UpdateData(true);
      m_nLoadPosition         = 0;
      m_nBufferPlayPosition   = 0;
      m_nBufferDonePosition   = 0;
      }
   __finally
      {
      LeaveCriticalSection(&m_csProcess);
      }
}
//------------------------------------------------------------------------------

//------------------------------------------------------------------------------
/// clears data in one or more tracks and clears track frame as well
//------------------------------------------------------------------------------
void SoundDllProMain::ClearTracks(std::vector<int> viTracks)
{
   if (IsPlaying() && !Paused() && m_nRunLength != 0)
      throw Exception("tracks can only be cleared while device is running, if 'start' was called with 'length' set to 0!");
   EnterCriticalSection(&m_csProcess);
   try
      {
      unsigned int nTracks = (unsigned int)viTracks.size();
      if (!nTracks)
         throw Exception("no channels track for clearing");
      for (unsigned int nTrack = 0; nTrack < nTracks; nTrack++)
         {
         if (viTracks[nTrack] < 0 || viTracks[nTrack] >= (int)m_vTracks.size())
            throw Exception("invalid track specified for clearing");
         m_vTracks[(unsigned long)viTracks[nTrack]]->Cleanup(true);
         }
      m_pfrmTracks->UpdateData(true);
      }
   __finally
      {
      LeaveCriticalSection(&m_csProcess);
      }
}
//------------------------------------------------------------------------------

//------------------------------------------------------------------------------
/// Stops visualisation, channels, tracks. Resets marking buttons and perfomance
/// counter
//------------------------------------------------------------------------------
void SoundDllProMain::DoStop(void)
{
   m_mbMarkButtons.ResetButtons();
   // wait until OnVisualizeDone is called with timeout. If it's never called
   // we call OnVisualizeDone by hand to close rec and debug files manually!
   DWORD dw = WaitForSingleObject(&m_hOnVisualizeDoneEvent, 1000);
   ResetEvent(m_hOnVisualizeDoneEvent);
   if (dw == WAIT_TIMEOUT)
	  OnStopComplete();
   ResetLevels();
   m_nLoadPosition         = 0;
   m_nBufferPlayPosition   = 0;
   m_nBufferDonePosition   = 0;
}
//------------------------------------------------------------------------------

//------------------------------------------------------------------------------
/// Stops device, visualization, calls DoStop
//------------------------------------------------------------------------------
void SoundDllProMain::Stop(void)
{
   try
      {
      // reset event that is set by OnVisualizeDone function, to wait in
      // DoStop for it
      ResetEvent(m_hOnVisualizeDoneEvent);
      m_pscSoundClass->SoundStop();
      m_nHangsForError = 1;
      m_nHangsDetected = 0;
      }
   __finally
      {
      DoStop();
      }
}
//------------------------------------------------------------------------------

//------------------------------------------------------------------------------
/// Does (smooth) Stop, calls SoundStop and checks if stop was REALLY done 
//------------------------------------------------------------------------------
void SoundDllProMain::Stop(bool bSmooth, bool bWaitForStopDone)
{
   try
      {
      // reset event that is set by OnVisualizeDone function, to wait in
      // DoStop for it
      ResetEvent(m_hOnVisualizeDoneEvent);
      if (!DeviceIsRunning())
         {
         ClearData();
         return;
         }
      if (bSmooth && !Paused() && !Muted())
         Mute();

      // NOTE: sound class itself waits for stop done ...
      m_pscSoundClass->SoundStop(bWaitForStopDone);
      // ... but additionally check here, if stop was REALLY done
      if (bWaitForStopDone)
         {
         // 9.09.08: new feature for (buggy) soundcards: check, that stop is really
         // done, i.e. that buffer switch is really not called any more!
         uint64_t nPlayedBuffers;
         DWORD dwTimoutStart  = GetTickCount();
         DWORD dwTimeOut      = (DWORD)m_nStopTimeout;
         DWORD dwWait         = (DWORD)(3000.0*m_dSecondsPerBuffer);
         DWORD dwNow;
         while (1)
            {
            // wait for three 'expected' callbacks. If no callback happened within that
            // time, stop seemed to be successful
            nPlayedBuffers = m_nPlayedBuffers;
            dwNow = GetTickCount();
            while (ElapsedSince(dwNow) < dwWait)
               Application->ProcessMessages();
            // still same count?  All ok!
            if (nPlayedBuffers == m_nPlayedBuffers)
               break;
            // check for (global) timeout
            if (ElapsedSince(dwTimoutStart) > dwTimeOut)
               throw Exception("device did not stop in time!");
            }
         }
      }
   __finally
      {
      // reset initial gain and ramp status (if gain ramp running on call to ::Stop)
      m_vfGain = m_vfPendingGain;
      m_hwGain.SetState(WINDOWSTATE_UP);
      m_nHangsForError = 1;
      DoStop();
      }
}
//------------------------------------------------------------------------------

//------------------------------------------------------------------------------
/// Resets playback position to a certain position
//------------------------------------------------------------------------------
void SoundDllProMain::SetPosition(uint64_t nPosition)
{                                   
   if (m_bAutoCleanup)
      throw Exception("position cannot be resetted if 'autocleardata' is active (see command 'init')");
//   if (IsPlaying() && !Paused())
//      throw Exception("position cannot be resetted if device is running!");
   EnterCriticalSection(&m_csProcess);
   try
      {
      uint64_t nDelta = m_nLoadPosition - nPosition;
      m_nLoadPosition         -= nDelta; // i.e. = nPosition
      m_nBufferPlayPosition   -= nDelta;
      m_nBufferDonePosition   -= nDelta;

      for (unsigned int nTrackIndex = 0; nTrackIndex < m_vTracks.size(); nTrackIndex++)
         m_vTracks[nTrackIndex]->SetPosition(nPosition);
      if (m_pfrmTracks->Visible)
         {
         m_pfrmTracks->SetCursorPos((int64_t)m_nBufferPlayPosition);
         m_nLastCursorPosition = m_nBufferPlayPosition;
         }
      }
   __finally
      {
      LeaveCriticalSection(&m_csProcess);
      }
}
//------------------------------------------------------------------------------

//------------------------------------------------------------------------------
/// Callback called by sound class. Stops output and stores error message
//------------------------------------------------------------------------------
void SoundDllProMain::OnError()
{
   try
      {
      Stop(false, false);
      }
   catch (...)
      {
      }
   if (Trim(m_strFatalError).Length() == 0)
      m_strFatalError = "An unknown error occurred in the sound class";
   // OutputDebugString(("Fatal error: " + m_strFatalError).c_str());
}
//------------------------------------------------------------------------------

//------------------------------------------------------------------------------
/// Callback called by sound class. Posts a message to AbaoutForm with current
/// device status 
//------------------------------------------------------------------------------
void SoundDllProMain::OnStateChange(State asState)
{
   // NOTE: we must not set VCL captions here, because we are in processing thread!
   // We only set m_pfrmAbout->PerformanceTimer->Tag. That Timer sets caption in it's own
   // OnTimer event!
   if (m_pfrmAbout)
      m_pfrmAbout->PerformanceTimer->Tag = asState;
}
//------------------------------------------------------------------------------

//------------------------------------------------------------------------------
/// Callback called by sound class on every Xrun. Increases XRun counter
/// \param[in] xtXrunType type of XRun (not used
//------------------------------------------------------------------------------
void SoundDllProMain::OnXrun(XrunType xtXrunType)
{
   m_vanXrunCounter[xtXrunType]++;
//   OutputDebugString((IntToStr(m_vanXrunCounter.sum()) + " xruns (" + IntToStr(xtXrunType) + ")").c_str());
}
//------------------------------------------------------------------------------

//------------------------------------------------------------------------------
/// Callback called by sound class. If m_nHangsForError is exceeded device is
/// stopped and error is stored
//------------------------------------------------------------------------------
void SoundDllProMain::OnHang()
{
   m_nHangsDetected++;
   
   if (m_nHangsDetected < m_nHangsForError)
      return;
   m_strFatalError = "hang detected: driver does not want data (any more)";
   int nBufSizeBest = m_pscSoundClass->SoundBufsizeBest();
   if (m_nBufSizeBest != nBufSizeBest)
      m_strFatalError += ": most probably the buffersize of the driver was changed externally.";
   Stop(false, false);
}
//------------------------------------------------------------------------------

//------------------------------------------------------------------------------
/// calls SetGain class and updates sliders on mixer
//------------------------------------------------------------------------------
void SoundDllProMain::SetOutputGain(const std::vector<float> & vfGain)
{
   SetGain(vfGain);
   m_pfrmMixer->UpdateOutputSliders(vfGain);
}
//------------------------------------------------------------------------------

//------------------------------------------------------------------------------
/// \brief sets linear gain value for output channels
/// \param[in] vfGain vector containing gains (size must be <= number of available output channels)
/// \exception Asio::EAsioError if state of m_hwGain is not WINDOWSTATE_UP
/// \exception Asio::EAsioError if size of vfGain exceeds number of available output channels
//------------------------------------------------------------------------------
void SoundDllProMain::SetGain(const std::vector<float> & vfGain)
{
   // Note thread saftey here is guaranteed by:
   // -  entering function is forbidden, if corresponding ramp state
   //    is != WINDOWSTATE_UP. If it _is_ WINDOWSTATE_UP, the processing thread
   //    never accesses m_vfPendingGain
   // - m_vfPendingGain is only written here.
   // NOTE: this must never happen, because we wait for ramp to be up below!
   if (m_hwGain.GetState() != WINDOWSTATE_UP)
      throw Exception("gain ramp is not in state up!");
   // check size
   unsigned int nSize = (unsigned int)vfGain.size();
   if (nSize > m_vfGain.size())
      throw Exception("fatal dim-error");
   unsigned int nChannel;
   for (nChannel = 0; nChannel < nSize; nChannel++)
      m_vfPendingGain[nChannel] = vfGain[nChannel];
   // device is not running, so we simply copy pending gain to applied gain
   if (!DeviceIsRunning() || Paused() || Muted())
      m_vfGain = m_vfPendingGain;
   else
      {
      // set ramp down
      m_hwGain.SetState(WINDOWSTATE_DOWN);
      // and wait 'til it's up again
      WaitForRampState(m_hwGain, WINDOWSTATE_UP);
      }

   // NOTE: we have to check for copying the gains again: if device was stopped
   // while waiting for ramp state, then 'pending' is never reached
   if (!DeviceIsRunning())
      m_vfGain = m_vfPendingGain;
}
//------------------------------------------------------------------------------

//------------------------------------------------------------------------------
/// \brief returns vector with current gains
/// \retval vector containing current gains
//------------------------------------------------------------------------------
std::vector<float> SoundDllProMain::GetGain(void)
{
   return m_vfGain;
}
//------------------------------------------------------------------------------

//------------------------------------------------------------------------------
/// sets gain without ramping with thread synchronisation
/// \param[in] vfGain vector containing gains (size must be <= number of available output channels)
/// \exception Exception if size of vfGain exceeds number of available output channels
//------------------------------------------------------------------------------
void SoundDllProMain::SetGainDirect(const std::vector<float> & vfGain)
{
   if (vfGain.size() != m_vfGain.size())
      throw Exception("fatal dim-error");
   EnterCriticalSection(&m_csProcess);
   try
      {
      m_vfGain = vfGain;
      m_vfPendingGain = vfGain;
      }
   __finally
      {
      LeaveCriticalSection(&m_csProcess);
      }
}
//------------------------------------------------------------------------------

//------------------------------------------------------------------------------
/// Sets new track gains and ramp length for applying them
//------------------------------------------------------------------------------
void SoundDllProMain::SetTrackGain( const std::vector<float > & vfTrackGain,
                                    unsigned int nRampLength,
                                    bool bUpdateSliders)
{
   // Note thread saftey here is guaranteed by:
   // -  entering function is forbidden, if corresponding ramp state
   //    is != WINDOWSTATE_UP. If it _is_ WINDOWSTATE_UP, the processing thread
   //    never accesses m_vfPendingTrackGain
   // - m_vfPendingTrackGain is only written here.
   // NOTE: this must never happen, because we wait for ramp to be up below!
   if (m_hwTrackGain.GetState() != WINDOWSTATE_UP)
      throw Exception("gain ramp is not in state up!");
   // check sizes
   if (  vfTrackGain.size() != m_vfTrackGain.size())
      throw Exception("fatal dim-error 2");
   m_vfPendingTrackGain = vfTrackGain;
   if (nRampLength == 0)
      nRampLength = 1;
   m_hwTrackGain.SetLength(nRampLength);
   // device is not running, so we simply copy pending gain to applied gain
   if (!DeviceIsRunning() || Paused() || Muted())
      {
      m_vfTrackGain = m_vfPendingTrackGain;
      }
   else
      {
      // set ramp down
      m_hwTrackGain.SetState(WINDOWSTATE_DOWN);
      // and wait 'til it's up again
      // WaitForRampStateLen(m_hwTrackGain, WINDOWSTATE_UP, nRampLength);
      WaitForRampState(m_hwTrackGain, WINDOWSTATE_UP, 1000.0f * (float)nRampLength / (float)SoundGetSampleRate());
      }
   // NOTE: we have to check for copying the gains again: if device was stopped
   // while waiting for ramp state, then 'pending' is never reached
   if (!DeviceIsRunning())
      m_vfTrackGain = m_vfPendingTrackGain;
   if (bUpdateSliders)
      m_pfrmMixer->UpdateTrackSliders();
}
//------------------------------------------------------------------------------

//------------------------------------------------------------------------------
/// returns current track gain
//------------------------------------------------------------------------------
const std::vector<float >& SoundDllProMain::GetTrackGain()
{
   return m_vfTrackGain;
}
//------------------------------------------------------------------------------

//------------------------------------------------------------------------------
/// sets track gain without ramping with thread synchronisation
//------------------------------------------------------------------------------
void SoundDllProMain::SetTrackGainDirect(const std::vector<float > & vfTrackGain)
{
   if (vfTrackGain.size() != m_vfTrackGain.size())
      throw Exception("fatal sizing error out of range");
   EnterCriticalSection(&m_csProcess);
   try
      {
      m_vfTrackGain = vfTrackGain;
      m_vfPendingTrackGain = vfTrackGain;
      }
   __finally
      {
      LeaveCriticalSection(&m_csProcess);
      }
}
//------------------------------------------------------------------------------

//------------------------------------------------------------------------------
/// sets rec gain without ramping
/// \param[in] vfGain vector containing gains (size must be <= number of available input channels)
/// \exception Exception if size of vfGain exceeds number of available input channels
//------------------------------------------------------------------------------
void SoundDllProMain::SetRecGain(const std::vector<float> & vfGain, bool bUpdateSliders)
{
   if (vfGain.size() != m_vfInputGain.size())
      throw Exception("fatal dim-error");
   EnterCriticalSection(&m_csProcess);
   try
      {
      m_vfInputGain = vfGain;
      }
   __finally
      {
      LeaveCriticalSection(&m_csProcess);
      }
   if (bUpdateSliders)
      m_pfrmMixer->UpdateInputSliders(vfGain);
}
//------------------------------------------------------------------------------

//------------------------------------------------------------------------------
/// returns current recording gain
//------------------------------------------------------------------------------
std::vector<float > SoundDllProMain::GetRecGain()
{
   return m_vfInputGain;
}
//------------------------------------------------------------------------------

//------------------------------------------------------------------------------
/// sets channel mute status without ramping with thread synchronisation
//------------------------------------------------------------------------------
void SoundDllProMain::SetChannelMute(  const std::valarray<bool > &vabMute,
                                       TChannelType ct,
                                       bool bUpdateMixer)
{
   if (vabMute.size() != m_vvabChannelMute[ct].size())
	  throw Exception("sizing error in " + UnicodeString(__FUNC__));
   EnterCriticalSection(&m_csProcess);
   try
	  {
	  m_vvabChannelMute[ct] = vabMute;
	  CalculateAppliedMuteStatus(ct);
	  if (bUpdateMixer)
		 m_pfrmMixer->UpdateButtons(ct);
	  }
   __finally
	  {
	  LeaveCriticalSection(&m_csProcess);
	  }
}
//------------------------------------------------------------------------------

//------------------------------------------------------------------------------
/// returns channel mute status vector
//------------------------------------------------------------------------------
const std::valarray<bool >& SoundDllProMain::GetChannelMute(TChannelType ct)
{
   return m_vvabChannelMute[ct];
}
//------------------------------------------------------------------------------

//------------------------------------------------------------------------------
/// resets clip counters
//------------------------------------------------------------------------------
void SoundDllProMain::ResetClipCount()
{
   EnterCriticalSection(&m_csProcess);
   try
      {
      unsigned int n = (unsigned int)m_vvnClipCount[Asio::OUTPUT].size();
      unsigned int nChannelIndex;
      for (nChannelIndex = 0; nChannelIndex < n; nChannelIndex++)
        {
        m_vvnClipCount[Asio::OUTPUT][nChannelIndex] = 0;
        }
      n = (unsigned int)m_vvnClipCount[Asio::INPUT].size();
      for (nChannelIndex = 0; nChannelIndex < n; nChannelIndex++)
        {
        m_vvnClipCount[Asio::INPUT][nChannelIndex] = 0;
        }
      m_vanTrackClipCount = 0;
      m_pfrmMixer->UpdateClipCounts(m_vanTrackClipCount);
      }
   __finally
      {
      LeaveCriticalSection(&m_csProcess);
      }
}
//------------------------------------------------------------------------------

//------------------------------------------------------------------------------
/// returns track clip count vector
//------------------------------------------------------------------------------
const std::valarray<unsigned int >& SoundDllProMain::GetTrackClipCount()
{
   return m_vanTrackClipCount;
}
//------------------------------------------------------------------------------

//------------------------------------------------------------------------------
/// sets channel solo status without ramping with thread synchronisation
//------------------------------------------------------------------------------
void SoundDllProMain::SetChannelSolo(  const std::valarray<bool > &vabSolo,
                                       TChannelType ct,
                                       bool bUpdateMixer)
{
   if (vabSolo.size() != m_vvabChannelSolo[ct].size())
      throw Exception("sizing error in " + UnicodeString(__FUNC__));
   EnterCriticalSection(&m_csProcess);
   try
      {
      m_vvabChannelSolo[ct] = vabSolo;
      CalculateAppliedMuteStatus(ct);
      if (bUpdateMixer)
         m_pfrmMixer->UpdateButtons(ct);
      }
   __finally
      {
      LeaveCriticalSection(&m_csProcess);
      }
}
//------------------------------------------------------------------------------

//------------------------------------------------------------------------------
/// returns channel solo status vector
//------------------------------------------------------------------------------
const std::valarray<bool >& SoundDllProMain::GetChannelSolo(TChannelType ct)
{
   return m_vvabChannelSolo[ct];
}
//------------------------------------------------------------------------------

//------------------------------------------------------------------------------
/// returns comma-separated list of channel names
//------------------------------------------------------------------------------
AnsiString SoundDllProMain::GetChannelNames(TChannelType ct)
{
   AnsiString str;
   unsigned int n;
   if (ct == CT_TRACK)
      {
      for (n = 0; n < SoundClass()->m_vTracks.size(); n++)
         str += "\"" + SoundClass()->m_vTracks[n]->Name() + "\"," ;
      }
   else if (ct == CT_OUTPUT)
      {
      for (n = 0; n < SoundClass()->m_vOutput.size(); n++)
         str += "\"" + SoundClass()->m_vOutput[n]->Name() + "\"," ;
      }
   else
      {
      for (n = 0; n < SoundClass()->m_vInput.size(); n++)
         str += "\"" + SoundClass()->m_vInput[n]->Name() + "\"," ;
      }
   RemoveTrailingChar(str);
   return str;
}
//------------------------------------------------------------------------------

//------------------------------------------------------------------------------
/// writes channel names to passed stringlist
//------------------------------------------------------------------------------
void SoundDllProMain::GetChannelNames(TChannelType ct, TStringList *psl)
{
   if (psl)
      {
      psl->Delimiter = ',';
      psl->DelimitedText = GetChannelNames(ct);
      }
}
//------------------------------------------------------------------------------

//------------------------------------------------------------------------------
/// converts channel names to channel indices and calls ConvertChannelArgument
//------------------------------------------------------------------------------
std::vector<int> SoundDllProMain::ConvertSoundChannelArgument( AnsiString strChannels,
                                                               TChannelType ct,
                                                               int nFlags
                                                               )
{
   TStringList *psl = new TStringList();
   TStringList *pslAllNames = new TStringList();
   unsigned int nNumChannels;
   int n, nTmp;
   try
      {
      psl->Delimiter = ',';
      psl->DelimitedText = strChannels;
      GetChannelNames(ct, pslAllNames);
      for (n = 0; n < psl->Count; n++)
         {
         // if not an int, try to convert string to channel index
         if (!TryStrToInt(psl->Strings[n], nTmp))
            {
            nTmp = pslAllNames->IndexOf(psl->Strings[n]);
            if (nTmp < 0)
               throw Exception("track/channel name unknown: " + psl->Strings[n]);
            psl->Strings[n] = IntToStr(nTmp);
            }
         }
      strChannels = psl->DelimitedText;
      if (ct == CT_TRACK)
         nNumChannels = (unsigned int)m_vTracks.size();
      else if (ct == CT_OUTPUT)
         nNumChannels = (unsigned int)m_vOutput.size();
      else
         nNumChannels = (unsigned int)m_vInput.size();
      }
   __finally
      {
      TRYDELETENULL(psl);
      TRYDELETENULL(pslAllNames);
      }
   return ConvertChannelArgument(strChannels, (int)nNumChannels, nFlags);
}
//------------------------------------------------------------------------------

//------------------------------------------------------------------------------
/// calculates mute values to be applied from current solo and mute status vectors
//------------------------------------------------------------------------------
void SoundDllProMain::CalculateAppliedMuteStatus(TChannelType ct)
{
   EnterCriticalSection(&m_csProcess);
   try
      {
      unsigned int nChannel;
      bool bSolo = false;
      for (nChannel = 0; nChannel < m_vvabChannelSolo[ct].size(); nChannel++)
         {
         if (m_vvabChannelSolo[ct][nChannel])
            {
            bSolo = true;
            break;
            }
         }
      if (bSolo)
         {
         for (nChannel = 0; nChannel < m_vvabChannelSolo[ct].size(); nChannel++)
            m_vvabAppliedChannelMute[ct][nChannel] = !m_vvabChannelSolo[ct][nChannel];
         }
      else
         {
         for (nChannel = 0; nChannel < m_vvabChannelMute[ct].size(); nChannel++)
            m_vvabAppliedChannelMute[ct][nChannel] = m_vvabChannelMute[ct][nChannel];
         }
      }
   __finally
      {
      LeaveCriticalSection(&m_csProcess);
      }
}
//------------------------------------------------------------------------------

//------------------------------------------------------------------------------
/// returns a vector containing all input channel indices that are currently
/// mapped to a track
//------------------------------------------------------------------------------
std::vector<unsigned int>   SoundDllProMain::GetTrackInputChannels(unsigned int nTrackIndex)
{
   std::vector<unsigned int> vi;
   unsigned nChannel, nIndex;
   // go through all input channels
   for (nChannel = 0; nChannel < m_vviIOMapping.size(); nChannel++)
      {
      for (nIndex = 0; nIndex < m_vviIOMapping[nChannel].size(); nIndex++)
         {
         if (m_vviIOMapping[nChannel][nIndex] == (int)nTrackIndex)
            {
            vi.push_back(nChannel);
            break;
            }
         }
      }
   return vi;
}
//------------------------------------------------------------------------------

//------------------------------------------------------------------------------
/// \brief main processing callback called by sound class
/// \param[in,out] vvfBuffersIn
///  A vector of sound signal buffers, containing the input signal
///  as float samples in the range [-1:+1].  Each buffer corresponds
///  to one of the prepared input channels.
/// \param[in,out] vvfBuffersOut
///  A vector of sound signal buffers, containing the input signal
///  as float samples in the range [-1:+1].  Each buffer corresponds
///  to one of the prepared output channels.
/// \param bIsLast
///     A flagwhere this function may indicate, that current buffer is the last to
///     be processed
/// Processing sequence:
/// - checks for input clipping
/// - calls DoSignalProcessing (main signal processing function)
/// - does 'autostop'
/// - applies master gain
/// - checks for ouput clipping
/// - adds demo sound
//------------------------------------------------------------------------------
void SoundDllProMain::Process(vvf& vvfBuffersIn, vvf& vvfBuffersOut, bool& bIsLast)
{
   // NOTE: we check for clipping on the input channels _before_ any signal
   // processing (i.e. before calling m_lpfnAsioProcess), because we want
   // to detect clipping in A/D-conversion, not in signal processing on the input!
   unsigned int nChannels  = (unsigned int)vvfBuffersIn.size();
   unsigned int nFrames    = (unsigned int)SoundBufsizeSamples();
   unsigned int nChannel;
   unsigned int nFrame;
   bIsLast = false;
   // clipping for input: we define 'clipping' as two consecutive full scale samples.
   // We only count buffers for efficiency reasons!
   // Additionally here is the recording gain applied
   if (!!nChannels)
      {
      for (nChannel = 0; nChannel < nChannels; ++nChannel)
         {
         if (vvfBuffersIn[nChannel].size() != nFrames)
            throw Exception("unexpected channel sizing error");
         // NOTE: count one less: we do _not_ detect clipping accross buffers
         for (nFrame = 0; nFrame < nFrames-1; ++nFrame)
            {
            // check for clipping
            if (vvfBuffersIn[nChannel][nFrame] >= m_pscSoundClass->SoundActiveChannelMaxValue(Asio::INPUT, nChannel)*m_vvfClipThreshold[Asio::INPUT][nChannel])
               {
               if (vvfBuffersIn[nChannel][nFrame+1] >= m_pscSoundClass->SoundActiveChannelMaxValue(Asio::INPUT, nChannel)*m_vvfClipThreshold[Asio::INPUT][nChannel])
                  {
                  m_vvnClipCount[Asio::INPUT][nChannel]++;
                  // leave: we only count buffer-wise
                  break;
                  }
               }
            else if (vvfBuffersIn[nChannel][nFrame] <= m_pscSoundClass->SoundActiveChannelMinValue(Asio::INPUT, nChannel)*m_vvfClipThreshold[Asio::INPUT][nChannel])
               {
               if (vvfBuffersIn[nChannel][nFrame+1] <= m_pscSoundClass->SoundActiveChannelMinValue(Asio::INPUT, nChannel)*m_vvfClipThreshold[Asio::INPUT][nChannel])
                  {
                  m_vvnClipCount[Asio::INPUT][nChannel]++;
                  // leave: we only count buffer-wise
                  break;
                  }
               }
            }
         if (m_vfInputGain[nChannel] != 1.0f)
            vvfBuffersIn[nChannel] *= m_vfInputGain[nChannel];
         }
      }
   // call callback even if its muted: maybe someone is counting there
   if (!Paused())
      {
      DoSignalProcessing(vvfBuffersIn, vvfBuffersOut);
      // check for 'autostop/pause
      if (m_bStopOnEmpty)
         {
         bool bIsEmpty = true;
         size_t nTracks       = m_vTracks.size();
         for (unsigned int nTrack = 0; nTrack < nTracks; nTrack++)
            {
            if (m_vTracks[nTrack]->HasData())
               {
               bIsEmpty = false;
               break;
               }
            }
         if (bIsEmpty)
            {
            // if only to be paused store loadposition
            if (m_bPauseOnAutoStop)
               {
               if (!m_nAutoPausePosition)
                  m_nAutoPausePosition = m_nLoadPosition;
               }
            // set 'last' flag for buffer for stopping
            else
               {
               m_bFile2FileDone = true;
               bIsLast = true;
               }
            }
         }
      }
   // apply pause here: generates latency, but otherwise we cannot get it up
   // and running at same sample position...
   ApplyRamp(vvfBuffersOut, m_hwPause);
   // buffer counter incremented in every case (even if zeros are played in pause mode)
   m_nProcessedBuffers++;
}
//------------------------------------------------------------------------------

//------------------------------------------------------------------------------
/// called form ::Process in processing thread (in software buffering loop,
/// so 'time-consuming' processing allowed). Does aaaaaaaaaalll the stuff, namely
/// - copy track data and IO-data together
/// - call track VST plugins
/// - apply track gain
/// - copy track data to output channel data
/// - call master VST plugins
/// - call MATLAB script plugin
/// - applies muting
//------------------------------------------------------------------------------
void SoundDllProMain::DoSignalProcessing(vvf &vvfIn, vvf &vvfOut)
{
   try
      {
      double dSampleRate = SoundGetSampleRate();
      // switch off clang warning: float comparison by purpose
      #pragma clang diagnostic push
      #pragma clang diagnostic ignored "-Wfloat-equal"
      if (m_dSampleRate != dSampleRate)
         throw Exception("samplerate of device changed to " + FloatToStr((long double)dSampleRate));
      #pragma clang diagnostic pop
      
      size_t nChannel, nFrame, nTrack, nIndex;
      size_t nChannels     = vvfOut.size();
      size_t nFrames       = (size_t)SoundBufsizeSamples();
      size_t nInChannels   = vvfIn.size();
      size_t nTracks       = m_vTracks.size();
      if (  m_vOutput.size()        < nChannels
         || m_vviIOMapping.size()   != nInChannels
         )
         throw Exception("sizing error 1");
      EnterCriticalSection(&m_csProcess);

      try
         {
         m_pcProcess[PERF_COUNTER_DSP].Start();
         // call function to check for start threshold
         if (WaitForThreshold(vvfIn, true))
            return;

         // for performance reasons, we apply a track gain ramp only if necessary,
         // i.e. if status of track gain ramp is _not_ WINDOWSTATE_UP
         bool bDoTrackGainRamp   = m_hwTrackGain.GetState() != WINDOWSTATE_UP;
         bool bCopyGains         = false;
         if (bDoTrackGainRamp)
            {
            // generate ramp once in ramp buffer
            for (nFrame = 0; nFrame < nFrames; nFrame++)
               m_vafTrackGainRamp[nFrame] = m_hwTrackGain.GetValue();
            // now we check, if ramp is up now (after retrieving ramp values).
            // If so, we have to copy pending gains to real gains _after_ applying
            // this ramp!
            bCopyGains = (m_hwTrackGain.GetState() == WINDOWSTATE_UP);
            }
         #ifdef PERFORMANCE_TEST
         m_pcTest[0].Start();
         #endif
         // then retrieve track data ...
         for (nTrack = 0; nTrack < nTracks; nTrack++)
            {
            // retrieve buffer from track
            m_vTracks[nTrack]->GetBuffer(m_vvafTrackBuffers[nTrack]);
            // if muted, overwrite with zeros
            if (m_vvabAppliedChannelMute[CT_TRACK][nTrack])
               m_vvafTrackBuffers[nTrack] = 0.0f;
            }
         #ifdef PERFORMANCE_TEST
         m_pcTest[0].Stop();
         m_pcTest[1].Start();
         #endif
         // call recording VST plugins
         if (m_pVSTHostRecord)
            {
            // call external processing (audiospike)
            if (!!m_lpfnExtPreRecVSTProc)
               m_lpfnExtPreRecVSTProc(vvfIn);
            m_pcProcess[PERF_COUNTER_VSTREC].Start();
            m_pVSTHostRecord->Process(vvfIn);
            m_pcProcess[PERF_COUNTER_VSTREC].Stop();
            // call external processing (audiospike)
            if (!!m_lpfnExtPostRecVSTProc)
               m_lpfnExtPostRecVSTProc(vvfIn);
            }
         // ... and do I/O copies (add it to track buffer)
         for (nChannel = 0; nChannel < nInChannels; nChannel++)
            {
            // clear muted input channels
            // IMPORTANT NOTE: this will _not_ mute the input completely, because
            // CAsio puts recorded data to processing queue _and_ done-queue at
            // the same time as copy. Thus we here 'mute' only the copy procedure
            // to the outputs, i.e. we only copy the corresponding input channel
            // to a track, if it is _not_ muted
            for (nIndex = 0; nIndex < m_vviIOMapping[nChannel].size(); nIndex++)
               {
               if (m_vviIOMapping[nChannel][nIndex] >= (int)nTracks)
                  throw Exception("internal channel sizing I/O error");
               if (!m_vvabAppliedChannelMute[CT_INPUT][nChannel])
                  m_vvafTrackBuffers[(unsigned int)m_vviIOMapping[nChannel][nIndex]] += vvfIn[nChannel];
               }
            }
         #ifdef PERFORMANCE_TEST
         m_pcTest[1].Stop();
         #endif
         // call track VST plugins
         if (m_pVSTHostTrack)
            {
            m_pcProcess[PERF_COUNTER_VSTTRACK].Start();
            m_pVSTHostTrack->Process(m_vvafTrackBuffers);
            m_pcProcess[PERF_COUNTER_VSTTRACK].Stop();
            }
         // finally apply gain or gain ramp respectively
         if (bDoTrackGainRamp)
            {
            for (nTrack = 0; nTrack < nTracks; nTrack++)
               m_vvafTrackBuffers[nTrack] *= (m_vafTrackGainRamp*(m_vfPendingTrackGain[nTrack] - m_vfTrackGain[nTrack]) + m_vfTrackGain[nTrack]);
            // finally copy gains if necessary
            if (bCopyGains)
               m_vfTrackGain = m_vfPendingTrackGain;
            }
         // simple static gain
         else
            {
            for (nTrack = 0; nTrack < nTracks; nTrack++)
               m_vvafTrackBuffers[nTrack] *= m_vfTrackGain[nTrack];
            }
         // Now add or multiply up and store levels (maximum)
         unsigned int n = (unsigned int)(m_nLoadPosition / (unsigned int)SoundBufsizeSamples() % (unsigned int)m_nNumTrackLevels);
         float fMax;
         for (nTrack = 0; nTrack < nTracks; nTrack++)
            {
            fMax = ArrayAbsMax(m_vvafTrackBuffers[nTrack]);
            // store maxima for visualization
            if (n < m_vvfTrackLevel.size())
               m_vvfTrackLevel[n][nTrack] = fMax;
            if (fMax > 1.0f)
               m_vanTrackClipCount[nTrack]++;
            // add/multiply up the tracks to the output channels
            if (m_vTracks[nTrack]->Multiply())
               vvfOut[m_vTracks[nTrack]->ChannelIndex()] *= m_vvafTrackBuffers[nTrack];
            else
               vvfOut[m_vTracks[nTrack]->ChannelIndex()] += m_vvafTrackBuffers[nTrack];
            }
         // call external processing (audiospike)
         if (!!m_lpfnExtPreVSTProc)
            m_lpfnExtPreVSTProc(vvfOut);
         // here were done with all track related stuff: call master VST plugins
         // (if m_bMasterVSTAfterMLPlugin NOT set)
         if (m_pVSTHostMaster)
            {
            m_pcProcess[PERF_COUNTER_VSTMASTER].Start();
            m_pVSTHostMaster->Process(vvfOut);
            m_pcProcess[PERF_COUNTER_VSTMASTER].Stop();
            }
         // pass buffers to MATLAB plugin ....
         if (m_pMPlugin)
            {
            m_pcProcess[PERF_COUNTER_ML].Start();
            m_pMPlugin->Process(vvfIn, vvfOut);
            m_pcProcess[PERF_COUNTER_ML].Stop();
            }
         #ifndef FINALVST_ONPLAY
         if (m_pVSTHostFinal)
            {
            m_pcProcess[PERF_COUNTER_VSTMASTER].Start();
            m_pVSTHostFinal->Process(vvfOut);
            m_pcProcess[PERF_COUNTER_VSTMASTER].Stop();
            }
         #endif
         // finally clear muted output channels
         for (nChannel = 0; nChannel < nChannels; nChannel++)
            {
            // clear muted input channels
            if (m_vvabAppliedChannelMute[CT_OUTPUT][nChannel])
               vvfOut[nChannel] = 0.0f;
            }

         // adjust loading position
         m_nLoadPosition   += nFrames;
         }
      __finally
         {
         m_pcProcess[PERF_COUNTER_DSP].Stop();
         LeaveCriticalSection(&m_csProcess);
         }
      }
   catch (EAsioError &e)
      {
      m_strFatalError = e.m_lpszMsg + AnsiString(" (DoSignalProcessing)");
      throw;
      }
   catch (Exception &e)
      {
      m_strFatalError = e.Message + " (" + AnsiString(e.ClassName()) + " in DoSignalProcessing)";
      throw;
      }
}
//------------------------------------------------------------------------------

//------------------------------------------------------------------------------
/// Called by sound class: Visualization, saving to files and polling of data
/// from MATLAB
//------------------------------------------------------------------------------
#pragma argsused
void SoundDllProMain::OnBufferDone(vvf& vvfBuffersIn, vvf& vvfBuffersOut, bool& bIsLast)
{
   #ifdef VIS_DEBUG
   int nStep = 0;
   #endif
   if (Paused() || IsWaitingForStartThreshold())
      {
      // NOTE: buufer counter must be incremented in EVERY case (all three
      // buffer counters are!!)
      m_nDoneBuffers++;
      return;
      }
   try
      {
      uint64_t nBufferPlayPosition = GetSamplePosition();
      if (m_pfrmTracks->Visible)
         {
         if (nBufferPlayPosition > m_nLastCursorPosition)
            m_pfrmTracks->SetCursorPos((int64_t)nBufferPlayPosition);
         m_nLastCursorPosition = nBufferPlayPosition;
         }
      #ifdef VIS_DEBUG
      nStep++;
      #endif
      // ---  VISUALIZATION
      size_t nChannel;
      // no standard visualization for file2file!
      if (!m_bFile2File)
         {
         // store channel maxima for visualization
         for (nChannel = 0; nChannel < vvfBuffersIn.size(); nChannel++)
            {
            // here mute the 'real saved' input channels if necessary
            if (m_vvabAppliedChannelMute[CT_INPUT][nChannel])
               vvfBuffersIn[nChannel] = 0.0f;

            // if not muted we apply the recording gain here. NOTE: this must
            // be done ADDITIONALLY to DoSignalProcessing because buffers are independantly
            // passed by Casio to DoSignalProcessing and OnPlay!!!
            else if (m_vfInputGain[nChannel] != 1.0f)
               vvfBuffersIn[nChannel] *= m_vfInputGain[nChannel];
            m_vfInChannelLevel[nChannel] = ArrayAbsMax(vvfBuffersIn[nChannel]);
            }
         #ifdef VIS_DEBUG
         nStep++;
         #endif
         for (nChannel = 0; nChannel < vvfBuffersOut.size(); nChannel++)
            m_vfOutChannelLevel[nChannel] = ArrayAbsMax(vvfBuffersOut[nChannel]);
         #ifdef VIS_DEBUG
         nStep++;
         #endif
         // calculate track level index to use
         unsigned int nTrackVolumeIndex = (unsigned int)(nBufferPlayPosition / (uint64_t)SoundBufsizeSamples() % (uint64_t)m_nNumTrackLevels);
         if (nTrackVolumeIndex < m_vvfTrackLevel.size())
            m_pfrmMixer->UpdateLevels(m_vvfTrackLevel[nTrackVolumeIndex], m_vanTrackClipCount);

         #ifdef VIS_DEBUG
         nStep++;
         #endif
         // call button marking
         DoButtonMarking((int64_t)nBufferPlayPosition);
         } // file2file

      size_t nChannels  = vvfBuffersOut.size();
      // do debug saving
      for (nChannel = 0; nChannel < nChannels; nChannel++)
         m_vOutput[nChannel]->SaveBuffer(vvfBuffersOut[nChannel]);
      #ifdef VIS_DEBUG
      nStep++;
      #endif

      // do record saving. Here we must sync (see SDPInput::SaveBuffer)
      nChannels  = vvfBuffersIn.size();
      if (nChannels)
         {
         EnterCriticalSection(&m_csBufferDone);
         try
            {
            if (m_bTestCopyOutToIn && vvfBuffersOut.size())
               {
               for (nChannel = 0; nChannel < vvfBuffersOut.size(); nChannel++)
                  {
                  if (nChannel >= nChannels)
                     break;
                  vvfBuffersIn[nChannel] = vvfBuffersOut[nChannel];
                  }
               }
            m_pcProcess[PERF_COUNTER_REC].Start();
            // call function to check for record threshold
            bool bSaveToFile = !WaitForThreshold(vvfBuffersIn, false);
            vvf* pBuffers = &vvfBuffersIn;
            if (m_nRecDownSampleFactor != 1)
               {
               pBuffers = &m_vvfDownSample;
               float fRecDownSampleFactor = (float)m_nRecDownSampleFactor;
               unsigned int nNum = (unsigned int)vvfBuffersIn[0].size()/m_nRecDownSampleFactor;
               if (!!nNum)
                  {
                  for (nChannel = 0; nChannel < nChannels; nChannel++)
                     {
                     if (m_vvfDownSample[nChannel].size() != nNum)
                        m_vvfDownSample[nChannel].resize(nNum);
                     m_vvfDownSample[nChannel] = 0.0f;
                     unsigned int n,m, nIndex;
                     nIndex = 0;
                     for (n = 0; n < nNum; n++)
                        {
                        for (m = 0; m < m_nRecDownSampleFactor; m++)
                           m_vvfDownSample[nChannel][n] += vvfBuffersIn[nChannel][nIndex++];
                        m_vvfDownSample[nChannel][n] /= fRecDownSampleFactor;
                        }
                     }
                  }
               }
            // now pass buffer to for saving
            for (nChannel = 0; nChannel < nChannels; nChannel++)
               m_vInput[nChannel]->SaveBuffer((*pBuffers)[nChannel], bSaveToFile);
            // call external processing (if any)
            if (!!m_lpfnExtDoneProc)
               m_lpfnExtDoneProc(*pBuffers);
            m_pcProcess[PERF_COUNTER_REC].Stop();
            #ifdef VIS_DEBUG
            nStep++;
            #endif
            }
         __finally
            {
            LeaveCriticalSection(&m_csBufferDone);
            }
         }
      m_nBufferDonePosition   += (uint64_t)SoundBufsizeSamples();
      m_nDoneBuffers++;
      }
   catch (EAsioError &e)
      {
      m_strFatalError = e.m_lpszMsg + AnsiString(" (OnBufferDone)");
      #ifdef VIS_DEBUG
      m_strFatalError += "Step: " + IntToStr(nStep);
      #endif
      throw;
      }
   catch (Exception &e)
      {
      m_strFatalError = e.Message + " (" + AnsiString(e.ClassName()) + " in OnBufferDone)";
      #ifdef VIS_DEBUG
      m_strFatalError += "Step: " + IntToStr(nStep);
      #endif
      throw;
      }
}
//------------------------------------------------------------------------------

//------------------------------------------------------------------------------
/// handles record and start threshold (if any). If threshold is exceeded or
/// no threshold active, then false is returned, true otherwise
//------------------------------------------------------------------------------
bool SoundDllProMain::WaitForThreshold(vvf& vvfBuffersIn, bool bStart)
{
   unsigned int nTresholdChannels = bStart ?
                                    (unsigned int)m_viStartThresholdChannels.size() :
                                    (unsigned int)m_viRecordThresholdChannels.size();
   // trivial cases: no input channels or no threshold channels
   if (!vvfBuffersIn.size() || !nTresholdChannels)
      return false;
   // get threshold mode ...
   int   nThresholdMode = bStart ? m_nStartThresholdMode : m_nThresholdMode;
   // and reference to threshold and thrreshold channels
   float &fThreshold    = bStart ? m_fStartThreshold     : m_fRecordThreshold;
   std::vector<int> &viThresholdChannels =   bStart ?
                                             m_viStartThresholdChannels :
                                             m_viRecordThresholdChannels;
   unsigned int nChannel;
   bool bExceeded = false;

   // mode is OR: on channel exceeding the threshold is enough
   if (nThresholdMode != SDA_THRSHLDMODE_AND)
      {
      // go through channels
      for (nChannel = 0; nChannel < nTresholdChannels; nChannel++)
         {
         // threshold exceeded in that channel?
         if (  vvfBuffersIn[(unsigned int)viThresholdChannels[nChannel]].max() > fThreshold
            || vvfBuffersIn[(unsigned int)viThresholdChannels[nChannel]].min() < -fThreshold
            )
            {
            bExceeded = true;
            break;
            }
         }
      }
   // mode is AND: all channels must exceed threshold
   else
      {
      bExceeded = true;
      // go through channels
      for (nChannel = 0; nChannel < nTresholdChannels; nChannel++)
         {
         // threshold exceeded in that channel?
         if (  vvfBuffersIn[(unsigned int)viThresholdChannels[nChannel]].max() > fThreshold
            || vvfBuffersIn[(unsigned int)viThresholdChannels[nChannel]].min() < -fThreshold
            )
            continue;
         bExceeded = false;
         break;
         }
      }
   // if threshold is exceeded, then disable it from now on
   if (bExceeded)
      {
      viThresholdChannels.clear();
      fThreshold = 0.0f;
      }
   return !bExceeded;
}
//------------------------------------------------------------------------------

//------------------------------------------------------------------------------
/// Called by sound class in realtime thread: no time consuming processing
/// allowed here!! Here only channel gain is applied, current position is
/// updated and stopping is initialised, if requested
//------------------------------------------------------------------------------
void SoundDllProMain::OnBufferPlay(vvf& vvfBuffer)
{
   // apply channel gain. Done in real time thread to have lowest latency
   unsigned int nChannels = (unsigned int)vvfBuffer.size();
   unsigned int nChannel, nFrames, nFrame;

   if (!!nChannels)
      {
      nFrames = (unsigned int)vvfBuffer[0].size();
      for (nChannel = 0; nChannel < nChannels; ++nChannel)
         {
         if (vvfBuffer[nChannel].size() != nFrames)
            throw Exception("unexpected channel sizing error");
         }
      // apply a total gain
      if (m_hwGain.GetState() == WINDOWSTATE_UP)
         {
         for (nChannel = 0; nChannel < nChannels; ++nChannel)
            vvfBuffer[nChannel] *= m_vfGain[nChannel];
         }
      else
         {
         float fGain, fWindow;
         bool bPendingGainCopied = false;
         // NOTE: due to ramp we have to do the iteration through frames first!
         for (nFrame = 0; nFrame < nFrames; ++nFrame)
            {
            fWindow = m_hwGain.GetValue();
            for (nChannel = 0; nChannel < nChannels; ++nChannel)
               {
               fGain = m_vfGain[nChannel] + fWindow*(m_vfPendingGain[nChannel] - m_vfGain[nChannel]);
               vvfBuffer[nChannel][nFrame] *= fGain;
               }
            // check, if ramp state is up now (after this call to GetValue() and then copy pending to applied gain
            if (!bPendingGainCopied && m_hwGain.GetState() == WINDOWSTATE_UP)
               {
               m_vfGain = m_vfPendingGain;
               bPendingGainCopied = true;
               }
            }
         }
      // for debugging: place final VST host here rather than in DoSignalProcessing(
      // to be able to check final signal including gain!
      #ifdef FINALVST_ONPLAY
      if (m_pVSTHostFinal)
         {
         m_pcProcess[PERF_COUNTER_VSTMASTER].Start();
         m_pVSTHostFinal->Process(vvfBuffer);
         m_pcProcess[PERF_COUNTER_VSTMASTER].Stop();
         }
      #endif
      // in file2file mode sound class is not initialized, thus SoundActiveChannelMaxValue() would fail
      if (m_pscSoundClass->SoundInitialized())
         {
         // check for clipping. We only count buffers for efficiency reasons!
         // NOTE: hardclipping is done bei CAsio class!!
         for (nChannel = 0; nChannel < nChannels; ++nChannel)
            {
            if (  vvfBuffer[nChannel].max() > m_pscSoundClass->SoundActiveChannelMaxValue(Asio::OUTPUT, nChannel)*m_vvfClipThreshold[Asio::OUTPUT][nChannel]
               || vvfBuffer[nChannel].min() < m_pscSoundClass->SoundActiveChannelMinValue(Asio::OUTPUT, nChannel)*m_vvfClipThreshold[Asio::OUTPUT][nChannel]
               )
               m_vvnClipCount[Asio::OUTPUT][nChannel]++;
            }
         }
      }

   if (!Paused())
      {
      if (IsWaitingForStartThreshold())
         return;
      m_nBufferPlayPosition += (uint64_t)SoundBufsizeSamples();
      if (m_pscSoundClass->SoundIsStopping())
         return;
       /* TODO : evtl. noch einen Puffer warten? */
      if (  (m_nRunLength > 0 && (int64_t)m_nBufferPlayPosition > m_nRunLength)
         || (m_nAutoPausePosition > 0 && m_nBufferPlayPosition > m_nAutoPausePosition)
         )
         {
         // if 'pause-on-stop' active pause withpout ramp rather than stop
         if (m_bPauseOnAutoStop)
            {
            Pause(false);
            }
         else
            {
            Stop(false, false);
            }
         m_nAutoPausePosition  = 0;
         }
      }
   // apply mute ramp in playing callback (minimal latency)
   ApplyRamp(vvfBuffer, m_hwMute);
   // call external  processing
   if (!!m_lpfnExtPostVSTProc)
      m_lpfnExtPostVSTProc(vvfBuffer);

   // buffer counter incremented in every case (even if zeros are played in pause mode)
   m_nPlayedBuffers++;
}
//------------------------------------------------------------------------------

//------------------------------------------------------------------------------
/// called by Sound class, when visualize callback is done, i.e. if device is
/// already stopped and visualize data queue is empty again. Is used here to stop
/// input and output channels (closing record and debug files)
//------------------------------------------------------------------------------
void SoundDllProMain::OnStopComplete()
{
   EnterCriticalSection(&m_csBufferDone);
   try
      {
      unsigned int nChannelIndex;
      for (nChannelIndex = 0; nChannelIndex < m_vOutput.size(); nChannelIndex++)
         m_vOutput[nChannelIndex]->Stop();
      for (nChannelIndex = 0; nChannelIndex < m_vInput.size(); nChannelIndex++)
         m_vInput[nChannelIndex]->Stop();
      for (nChannelIndex = 0; nChannelIndex < m_vTracks.size(); nChannelIndex++)
         m_vTracks[nChannelIndex]->Stop();
      if (m_pVSTHostTrack)
         m_pVSTHostTrack->Stop();
      if (m_pVSTHostMaster)
         m_pVSTHostMaster->Stop();
      if (m_pVSTHostFinal)
         m_pVSTHostFinal->Stop();
      if (m_pVSTHostRecord)
         m_pVSTHostRecord->Stop();
      }
   __finally
      {
      LeaveCriticalSection(&m_csBufferDone);
      // Set event to tell DoStop, that we are done completely
      SetEvent(m_hOnVisualizeDoneEvent);
      DoStop();
      }
}
//------------------------------------------------------------------------------

//------------------------------------------------------------------------------
/// returns current sample position
//------------------------------------------------------------------------------
uint64_t SoundDllProMain::GetSamplePosition()
{
   uint64_t nReturn = m_nBufferPlayPosition;
   #ifdef NOMMDEVICE
   // NOTE: WDM calls 'OnPlay' in bufrered processing, because there is no real 'OnPlay'
   // available: thus we have to correct the audible (!) SamplePosition by BufSize*NumBufs
   // assuming that usually the buffers are filled...
   if (GetDriverModel() == DRV_TYPE_WDM)
      nReturn -= (uint64_t)((double)m_pscSoundClass->SoundBufsizeCurrent() * (double)m_pscSoundClass->SoundNumBufOut());
   #endif
   return nReturn;
}
//------------------------------------------------------------------------------

//------------------------------------------------------------------------------
/// returns current 'load' position, i.e. how many samples are already processed
/// by DoSignalProcessing
//------------------------------------------------------------------------------
uint64_t SoundDllProMain::GetLoadPosition()
{
   return m_nLoadPosition;
}
//------------------------------------------------------------------------------

//------------------------------------------------------------------------------
/// returns actula 'done' position , i.e. how many samples are already processed
/// by OnSaveAndVisualize
//------------------------------------------------------------------------------
uint64_t SoundDllProMain::GetBufferDonePosition()
{
   return m_nBufferDonePosition;
}
//------------------------------------------------------------------------------

//------------------------------------------------------------------------------
/// returns current buffer play position. Should be identical to GetSamplePosition
//------------------------------------------------------------------------------
uint64_t SoundDllProMain::GetBufferPlayPosition()
{
   return m_nBufferPlayPosition;
}
//------------------------------------------------------------------------------

//------------------------------------------------------------------------------
/// returns m_nRecDownSampleFactor
//------------------------------------------------------------------------------
unsigned int SoundDllProMain::GetRecDownSampleFactor()
{
   return m_nRecDownSampleFactor;
}
//------------------------------------------------------------------------------

//------------------------------------------------------------------------------
/// sets m_nRecDownSampleFactor
//------------------------------------------------------------------------------
void SoundDllProMain::SetRecDownSampleFactor(unsigned int n)
{
   if (!n)
      n = 1;
   if (((unsigned int)m_pscSoundClass->SoundBufsizeCurrent() % n) != 0)
      throw Exception("buffer size must be a multiple of down sampling factor");
   if ((int)n > m_pscSoundClass->SoundBufsizeCurrent() / 2)
      throw Exception("down sampling factor must not exceed half buffer size");
    m_nRecDownSampleFactor = n;
}
//------------------------------------------------------------------------------

//------------------------------------------------------------------------------
/// Builds a vector with all track indices and calls IsPlaying(vector)
//------------------------------------------------------------------------------
bool SoundDllProMain::IsPlaying()
{
   std::vector<int> viTracks;
   unsigned int nTrackIndex;
   for (nTrackIndex = 0; nTrackIndex < m_vTracks.size(); nTrackIndex++)
      viTracks.push_back((int)nTrackIndex);
   return IsPlaying(viTracks);
}
//------------------------------------------------------------------------------

//------------------------------------------------------------------------------
/// returns true if any of the tracks contained in passed vector is playing
// data, or false else
//------------------------------------------------------------------------------
bool SoundDllProMain::IsPlaying(const std::vector<int> & viTracks)
{
   if (!DeviceIsRunning())
      return false;
   int nTracksAvail     = (int)m_vTracks.size();
   unsigned int nTracks = (unsigned int)viTracks.size();
   unsigned int nTrackIndex;
   for (nTrackIndex = 0; nTrackIndex < nTracks; nTrackIndex++)
      {
      if (viTracks[nTrackIndex] >= nTracksAvail)
         throw Exception("fatal index error: device index out of range");
      if (m_vTracks[(unsigned int)viTracks[nTrackIndex]]->IsPlaying())
         return true;
      }
   return false;
}
//------------------------------------------------------------------------------

//------------------------------------------------------------------------------
/// Sets current 'starting' mode, i.e. running length and StopOnEmpty mode.
//------------------------------------------------------------------------------
void SoundDllProMain::StartMode(int64_t nRunLength, bool bPause)
{
   m_nRunLength = nRunLength;
   if (m_bFile2File)
	  m_bStopOnEmpty = true;
   else
	  m_bStopOnEmpty = (nRunLength < 0);

   m_bPauseOnAutoStop = bPause;
}
//------------------------------------------------------------------------------

//------------------------------------------------------------------------------
/// returns 'special' VST licenses as ;-delimited list. SoundMexPro returns
/// string "VST;"
//------------------------------------------------------------------------------
AnsiString SoundDllProMain::GetVSTProperties()
{
   return "VST;";
}
//------------------------------------------------------------------------------

//------------------------------------------------------------------------------
/// returns true if an async error ocurred and writes own error message and
/// error message from sound class to passed string
//------------------------------------------------------------------------------
bool  SoundDllProMain::AsyncError(AnsiString &str)
{
   str = m_strFatalError;
   if (m_pscSoundClass)
	  {
	  if (!str.IsEmpty())
		 str += ", ";
	  str += m_pscSoundClass->m_strFatalError = "";
	  }
   return !str.IsEmpty();
}
//------------------------------------------------------------------------------

//------------------------------------------------------------------------------
/// resets async error
//------------------------------------------------------------------------------
void SoundDllProMain::ResetError()
{
   m_strFatalError = "";
   if (m_pscSoundClass)
	  m_pscSoundClass->m_strFatalError = "";
}
//------------------------------------------------------------------------------

//------------------------------------------------------------------------------
/// sets record threshold value and mode for one or more record channels
//------------------------------------------------------------------------------
void SoundDllProMain::SetStartThreshold(std::vector<int> viChannels, float fValue, int nMode)
{
   EnterCriticalSection(&m_csBufferDone);
   try
      {
      m_viStartThresholdChannels.clear();
      if (fValue != 0.0f)
         {
         unsigned int nChannels = (unsigned int)viChannels.size();
         if (!nChannels)
            throw Exception("no channels specified for threshold");
         for (unsigned int nChannel = 0; nChannel < nChannels; nChannel++)
            {
            if (viChannels[nChannel] < 0 || viChannels[nChannel] >= (int)SoundActiveChannels(Asio::INPUT))
               throw Exception("invalid channel specified for threshold");
            m_viStartThresholdChannels.push_back((int)viChannels[nChannel]);
            }
         }
      m_fStartThreshold = fValue;
      m_nStartThresholdMode = nMode;
      }
   __finally
      {
      LeaveCriticalSection(&m_csBufferDone);
      }
}
//------------------------------------------------------------------------------

//------------------------------------------------------------------------------
/// returns current threshold value
//------------------------------------------------------------------------------
float SoundDllProMain::GetStartThreshold(int &nMode)
{
   nMode = m_nStartThresholdMode;
   return m_fStartThreshold;
}
//------------------------------------------------------------------------------

//------------------------------------------------------------------------------
/// returns true, if waiting for start threshold
//------------------------------------------------------------------------------
bool   SoundDllProMain::IsWaitingForStartThreshold()
{
   return m_viStartThresholdChannels.size() > 0;
}
//------------------------------------------------------------------------------

//------------------------------------------------------------------------------
/// sets record threshold value and mode for one or more record channels
//------------------------------------------------------------------------------
void SoundDllProMain::SetRecordThreshold(std::vector<int> viChannels, float fValue, int nMode)
{
   EnterCriticalSection(&m_csBufferDone);
   try
      {
      m_viRecordThresholdChannels.clear();
      if (fValue != 0.0f)
         {
         unsigned int nChannels = (unsigned int)viChannels.size();
         if (!nChannels)
            throw Exception("no channels specified for threshold");
         for (unsigned int nChannel = 0; nChannel < nChannels; nChannel++)
            {
            if (viChannels[nChannel] < 0 || viChannels[nChannel] >= (int)SoundActiveChannels(Asio::INPUT))
               throw Exception("invalid channel specified for threshold");
            m_viRecordThresholdChannels.push_back((int)viChannels[nChannel]);
            }
         }
      m_fRecordThreshold   = fValue;
      m_nThresholdMode     = nMode;
      }
   __finally
      {
      LeaveCriticalSection(&m_csBufferDone);
      }
}
//------------------------------------------------------------------------------

//------------------------------------------------------------------------------
/// returns current record threshold value
//------------------------------------------------------------------------------
float SoundDllProMain::GetRecordThreshold(int &nMode)
{
   nMode = m_nThresholdMode;
   return m_fRecordThreshold;
}
//------------------------------------------------------------------------------

//------------------------------------------------------------------------------
/// adds a button for button marking
//------------------------------------------------------------------------------
void SoundDllProMain::AddButton(HWND hWindowHandle, int64_t nStart, int64_t nLength)
{
   RECT rc;
   ZeroMemory(&rc, sizeof(rc));
   m_mbMarkButtons.AddButton(hWindowHandle, rc, nStart, nLength);
}
//------------------------------------------------------------------------------

//------------------------------------------------------------------------------
/// adds a button for button marking
//------------------------------------------------------------------------------
void SoundDllProMain::AddButton(RECT &rc, AnsiString strCaption, int64_t nStart, int64_t nLength)
{
   m_mbMarkButtons.AddButton(rc, strCaption, nStart, nLength);
}
//------------------------------------------------------------------------------

//------------------------------------------------------------------------------
/// calls button marking for a sample position
//------------------------------------------------------------------------------
void SoundDllProMain::DoButtonMarking(int64_t nSamplePosition)
{
   m_mbMarkButtons.DoButtonMarking(nSamplePosition);
}
//------------------------------------------------------------------------------

//------------------------------------------------------------------------------
/// retrieves information about a plugin by filename or by loaded channel/position.
/// If necessary loads a VST plugin temporarily to retrieve it's properties
//------------------------------------------------------------------------------
void SoundDllProMain::VSTQueryPlugin(TStringList *psl)
{
   if (!psl)
      throw Exception("invalid parameter list passed");
   TVSTHostPlugin* pPlugin = NULL;
   bool bPluginCreated = false;
   try
      {
      AnsiString strFileName = psl->Values[SOUNDDLLPRO_PAR_FILENAME];
      // filename passed: superseeds input/position
      if (!strFileName.IsEmpty())
         {
         if (!FileExists(strFileName))
            throw Exception("VST plugin file '" + strFileName + "' not found");
         // check, if this plugin is already loaded anywhere. If so, use that
         // loaded instance
         TVSTHost* pHost;
         unsigned int nHost, nChannel, nLayer;
         // go through all three hosts and all 'real' plugins
         for(nHost = 0; nHost < 4; nHost++)
            {
            if (nHost == 0)
               pHost = m_pVSTHostTrack;
            else if (nHost == 1)
               pHost = m_pVSTHostMaster;
            else if (nHost == 2)
               pHost = m_pVSTHostFinal;
            else
               pHost = m_pVSTHostRecord;
            if (!pHost)
               continue;
            for (nLayer = 0; nLayer < pHost->m_nLayers; nLayer++)
               {
               for (nChannel = 0; nChannel < pHost->m_nChannels; nChannel++)
                  {
                  if (!pHost->IsPlugin(nLayer, nChannel))
                     continue;
                  if (ExpandFileName(pHost->Plugin(nLayer, nChannel)->m_strLibName) == ExpandFileName(strFileName))
                     pPlugin = pHost->Plugin(nLayer, nChannel);
                  if (!!pPlugin)
                     break;
                  }
               if (!!pPlugin)
                  break;
               }
            if (!!pPlugin)
               break;
            }
         // if not found, load it
         if (!pPlugin)
            {
            bPluginCreated = true;
            std::vector<TVSTNode > vi;
            std::vector<int > vo;
            pPlugin = new TVSTHostPlugin( strFileName,
                                          TVSTHost::VSTHostCallback,
                                          (float)SoundGetSampleRate(),
                                          (int)SoundBufsizeSamples(),
                                          vi,
                                          vo
                                          );
            }
         }
      // no filename passed: query by input/position
      else
         {
         // NOTE: VSTGetPlugin throws exception, if no plugin there...
         pPlugin = VSTGetPlugin( psl->Values[SOUNDDLLPRO_PAR_TYPE],
                                 psl->Values[SOUNDDLLPRO_PAR_POSITION],
                                 psl->Values[SOUNDDLLPRO_PAR_INPUT]);
         }
      // clear list ...
      psl->Clear();
      // and write all values of interest
      AnsiString str = AnsiQuotedStr(pPlugin->m_strEffectName, '"') + ",";
      str += AnsiQuotedStr(pPlugin->m_strProductString, '"') + ",";
      str += AnsiQuotedStr(pPlugin->m_strVendorString, '"');
      psl->Values[SOUNDDLLPRO_PAR_INFO]      = str;
      psl->Values[SOUNDDLLPRO_PAR_INPUT]     = pPlugin->GetNumInputs();
      psl->Values[SOUNDDLLPRO_PAR_OUTPUT]    = pPlugin->GetNumOutputs();
      psl->Values[SOUNDDLLPRO_PAR_PROGRAMS]  = pPlugin->GetPrograms();
      psl->Values[SOUNDDLLPRO_PAR_PROGRAM]   = AnsiQuotedStr(pPlugin->GetProgramName(), '"');
      psl->Values[SOUNDDLLPRO_PAR_PARAMETER] = pPlugin->GetParameterNames();
      psl->Values[SOUNDDLLPRO_PAR_VALUE]     = pPlugin->GetParameterValues(AnsiString());
      }
   __finally
      {
      // cleanup plugin, if locally created
      if (bPluginCreated)
         TRYDELETENULL(pPlugin);
      }
}
//------------------------------------------------------------------------------

//------------------------------------------------------------------------------
/// returns one of the VST hosts by type name
//------------------------------------------------------------------------------
TVSTHost* SoundDllProMain::VSTGetHost(AnsiString strType)
{
   if (strType.IsEmpty() || !strcmpi(strType.c_str(), SOUNDDLLPRO_VAL_MASTER))
      return m_pVSTHostMaster;
   else if (!strcmpi(strType.c_str(), SOUNDDLLPRO_VAL_TRACK))
      return m_pVSTHostTrack;
   else if (!strcmpi(strType.c_str(), SOUNDDLLPRO_VAL_FINAL))
      return m_pVSTHostFinal;
   else if (!strcmpi(strType.c_str(), SOUNDDLLPRO_VAL_INPUT))
      return m_pVSTHostRecord;
   throw Exception("invalid plugin type '" + strType + "'");
}
//------------------------------------------------------------------------------

//------------------------------------------------------------------------------
/// returns a plugin by type, position and channel
//------------------------------------------------------------------------------
TVSTHostPlugin* SoundDllProMain::VSTGetPlugin(  AnsiString strType,
                                                AnsiString strPosition,
                                                AnsiString strChannel)
{
   TVSTHost *pHost = VSTGetHost(strType);
   if (!pHost)
      throw Exception("VSTHost of requested type not active (no corresponding channels in use?)");
   int nPosition;
   // set default for position
   if (strPosition.IsEmpty())
      strPosition = "0";
   if (!TryStrToInt(strPosition, nPosition))
      throw Exception("'position' must be an integer");
   // use first input to identify plugin
   int nChannel = ConvertChannelArgument(strChannel, (int)pHost->m_nChannels)[0];
   return pHost->Plugin((unsigned int)nPosition, (unsigned int)nChannel);
}
//------------------------------------------------------------------------------

//------------------------------------------------------------------------------
/// calls ADM
//------------------------------------------------------------------------------
ASIOError SoundDllProMain::SetInputMonitor(ASIOInputMonitor& aim)
{
   if (GetDriverModel() != DRV_TYPE_ASIO)
      throw Exception("ADM only supported for ASIO driver model");
   return ASIOFuture(kAsioSetInputMonitor, &aim);
}
//------------------------------------------------------------------------------

//------------------------------------------------------------------------------
/// returns if ASIO class does realtime processing or not
//------------------------------------------------------------------------------
bool SoundDllProMain::BufferedIO()
{
   return !m_bRealtime;
}
//------------------------------------------------------------------------------

//------------------------------------------------------------------------------
/// returns if device is running/playing currently
//------------------------------------------------------------------------------
bool SoundDllProMain::DeviceIsRunning()
{
   return m_pscSoundClass->SoundIsRunning();
}
//------------------------------------------------------------------------------

//------------------------------------------------------------------------------
/// returns number of buffers used for processing queue
//------------------------------------------------------------------------------
unsigned int SoundDllProMain::GetQueueNumBufs()
{
   if (m_bFile2File)
	  return 1;
   return m_iProcQueueBuffers;
}
//------------------------------------------------------------------------------

//------------------------------------------------------------------------------
/// returns string with supported samplerates (from array...)
//------------------------------------------------------------------------------
AnsiString SoundDllProMain::GetSupportedSamplerateString()
{
   int nNumRates = sizeof(anTestRates)/sizeof(long);
   AnsiString str;
   for(int i = 0; i < nNumRates; i++)
      {
      if (m_pscSoundClass->SoundCanSampleRate( anTestRates[i]))
         str += IntToStr((int)anTestRates[i]) + ", ";
      }
   str = Trim(str);
   RemoveTrailingChar(str);
   return str;
}
//------------------------------------------------------------------------------

//------------------------------------------------------------------------------
/// returns string with current sound format
//------------------------------------------------------------------------------
AnsiString SoundDllProMain::GetSoundFormatString()
{
   return m_pscSoundClass->SoundFormatString();
}
//------------------------------------------------------------------------------

//------------------------------------------------------------------------------
/// shows DSP load and xrun on form(s)
//------------------------------------------------------------------------------
void SoundDllProMain::ShowPerformance()
{
   static int nCount = 0;
   bool bTracks   = m_pfrmTracks && m_pfrmTracks->Visible;
   bool bMixer    = m_pfrmMixer  && m_pfrmMixer->Visible;
   if (bTracks || bMixer)
      {
      AnsiString str = "DSP: ";
      str += IntToStr((int)floor(100.0*m_pcProcess[PERF_COUNTER_DSP].DecayValue()/m_dSecondsPerBuffer));
      str += "/";
      str += IntToStr((int)floor(100.0*m_pcProcess[PERF_COUNTER_DSP].MaxValue()/m_dSecondsPerBuffer));

      if (bTracks)
         {
         m_pfrmTracks->SetStatusString(str, 2);
         if ((nCount % 3) == 0)
            m_pfrmTracks->SetStatusString("xruns: " + IntToStr((int)GetXruns().sum()), 1);
         }
      if (bMixer)
         {
         m_pfrmMixer->SetStatusString(str, 2);
         if ((nCount % 3) == 0)
            m_pfrmMixer->SetStatusString("xruns: " + IntToStr((int)GetXruns().sum()), 1);
         }
      nCount++;
      }
}
//------------------------------------------------------------------------------

//------------------------------------------------------------------------------
/// resets internal levels and GUI-Levelmeters
//------------------------------------------------------------------------------
void SoundDllProMain::ResetLevels()
{
   m_vfInChannelLevel   = 0.0f;
   m_vfOutChannelLevel  = 0.0f;
   if (m_pfrmMixer)
	  m_pfrmMixer->ResetLevels();
}
//------------------------------------------------------------------------------

//------------------------------------------------------------------------------
/// returns m_bFile2File
//------------------------------------------------------------------------------
bool   SoundDllProMain::IsFile2File()
{
   return m_bFile2File;
}
//------------------------------------------------------------------------------

//------------------------------------------------------------------------------
/// calls SoundLoadDriver of sound class
//------------------------------------------------------------------------------
void SoundDllProMain::SoundLoadDriver(AnsiString strDriver)
{
   int nIndex;
   if (TryStrToInt(strDriver, nIndex))
      {
      if (nIndex >= (int)SoundNumDrivers() || nIndex < 0)
         throw Exception("invalid driver index passed: out of range");
      m_pscSoundClass->SoundLoadDriverByIndex((unsigned int)nIndex);
      }
   else
      {
      m_pscSoundClass->SoundLoadDriverByName(strDriver);
      }
}
//------------------------------------------------------------------------------

//------------------------------------------------------------------------------
/// calls SoundUnloadDriver of sound class
//------------------------------------------------------------------------------
void SoundDllProMain::SoundUnloadDriver(void)
{
   try
      {
      m_pscSoundClass->SoundUnloadDriver();
      Exit();
      }
   catch (...)
      {
      }
}
//------------------------------------------------------------------------------

//------------------------------------------------------------------------------
/// calls SoundShowControlPanel of sound class
//------------------------------------------------------------------------------
void SoundDllProMain::SoundShowControlPanel(void)
{
   m_pscSoundClass->SoundShowControlPanel();
}
//------------------------------------------------------------------------------

//------------------------------------------------------------------------------
/// returns samplerate from sound class or for file2file-action
//------------------------------------------------------------------------------
double SoundDllProMain::SoundGetSampleRate()
{
   if (m_bFile2File)
	  return m_dSampleRate;
   return m_pscSoundClass->SoundGetSampleRate();
}
//------------------------------------------------------------------------------

//------------------------------------------------------------------------------
/// returns latency for a direction samplerate.
//------------------------------------------------------------------------------
#pragma argsused
long SoundDllProMain::SoundGetLatency(Direction adDirection)
{
   return m_pscSoundClass->SoundGetLatency(adDirection);
}
//------------------------------------------------------------------------------

//------------------------------------------------------------------------------
/// queries sound class for number of drivers
//------------------------------------------------------------------------------
size_t SoundDllProMain::SoundNumDrivers()
{
   return m_pscSoundClass->SoundNumDrivers();
}
//------------------------------------------------------------------------------

//------------------------------------------------------------------------------
/// queries sound class for driver name
//------------------------------------------------------------------------------
AnsiString SoundDllProMain::SoundDriverName(unsigned int iIndex)
{
   return m_pscSoundClass->SoundDriverName(iIndex);
}
//------------------------------------------------------------------------------

//------------------------------------------------------------------------------
/// queries sound class for index of current driver
//------------------------------------------------------------------------------
unsigned int SoundDllProMain::SoundCurrentDriverIndex(void)
{
   return m_pscSoundClass->SoundCurrentDriverIndex();
}
//------------------------------------------------------------------------------

//------------------------------------------------------------------------------
/// queries sound class for number of channels
//------------------------------------------------------------------------------
long SoundDllProMain::SoundChannels(Direction adDirection)
{
   return m_pscSoundClass->SoundChannels(adDirection);
}
//------------------------------------------------------------------------------

//------------------------------------------------------------------------------
/// queries sound class for name of channel
//------------------------------------------------------------------------------
AnsiString SoundDllProMain::SoundChannelName(unsigned int iChannelIndex, Direction adDirection)
{
   return m_pscSoundClass->SoundChannelName(iChannelIndex, adDirection);
}
//------------------------------------------------------------------------------

//------------------------------------------------------------------------------
/// returns active channels from sound class or for file2file-action
//------------------------------------------------------------------------------
size_t SoundDllProMain::SoundActiveChannels(Direction adDirection)
{
   if (m_bFile2File)
      {
      if (adDirection == Asio::OUTPUT)
         return (size_t)m_nOutChannelsFile2File;
      else
         return 0;
      }
   else
      return m_pscSoundClass->SoundActiveChannels(adDirection);
}
//------------------------------------------------------------------------------

//------------------------------------------------------------------------------
/// returns current buffer size from sound class or for file2file-action
//------------------------------------------------------------------------------
long SoundDllProMain::SoundBufsizeSamples()
{
   if (m_bFile2File)
      return m_nBufsizeFile2File;
   return m_pscSoundClass->SoundBufsizeCurrent();
}
//------------------------------------------------------------------------------

//------------------------------------------------------------------------------
/// returns active channel names from sound class or for file2file-action
//------------------------------------------------------------------------------
AnsiString SoundDllProMain::SoundGetActiveChannelName(unsigned int iChannelIndex, Direction adDirection)
{
   if (m_bFile2File)
      {
      if (adDirection == Asio::OUTPUT)
         return "Output " + IntToStr((int)iChannelIndex);
      else
         return "Input " + IntToStr((int)iChannelIndex);
      }
   else
      {
      if (!m_pscSoundClass->SoundInitialized())
         throw Exception("Cannot get channel info if not initialized");
      if (iChannelIndex >= SoundActiveChannels(adDirection))
         throw Exception("channel index exceeded");
      return m_pscSoundClass->SoundGetActiveChannelName(iChannelIndex, adDirection);
      }
}
//------------------------------------------------------------------------------

//------------------------------------------------------------------------------
/// returns internal thread priority
//------------------------------------------------------------------------------
int SoundDllProMain::ThreadPriority()
{
   return m_nThreadPriority;
}
//------------------------------------------------------------------------------

//------------------------------------------------------------------------------
/// returns m_dSecondsPerBuffer
//------------------------------------------------------------------------------
double SoundDllProMain::SecondsPerBuffer()
{
   return m_dSecondsPerBuffer;
}
//------------------------------------------------------------------------------

//------------------------------------------------------------------------------
/// \brief pauses the device by starting pause ramp to run down
//------------------------------------------------------------------------------
void SoundDllProMain::Pause(bool bRamped)
{
   if (Paused())
      return;
   if (bRamped)
      WaitForRampState(m_hwPause, WINDOWSTATE_DOWN);
   else
      m_hwPause.SetState(WINDOWSTATE_DOWN);
   // reset initial  gain and ramp status (if gain ramp running on call to Pause)
   m_vfGain = m_vfPendingGain;
   m_hwGain.SetState(WINDOWSTATE_UP);
   m_vfInChannelLevel   = 0.0f;
   m_vfOutChannelLevel  = 0.0f;
   ResetLevels();
   #ifdef PERFORMANCE_TEST
   for (int i = 0; i < NUM_TESTCOUNTER; i++)
      m_pcTest[i].Reset();
   #endif
}
//------------------------------------------------------------------------------

//------------------------------------------------------------------------------
/// \brief unpauses the device by starting pause ramp to run up and resets
/// XRun and Overdriver counter
//------------------------------------------------------------------------------
void SoundDllProMain::Unpause(void)
{
   m_vanXrunCounter = 0;
   WaitForRampState(m_hwPause, WINDOWSTATE_UP);
}
//------------------------------------------------------------------------------

//------------------------------------------------------------------------------
/// \brief returns pause status
/// \retval true if device is paused
/// \retval false else
//------------------------------------------------------------------------------
bool SoundDllProMain::Paused(void)
{
   return (m_hwPause.GetState() == WINDOWSTATE_DOWN);
}
//------------------------------------------------------------------------------

//------------------------------------------------------------------------------
/// \brief mutes the device by starting mute ramp to run down
//------------------------------------------------------------------------------
void SoundDllProMain::Mute(void)
{
   WaitForRampState(m_hwMute, WINDOWSTATE_DOWN);
   // reset initial gain and ramp status (if gain ramp running on call to Mute)
   m_vfGain = m_vfPendingGain;
   m_hwGain.SetState(WINDOWSTATE_UP);
   m_pfrmMixer->SetStatusString("MASTER MUTE", 3);
}
//------------------------------------------------------------------------------

//------------------------------------------------------------------------------
/// \brief mutes the device by starting mute ramp to run up and resets
/// XRun and Overdriver counter
//------------------------------------------------------------------------------
void SoundDllProMain::Unmute(void)
{
   m_vanXrunCounter = 0;
   WaitForRampState(m_hwMute, WINDOWSTATE_UP);
   m_pfrmMixer->SetStatusString("", 3);
}
//------------------------------------------------------------------------------

//------------------------------------------------------------------------------
/// \brief returns mute status
/// \retval true if device is muted
/// \retval false else
//------------------------------------------------------------------------------
bool SoundDllProMain::Muted(void)
{
   return (m_hwMute.GetState() == WINDOWSTATE_DOWN);
}
//------------------------------------------------------------------------------

//------------------------------------------------------------------------------
/// \brief sets clip threshold value
/// \param[in] vfThreshold vector with new thresholds
/// \param[in] adDirection direction of interest (Asio::INPUT or Asio::OUTPUT)
/// \retval vector containing current gains
//------------------------------------------------------------------------------
void SoundDllProMain::SetClipThreshold(const std::vector<float> & vfThreshold, Direction adDirection)
{
   if (DeviceIsRunning() || !m_bInitialized)
      throw Exception("cannot change clipping threshold while device is running or not initialized");
   // check size
   unsigned int nSize = (unsigned int)vfThreshold.size();
   if (nSize > m_vvfClipThreshold[adDirection].size())
      throw Exception("fatal dim-error");
   unsigned int nChannel;
   for (nChannel = 0; nChannel < nSize; nChannel++)
      m_vvfClipThreshold[adDirection][nChannel] = vfThreshold[nChannel];
}
//------------------------------------------------------------------------------

//------------------------------------------------------------------------------
/// \brief returns vector with current clipping thresholds for input or output
/// \param[in] adDirection direction of interest (Asio::INPUT or Asio::OUTPUT)
/// \retval vector containing current clipping thresholds for input or output
//------------------------------------------------------------------------------
std::vector<float> SoundDllProMain::GetClipThreshold(Direction adDirection)
{
   return m_vvfClipThreshold[adDirection];
}
//------------------------------------------------------------------------------

//------------------------------------------------------------------------------
/// \brief returns vector with current clip counts
/// \param[in] adDirection direction of interest (Asio::INPUT or Asio::OUTPUT)
/// \retval vector containing current gains
//------------------------------------------------------------------------------
std::vector<unsigned int> SoundDllProMain::GetClipCount(Direction adDirection)
{
   return m_vvnClipCount[adDirection];
}
//------------------------------------------------------------------------------

//------------------------------------------------------------------------------
/// \brief returns number of Xruns of a certain type
/// \retval valarray with numbers of Xruns
//------------------------------------------------------------------------------
std::valarray<unsigned int>& SoundDllProMain::GetXruns()
{
   return m_vanXrunCounter;
}
//------------------------------------------------------------------------------
//------------------------------------------------------------------------------
/// \brief sets current ramp length in milliseconds
/// \param[in] f ramp length in milliseconds
/// \exception Asio::EStateError if one of the ramps is active
/// \exception Asio::EAsioError if f < 0
//------------------------------------------------------------------------------
void SoundDllProMain::SetRampLength(float f)
{
   // here we cannot use AssertInitialized, because we do not require a
   // 'minimum state', but a 'maximum state': device must not run!!
   if (DeviceIsRunning())
      throw Exception("cannot change ramp length on running device driver!");
   if (f < 0)
      throw Exception("invalid ramplength < 0");
   m_fRampLength = f;
   if (m_fRampLength < 0.1f)
      m_fRampLength = 0.1f;
}
//------------------------------------------------------------------------------

//------------------------------------------------------------------------------
/// \brief applies a ramp factor or muting to a buffer vector
/// \param[in,out] vvfBuffers
///  A vector of sound signal buffers, containing the output signal
///  as float samples in the range [-1:+1].  Each buffer corresponds
///  to one of the prepared output channels.
/// \param[in] hw reference hanning window
//------------------------------------------------------------------------------
void SoundDllProMain::ApplyRamp(vvf & vvfBuffers, CHanningWindow &hw)
{
   // return if ramp is up
   if (hw.GetState() == WINDOWSTATE_UP)
      return;
   unsigned int nChannel;
   unsigned int nChannels = (unsigned int)vvfBuffers.size();
   if (!nChannels)
      {
      // set ramp to extrema if no channels present for any reason
      if (hw.GetState() == WINDOWSTATE_RUNUP)
         hw.SetState(WINDOWSTATE_UP);
      else if (hw.GetState() == WINDOWSTATE_RUNDOWN)
         hw.SetState(WINDOWSTATE_DOWN);
      return;
      }
   // if already down simply write zeroes. CHanningWindow is capapable of doing this
   // as well in frame-loop, but writing complete valarray here is more efficient!!
   if (hw.GetState() == WINDOWSTATE_DOWN)
      {
      for (nChannel = 0; nChannel < nChannels; ++nChannel)
         {
         vvfBuffers[nChannel] = 0.0f;
         }
      return;
      }
   unsigned int nFrame;
   unsigned int nFrames = (unsigned int)vvfBuffers[0].size();
   // this should never happen: different frame lengths in different channels!!
   for (nChannel = 0; nChannel < nChannels; ++nChannel)
      {
      if (vvfBuffers[nChannel].size() != nFrames)
         throw Exception("fatal channel size (frame number) error in "  + UnicodeString(__FUNC__));
      }
   float f;
   for (nFrame = 0; nFrame < nFrames; ++nFrame)
      {
      f = hw.GetValue();
      for (nChannel = 0; nChannel < nChannels; ++nChannel)
         {
         vvfBuffers[nChannel][nFrame] *= f;
         }
      }
}
//------------------------------------------------------------------------------

//------------------------------------------------------------------------------
/// \brief waits for a ramp to be finished with timeout
/// \param[in] hw reference to CHanningWindow of interest
/// \param[in] ws WindowState that we wait for hw to be in that state
/// \param[in] fRampLength length of ramp, defaults to m_fRampLength
/// \exception Asio::EAsioError if ws is other that WINDOWSTATE_DOWN or WINDOWSTATE_UP
//------------------------------------------------------------------------------
void SoundDllProMain::WaitForRampState(CHanningWindow &hw, WindowState ws, float fRampLength)
{
   // check for allowed  values for ws
   if (ws != WINDOWSTATE_DOWN && ws != WINDOWSTATE_UP)
      throw Exception("invalid WindowState value passed to "  + UnicodeString(__FUNC__));
   // ----------------------------------------------------------------------------------------
   // check for allowed (initial) states of hw
   if (hw.GetState() == WINDOWSTATE_RUNDOWN)
      throw Exception("function called for ramp that is already running down "  + UnicodeString(__FUNC__));
   if (hw.GetState() == WINDOWSTATE_RUNUP)
      throw Exception("function called for ramp that is already running up "  + UnicodeString(__FUNC__));
   // end of version 2008-07-28
   // ----------------------------------------------------------------------------------------
   if (fRampLength < 0)
      fRampLength = m_fRampLength;
   // if device is not running, or ramp length is 0, we set rs to ws
   // directly (no ramping necessary!)
   if (!DeviceIsRunning() || fRampLength == 0.0f)
      hw.SetState(ws);
   // if rs is already in state ws
   if (hw.GetState() == ws)
      return;
   if (ws == WINDOWSTATE_DOWN)
      hw.SetState(WINDOWSTATE_RUNDOWN);
   else
      hw.SetState(WINDOWSTATE_RUNUP);
   DWORD dwNow                = GetTickCount();
   DWORD dwTimeout            = (DWORD)(5*fRampLength);
   // set minimum timeout to four times Hang-Detector (we want to detect Hang 'before'
   // ramp timeout to 'see' the correct reason) 
   if (dwTimeout < 4*m_pscSoundClass->SoundGetWatchdogTimeout())
      dwTimeout = 4*m_pscSoundClass->SoundGetWatchdogTimeout();
   // ----------------------------------------------------------------------------------------
   // calculate time per buffer
   double dwMSPerBuffer = 1000.0*(double)SoundBufsizeSamples() / SoundGetSampleRate();
   if (dwTimeout < (5.0* dwMSPerBuffer))
      dwTimeout = (DWORD)(5.0* dwMSPerBuffer);
   // temporary variable to avoid mulitple calls to hw.GetState() in while loop
   WindowState wsTmp;
   // end of version 2008-07-28
   // ----------------------------------------------------------------------------------------
   while (1)
      {
      // ----------------------------------------------------------------------------------------
      // NOTE: called by main thread: keep VCL message loop alive
      Application->ProcessMessages();
      // check state change 'meanwhile'!
      wsTmp = hw.GetState();
      // break here to be very sure: maybe someone other
      // (tries to) run on same ramp an changes state immediately!!)
      if (!DeviceIsRunning())
         {
         hw.SetState(ws);
         break;
         }
      // unwanted 'lock'-state arrived?
      if (wsTmp == WINDOWSTATE_UP && ws == WINDOWSTATE_DOWN)
         throw Exception("ramp state was set to 'UP' while waiting for 'DOWN' state");
      if (wsTmp == WINDOWSTATE_DOWN && ws == WINDOWSTATE_UP)
         throw Exception("ramp state was set to 'DOWN' while waiting for 'UP' state");
      // end of version 2008-07-28
      // ----------------------------------------------------------------------------------------
      // target state arrived?
      if (wsTmp == ws)
         {
         // wait a little to be sure
         Application->ProcessMessages();
         Sleep(10);
         Application->ProcessMessages();
         break;
         }

      if (ElapsedSince(dwNow) > dwTimeout)
         {
         Application->ProcessMessages();
         // 'Emergency' check again: before show timeout check again, if target
         // state reached. This is done, because this function runs in main (VCL)
         // thread and it might have been blocked due to GUI action. So maybe after
         // retrieving the state first, the thread was blocked. Then the timout would
         // occur even if processing thread has finished ramp meanwhile!
         if (hw.GetState() == ws || !DeviceIsRunning())
            return;
         m_strError = "timeout occurred in "  + UnicodeString(__FUNC__);
         if (m_nHangsDetected > 1)
            m_strError += " due to driver hang (" + IntToStr((int)m_nHangsDetected) + " times)";
         m_strError += " after " + IntToStr((int)m_nProcessedBuffers) + " buffers.";
//         OutputDebugString(m_strError.c_str());
         throw Exception(m_strError.c_str());
         }
      }
}
//------------------------------------------------------------------------------

