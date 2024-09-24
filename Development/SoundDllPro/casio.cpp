//-----------------------------------------------------------------------------
/// \file casio.cpp
/// \author Berg
/// \brief Implementation of CAsio class used by Asio module
///
/// Project SoundMexPro
/// Module SoundDllPro
///
/// Implementation of CAsio class used by Asio module. Interface to Steinbergs ASIO-SDK
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
//-----------------------------------------------------------------------------

#include "casio.h"
#include "SoundDataExchanger.h"

// avoid warnings from clang
#pragma clang diagnostic push
#pragma clang diagnostic ignored "-Wreserved-id-macro"
#pragma clang diagnostic ignored "-Wextra-semi"
#pragma clang diagnostic ignored "-Wundef"
#include <asiosys.h>
#include <asio.h>
#ifndef _WIN64
   #include <iasiothiscallresolver.h>
#endif

#include <asiodrivers.h>
#pragma clang diagnostic pop

#include <string.h>
#include <stdio.h>
#include <limits>
#include <algorithm>
#include <typeinfo>
#include <math.h>

//#define TEST_INIT_DEBUG
#ifdef TEST_INIT_DEBUG
#include "SoundDllPro_Tools.h"
#endif



/// A global variable used and exported by the ASIO SDK.
extern AsioDrivers *asioDrivers;

using namespace Asio;

CAsio * CAsio::sm_pcaInstance = 0;
const char * CAsio::sm_lpcszLastWarning = 0;
CAsio::ExceptionTrigger CAsio::sm_etExceptionTrigger = CAsio::NORMAL_OPERATION;

int CAsio::sm_nObjects = 0;
int CAsio::sm_nEvents = 0;
int CAsio::sm_nCriticalSections = 0;
int CAsio::sm_nThreads = 0;

LONG CAsio::sm_anThreadStartFunctionFlags[CASIO_THREADS] = {0, 0, 0, 0};
LONG CAsio::sm_anThreadMainFunctionFlags[CASIO_THREADS] = {0, 0, 0, 0};

/// Error code not specified by the Asio SDK.  Used in testing to simulate
/// unexpected response code from the Asio driver.
#define ASIO_UNEXPECTED 100

/// Maximum number of clock sources that can be handled by CAsio
#define MAX_CLOCKS 256

CAsio * CAsio::Instance()
{
    return sm_pcaInstance;
}

CAsio::CAsio()
    : m_phCbEvents(0),
      m_phStopEvents(0),
      m_phProcEvents(0),
      m_phDoneEvents(0),
      m_nHardwareChannelsIn(-1),
      m_nHardwareChannelsOut(-1),
      m_hCbThread(0),
      m_hStopThread(0),
      m_hProcThread(0),
      m_hDoneThread(0),
      m_padAsioDrivers(0),
      m_asState(FREE),
      m_nCurrentDriverIndex(-1),
      m_padiDriverInfo(NULL),
      m_nActiveChannelsIn(0),
      m_nActiveChannelsOut(0),
      m_rgabiBufferInfos(0),
      m_iPreparedBuffersize(0),
      m_vvbActiveChMasks(),
      m_vviActiveChIndices(),
      m_vvsActiveChNames(),
      m_vviActiveChDataTypes(),
      m_pacbCallbacks(NULL),
      m_nWatchdogTimeout(500),
      m_bStopping(false),
      m_nStopTimeout(0),
      m_nStopTimeoutsWaited(0),
      m_nStopSwitches(4),
      m_nStopSwitchesWaited(0),
      m_nDrvBufferSwitches(0),
      m_nProcBufferSwitches(0),
      m_nWatchdogWakeups(0),
      m_psxSoundDataExchanger(0),
      m_bDataNeedsRealtimeProcessing(false),
      m_nTestingProcessChannelsIn(0),
      m_nTestingProcessFramesIn(0),
      m_nTestingProcessChannelsOut(0),
      m_nTestingProcessFramesOut(0),
      m_nTestingProcessBuffersInWaiting(0),
      m_bTestingProcessPreloading(false),
      m_nTestingBufferDoneChannelsIn(0),
      m_nTestingBufferDoneChannelsOut(0),
      m_nTestingDoneBuffersWaiting(0),
      m_fTestingBufferDoneCaptureSample(0),
      m_fTestingBufferDonePlaybackSample(0),
      m_bShouldProcLoopTerminateCalled(false),
      m_bBuffersWaitingForProcLoopCalled(false),
      m_bHandleProcLoopDataCalled(false),
      m_lpcszTestingLastCallback(0),
      m_xtTestingLastXrunType(XrunType(0)),
      m_nTestAsioBufsizeQuery(0),
      m_bPostOutput(false),
      m_bDoneLoopActive(false),
      m_bDoneLoopWaitsWhenDoneQueuesEmpty(true),
      m_bWaitForDoneDataActive(false)
{
    // Check that this is the only CAsio instance
    if (InterlockedCompareExchangePointer(
            reinterpret_cast<void**>(&sm_pcaInstance),
            this,
            0)
        != 0)
    {
        throw EMultipleInstancesError("CAsio::CAsio",
                                      "Cannot load more than 1 ASIO driver");
    }

    // Check that global pointer inside SDK has no instance yet.
    m_padAsioDrivers = new AsioDrivers;
    ++CAsio::sm_nObjects;
    if (InterlockedCompareExchangePointer(
            reinterpret_cast<void**>(&asioDrivers),
            m_padAsioDrivers,
            0)
        != 0)
    {
        // The Asio SDK is in use, bypassing the CAsio layer. Clean up, give up
        delete m_padAsioDrivers;
        --CAsio::sm_nObjects;
        m_padAsioDrivers = 0;
        InterlockedCompareExchangePointer(
            reinterpret_cast<void**>(&sm_pcaInstance),
            0,
            this);
        throw EMultipleInstancesError("CAsio::CAsio",
                                      "Initializing ASIO SDK:"
                                      " already initialized");
    }

    // Allocate Asio Structures
    m_padiDriverInfo = new ASIODriverInfo();
    ++CAsio::sm_nObjects;
    m_pacbCallbacks = new ASIOCallbacks();
    ++CAsio::sm_nObjects;
    m_pacbCallbacks->bufferSwitch = 0;
    m_pacbCallbacks->bufferSwitchTimeInfo = 0;
    m_pacbCallbacks->sampleRateDidChange = 0;
    m_pacbCallbacks->asioMessage = 0;

    // Initialize Windows Lock and Events for thread communication.
    InitializeCriticalSection(&m_csLock);
    ++CAsio::sm_nCriticalSections;

    try
    {
        // Allocate Event Objects
        InitEvents();
        if (sm_etExceptionTrigger == CONSTRUCTOR_THROWS_WIN32_ERROR)
        {   /// This exception is thrown for testing purposes
            throw EWin32Error("CAsio::CAsio",
                              "Simulating Event creation failure", 0);
        }
        StartThreads();
    }
    catch(...)
    {
        // Release allocated resources and rethrow Exception.

        DestroyEvents();

        // release CS
        DeleteCriticalSection(&m_csLock);
        --CAsio::sm_nCriticalSections;

        // release Asio structures and cleanup global pointers
        DeallocateSDK();

        // reraise exception.
        throw;
    }
}

CAsio::~CAsio()
{
    try
    {
        // nothing in the destructor should ever throw an exception that is
        // not catched immediately.  This wrapping try block catches unexpected
        // errors and segmentation faults, e.g. from the asio driver.

        UnloadDriver();
        
        StopThreads();
        DestroyEvents();

        // release CS
        DeleteCriticalSection(&m_csLock);
        --CAsio::sm_nCriticalSections;

        // release Asio structures and cleanup global pointers
       DeallocateSDK();
    }
    catch(...)
    {
        // Should not occur.
        // If it does, there is nothing we can do about this hypothetical
        // error.
        // In a debug version, we can log its occurrence:
        TriggerWarning("CAsio::~CAsio: "
                       "Unexpected error caught in destructor!");        
    }
}

State CAsio::GetState() const
{
    return m_asState;
}

size_t CAsio::NumDrivers() const
{
    long iNumDrivers = m_padAsioDrivers->asioGetNumDev();
    if (iNumDrivers < 0 
        || sm_etExceptionTrigger == NUM_DRIVERS_THROWS_NEGATIVE_COUNT_ERROR)
    {
        throw ENegativeCountError(__FILE__, __LINE__,
                                  "CAsio::numDrivers",
                                  "Negative number of Asio drivers on system",
                                  this,
                                  iNumDrivers);
    }
    return size_t(iNumDrivers);
}

AnsiString CAsio::DriverNameAtIndex(size_t nIndex) const
{
    // Check range
    if (nIndex >= NumDrivers()) // can throw ENegativeCountError
    {
        throw EIndexError("CAsio::DriverNameAtIndex",
                          "Driver Index out of bounds",
                          0L, long(NumDrivers()) - 1L, (long)nIndex);
    }

    // Get driver name
    char lpszDriverName[MAXDRVNAMELEN];
    long iStatus = m_padAsioDrivers->asioGetDriverName((int)nIndex,
                                                       lpszDriverName,
                                                       MAXDRVNAMELEN);
    if (iStatus != 0 ||
        sm_etExceptionTrigger == DRIVER_NAME_AT_INDEX_THROWS_UNEXPECTED_ERROR)
    {
        throw EUnexpectedError(__FILE__, __LINE__,
                               "CAsio::DriverNameAtIndex",
                               "Unexpected Error code while querying name of"
                               " Asio driver",
                               this);
    }
    return AnsiString(lpszDriverName);
}

void CAsio::UnloadDriver()
{
    // switch off clang warning: fallthrough by purpose
    #pragma clang diagnostic push
    #pragma clang diagnostic ignored "-Wimplicit-fallthrough"
    switch (GetState())
    {
    case RUNNING:
        // stop asynchronously
        Stop();
        // and wait until it really is stopped
        WaitForSingleObject(m_hStoppedEvent,
                            (DWORD)(m_nStopSwitches * m_nStopTimeout + 1));
        // FALL THROUGH
    case PREPARED:
        DisposeBuffers();
        // FALL THROUGH
    case INITIALIZED:
        if (ASIOExit() == ASE_NotPresent)
        {
            TriggerWarning("CAsio::UnloadDriver: "
                           "No input/output present");
        }
        // FALL THROUGH
    case LOADED:
        m_padAsioDrivers->removeCurrentDriver();
        m_nCurrentDriverIndex = -1;
        SetState(FREE);
        break;
    case FREE:
        // Nothing to do in this case
        break;
    }

    m_nHardwareChannelsIn  = -1;
    m_nHardwareChannelsOut = -1;
    #pragma clang diagnostic pop
}

void CAsio::LoadDriverByIndex(size_t nIndex, void * lpSysRef)
{
    // Asio SDK loads drivers by name. Name lookup may throw
    // EIndexError or EUnexpectedError.
    AnsiString sDriverName = DriverNameAtIndex(nIndex);
    LoadDriver(sDriverName, nIndex, lpSysRef);
}

void CAsio::LoadDriverByName(AnsiString sDriverName, void * lpSysRef)
{
    size_t nIndex;
    AnsiString sSystemDriverName;
    for (nIndex = 0; nIndex < NumDrivers(); ++nIndex)
    {
        sSystemDriverName = DriverNameAtIndex(nIndex);
        if (sSystemDriverName == sDriverName)
        {
            LoadDriver(sSystemDriverName, nIndex, lpSysRef);
            return;
        }
    }
    throw ELoadError("CAsio::LoadDriverByName",
                     "No such driver");
}

size_t CAsio::CurrentDriverIndex() const
{
    AssertInitialized("Cannot return current driver's index", LOADED);
    return (size_t)m_nCurrentDriverIndex;
}

void CAsio::AssertInitialized(const char * lpszMsg,
                              State asMinimumState) const
{
    if (int(GetState()) < int(asMinimumState))
    {
        throw EStateError("CAsio::AssertInitialized", lpszMsg,
                          asMinimumState, RUNNING, GetState());
    }
}

long CAsio::DriverInfoAsioVersion() const
{
    AssertInitialized("Cannot return ASIO version supported by driver",
                      LOADED);
    return m_padiDriverInfo->asioVersion;
}

long CAsio::DriverInfoVersion() const
{
    AssertInitialized("Cannot return driver's version number", LOADED);
    return m_padiDriverInfo->driverVersion;
}

AnsiString CAsio::DriverInfoName() const
{
    AssertInitialized("Cannot return driver's internal name", LOADED);
    return m_padiDriverInfo->name;
}

long CAsio::HardwareChannels(Direction adDirection) const
{
   // exception for unit tests only
   if (sm_etExceptionTrigger == HARDWARE_CHANNELS_THROWS_NEGATIVE_COUNT_ERROR)
        throw ENegativeCountError(__FILE__, __LINE__,
                                  "CAsio::HardwareChannelsIntern",
                                  "Asio driver reports negative number of"
                                  " channels",
                                  this,
                                  -1);
   return adDirection == INPUT ? m_nHardwareChannelsIn : m_nHardwareChannelsOut;
}

void CAsio::HardwareChannelsInternal()
{
    AssertInitialized("Cannot query number of channels");
    long nIn = -1;
    long nOut = -1;
    if (ASIOGetChannels(&nIn, &nOut) != ASE_OK)
        return;

    // exception on negative values _and_ for unit tests
    if (nIn < 0 || sm_etExceptionTrigger == HARDWARE_CHANNELS_THROWS_NEGATIVE_COUNT_ERROR)
    {
        throw ENegativeCountError(__FILE__, __LINE__,
                                  "CAsio::HardwareChannelsIntern",
                                  "Asio driver reports negative number of"
                                  "input channels",
                                  this,
                                  nIn);
    }
    if (nOut < 0 || sm_etExceptionTrigger == HARDWARE_CHANNELS_THROWS_NEGATIVE_COUNT_ERROR)
    {
        throw ENegativeCountError(__FILE__, __LINE__,
                                  "CAsio::HardwareChannelsIntern",
                                  "Asio driver reports negative number of"
                                  "output channels",
                                  this,
                                  nOut);
    }

    m_nHardwareChannelsIn  = nIn;
    m_nHardwareChannelsOut = nOut;
}

long CAsio::Latency(Direction adDirection) const
{
    AssertInitialized("Cannot query Asio driver for latency");
    long nIn = 0;
    long nOut = 0;
    ASIOError aeErr;
    // switch off clang warning: not all enum values handled by pupose
    #pragma clang diagnostic push
    #pragma clang diagnostic ignored "-Wswitch-enum"
    switch (sm_etExceptionTrigger)
    {
    case LATENCY_THROWS_UNAVAILABLE_ERROR:
        aeErr = ASE_NotPresent;
        break;
    case LATENCY_THROWS_UNEXPECTED_ERROR:
        aeErr = ASIO_UNEXPECTED;
        break;
    default:
        aeErr = ASIOGetLatencies(&nIn, &nOut);
        break;
    }
    #pragma clang diagnostic pop
    switch (aeErr)
    {
    case ASE_OK:
        break;
    case ASE_NotPresent:
        throw EUnavailableError("CAsio::latency",
                                "Cannot query Asio driver for latency:"
                                " No input/output present");
    default:
        throw EUnexpectedError(__FILE__, __LINE__,
                               "CAsio::latency",
                               "Querying Asio driver for latencies failed"
                               " with unexpected error Code", this, aeErr);
    }
    return (adDirection == INPUT) ? nIn : nOut;
}

long CAsio::BufsizeBest() const
{
    return AsioBufsizeQuery();
}
long CAsio::BufsizeCurrent() const
{
    if (GetState() == PREPARED || GetState() == RUNNING)
    {
        return m_iPreparedBuffersize;
    }
    return BufsizeBest();
}

std::vector<long> CAsio::BufferSizes() const
{
    long iGranularity;
    long iBufsizeMin;
    long iBufsizeMax;
    long iBufsizeBest = AsioBufsizeQuery(&iBufsizeMin, &iBufsizeMax, &iGranularity);

    /// valid buffer sizes differ either by a constant increment
    /// (iGranularity > 0) or by a factor 2 (iGranularity == -1)
    long iIncrement = ( (iGranularity < 0) ? 0 : iGranularity );
    long iFactor = ( (iGranularity < 0) ? 2 : 1 );

    std::vector<long> viBufsizes;

    // NOTE: some drivers return iBufsizeMin != iBufsizeMax AND Granularity == 0.
    // According to the ASIO specification this is a bug.
    // If these buggy values are returned, then we only add Min, Max and Best
    // as possible buffer sizes
    if (iBufsizeMin != iBufsizeMax && iGranularity == 0)
      {
      viBufsizes.push_back(iBufsizeMin);
      viBufsizes.push_back(iBufsizeBest);
      viBufsizes.push_back(iBufsizeMax);
      }
    else
      {
       long iBufsize;
       for (iBufsize = iBufsizeMin;
            iBufsize < iBufsizeMax;
            iBufsize = iBufsize * iFactor + iIncrement)
       {
           viBufsizes.push_back(iBufsize);
       }

       if (iBufsize != iBufsizeMax)
       {

           throw EUnexpectedError(__FILE__, __LINE__,
                                  "CAsio::BufferSizes",
                                  "Driver reports inconsistent buffer sizes",
                                  this);
       }

       viBufsizes.push_back(iBufsize);
       }

    // finally we add BufsizeBest if it is not in vector due to buggy values
    // returned by driver
    if (std::find(viBufsizes.begin(), viBufsizes.end(), iBufsizeBest) == viBufsizes.end())
      viBufsizes.push_back(iBufsizeBest);

    return viBufsizes;
}

bool CAsio::CanSampleRate(double dSampleRate) const
{
    AssertInitialized("Cannot check if sample rate is supported");

    ASIOError aeErr;
    // switch off clang warning: not all enum values handled by pupose
    #pragma clang diagnostic push
    #pragma clang diagnostic ignored "-Wswitch-enum"
    switch(sm_etExceptionTrigger)
    {
    case CAN_SAMPLE_RATE_THROWS_UNAVAILABLE_ERROR:
        aeErr = ASE_NotPresent;
        break;
    case CAN_SAMPLE_RATE_THROWS_UNEXPECTED_ERROR:
        aeErr = ASIO_UNEXPECTED;
        break;
    default:
        aeErr = ASIOCanSampleRate(dSampleRate);
    }
    #pragma clang diagnostic pop
    switch (aeErr)
    {
    case ASE_OK:
        return true;
    case ASE_NoClock:
        return false;
    case ASE_NotPresent:
        throw EUnavailableError("CAsio::canSampleRate",
                                "Cannot check if sample rate is supported:"
                                " No input/output present.");
    }
    throw EUnexpectedError(__FILE__, __LINE__, "CAsio::canSampleRate",
                           "Driver returned unexpected error code",
                           this, aeErr);
}

double CAsio::SampleRate() const
{
    AssertInitialized("Cannot query for current sample rate");
    double dSampleRate = -1;
    ASIOError aeErr;
    // switch off clang warning: not all enum values handled by pupose
    #pragma clang diagnostic push
    #pragma clang diagnostic ignored "-Wswitch-enum"
    switch (sm_etExceptionTrigger)
    {
    case SAMPLE_RATE_THROWS_UNAVAILABLE_ERROR:
        aeErr = ASE_NotPresent;
        break;
    case SAMPLE_RATE_THROWS_UNEXPECTED_ERROR:
        aeErr = ASIO_UNEXPECTED;
        break;
    case SAMPLE_RATE_THROWS_NO_CLOCK_ERROR:
        aeErr = ASE_NoClock;
        break;
    default:
        aeErr = ASIOGetSampleRate(&dSampleRate);
        break;
    }
    #pragma clang diagnostic pop
    switch(aeErr) {
    case ASE_OK:
        return dSampleRate;
    case ASE_NoClock:
        throw ENoClockError("CAsio::SampleRate(void)",
                            "Current sample rate is unknown");
    case ASE_NotPresent:
        throw EUnavailableError("CAsio::SampleRate(void)",
                                "Cannot determine sample rate:"
                                " No input/output present");
    }
    throw EUnexpectedError(__FILE__, __LINE__, "CAsio::SampleRate(void)",
                           "Query for current sample rate returned unexpected"
                           " error code", this, aeErr);
}

void CAsio::SampleRate(double dSampleRate) const
{
    AssertInitialized("Cannot set the sampling rate");
    ASIOError aeErr;
    // switch off clang warning: not all enum values handled by pupose
    #pragma clang diagnostic push
    #pragma clang diagnostic ignored "-Wswitch-enum"
    switch (sm_etExceptionTrigger)
    {
    case SAMPLE_RATE_THROWS_UNAVAILABLE_ERROR:
        aeErr = ASE_NotPresent;
        break;
    case SAMPLE_RATE_THROWS_UNEXPECTED_ERROR:
        aeErr = ASIO_UNEXPECTED;
        break;
    case SAMPLE_RATE_THROWS_MODE_ERROR:
        aeErr = ASE_InvalidMode;
        break;
    default:
        aeErr = ASIOSetSampleRate(dSampleRate);
        break;
    }
    #pragma clang diagnostic pop
    switch (aeErr)
    {
    case ASE_OK:
        return;
    case ASE_NoClock:
        throw ENoClockError("CAsio::SampleRate(double)",
                            "Cannot set the sampling rate"
                            ": Sampling rate is not supported");
    case ASE_InvalidMode:
        throw EModeError("CAsio::SampleRate(double)",
                         "Cannot set the sampling rate"
                         ": Sound card uses external clock");
    case ASE_NotPresent:
        throw EUnavailableError("CAsio::SampleRate(double)",
                                "Cannot set the sampling rate"
                                ": No input/output present");
    }
    throw EUnexpectedError(__FILE__, __LINE__,
                           "CAsio::SampleRate(double)",
                           "Cannot set the sampling rate"
                           ": ASIO driver returned unexpected error code",
                           this, aeErr);
}

size_t CAsio::NumClockSources() const
{
    std::vector<ASIOClockSource> vacsClocks;
    GetClockSources(vacsClocks);
    return vacsClocks.size();
}
AnsiString CAsio::ClockSourceName(size_t nIndex) const
{
    std::vector<ASIOClockSource> vacsClocks;
    GetClockSources(vacsClocks);
    if (nIndex >= vacsClocks.size())
    {
        throw EIndexError("CAsio::ClockSourceName", "Index out of range",
                          0, long(vacsClocks.size()) - 1, (long)nIndex);
    }
    return AnsiString(vacsClocks[nIndex].name);
}
long CAsio::ClockSourceGroup(size_t nIndex) const
{
    std::vector<ASIOClockSource> vacsClocks;
    GetClockSources(vacsClocks);
    if (nIndex >= vacsClocks.size())
    {
        throw EIndexError("CAsio::ClockSourceGroup", "Index out of range",
                          0, long(vacsClocks.size()) - 1, (long)nIndex);
    }
    return vacsClocks[nIndex].associatedGroup;
}
long CAsio::ClockSourceChannel(size_t nIndex) const
{
    std::vector<ASIOClockSource> vacsClocks;
    GetClockSources(vacsClocks);
    if (nIndex >= vacsClocks.size())
    {
        throw EIndexError("CAsio::ClockSourceChannel", "Index out of range",
                          0, long(vacsClocks.size()) - 1, (long)nIndex);
    }
    return vacsClocks[nIndex].associatedChannel;
}

size_t CAsio::ClockSourceCurrent() const
{
    std::vector<ASIOClockSource> vacsClocks;
    GetClockSources(vacsClocks);
    size_t nIndex;
    for (nIndex = 0; nIndex < vacsClocks.size(); ++nIndex)
    {
        if (vacsClocks[nIndex].isCurrentSource == ASIOTrue
            &&
            sm_etExceptionTrigger !=
            CLOCK_SOURCE_CURRENT_THROWS_UNEXPECTED_ERROR)
        {
            return nIndex;
        }
    }
    throw EUnexpectedError(__FILE__, __LINE__, "CAsio::ClockSourceCurrent",
                           "No clock source is currently active", this);
}

void CAsio::ClockSourceSet(size_t nIndex)
{
    if (nIndex >= NumClockSources())
    {
        throw EIndexError("CAsio::ClockSourceSet", "Index out of range",
                          0, long(NumClockSources()) - 1, (long)nIndex);
    }                 
    ASIOError aeErr;
    // switch off clang warning: not all enum values handled by pupose
    #pragma clang diagnostic push
    #pragma clang diagnostic ignored "-Wswitch-enum"
    switch (sm_etExceptionTrigger)
    {
    case CLOCK_SOURCE_SET_TRIGGERS_MODE_ERROR:
        aeErr = ASE_InvalidMode;
        break;
    case CLOCK_SOURCE_SET_TRIGGERS_UNAVAILABLE_ERROR:
        aeErr = ASE_NotPresent;
        break;
    case CLOCK_SOURCE_SET_TRIGGERS_UNEXPECTED_ERROR:
        aeErr = ASIO_UNEXPECTED;
        break;
    default:
        aeErr = ASIOSetClockSource((long)nIndex);

    }
    #pragma clang diagnostic pop
    switch (aeErr)
    {
    case ASE_OK:
        return;
    case ASE_InvalidMode:
        throw EModeError("CAsio::ClockSourceSet",
                         "Cannot switch clock source: Sound card uses"
                         " external clock on active input channel");
    case ASE_NotPresent:
        throw EUnavailableError("CAsio::sampleRate",
                                "Cannot set the sampling rate"
                                ": No input/output present");
    }
    throw EUnexpectedError(__FILE__, __LINE__,
                           "CAsio::sampleRate",
                           "Cannot set the sampling rate"
                           ": ASIO driver returned unexpected error code",
                           this, aeErr);
}

long CAsio::ChannelGroup(long iChannelIndex, Direction adDirection) const
{
    ASIOChannelInfo aciInfo;
    GetChannelInfo(iChannelIndex, adDirection, &aciInfo);
    return aciInfo.channelGroup;
}

AnsiString CAsio::ChannelName(long iChannelIndex, Direction adDirection) const
{
    ASIOChannelInfo aciInfo;
    GetChannelInfo(iChannelIndex, adDirection, &aciInfo);
    return aciInfo.name;
}

long CAsio::ChannelDataType(long iChannelIndex, Direction adDirection) const
{
    ASIOChannelInfo aciInfo;
    GetChannelInfo(iChannelIndex, adDirection, &aciInfo);
    return aciInfo.type;
}

void CAsio::ShowControlPanel()
{
    AssertInitialized("Cannot show ASIO driver's control panel", INITIALIZED);
    ASIOError aeErr = ASIOControlPanel();
    switch (aeErr)
    {
    case ASE_OK:
    case ASE_SUCCESS:
        return;
    case ASE_NotPresent:
        throw EUnavailableError("CAsio::showControlPanel",
                                "No control panel available");
    }
    throw EUnexpectedError(__FILE__, __LINE__, "CAsio::showControlPanel",
                           "ASIO driver returned unexpected error code",
                           this, aeErr);
}

void CAsio::CreateBuffers(const std::vector<bool> & vbChannelMaskIn,
                          const std::vector<bool> & vbChannelMaskOut,
                          long iBufferSize,
                          long iProcQueueBuffers,
                          long iDoneQueueBuffers)
{
    // CreateBuffers calls the underlying Asio procedure and stores these
    // information member variables:
    // m_vvbActiveChMasks[INPUT]
    //     receives a normalized copy of vbChannelMaskIn
    // m_vvbActiveChMasks[OUTPUT]
    //     receives a normalized copy of vbChannelMaskOut
    // m_rgabiBufferInfos
    //     points to a newly allocated array of ASIOBufferInfo structures
    //     containing indices and buffer pointers for activated channels
    // m_vviActiveChIndices[INPUT]
    //     reveives a list of channel indices that are set in vbChannelMaskIn
    // m_vviActiveChIndices[OUTPUT]
    //     reveives a list of channel indices that are set in vbChannelMaskOut
    // m_vvsActiveChNames[INPUT]
    //     receives the list of names of the active input channels
    // m_vvsActiveChNames[OUTPUT]
    //     receives the list of names of the active output channels
    // m_vviActiveChDataTypes[INPUT]
    //     data type specifiers for active input channels (see ChannelDataType)
    // m_vviActiveChDataTypes[OUTPUT]
    //     data type specifiers for active output channels
    // m_vvfAciveChMaxValues[INPUT)
    //     maximum possible sample values of sound samples in active input channels
    // m_vvfAciveChMaxValues[OUTPUT)
    //     maximum possible sample values of sound samples in active output channels
    // m_vvfAciveChMinValues[INPUT)
    //     minimum possible sample values of sound samples in active input channels
    // m_vvfAciveChMinValues[OUTPUT)
    //     minimum possible sample values of sound samples in active output channels
    // m_nActiveChannelsIn
    //     receives the number of active input channels
    // m_nActiveChannelsOut
    //     receives the number of active output channels
    // m_pacbCallbacks
    //     The function pointers of the callbacks are stored here for the asio
    //     driver
    // m_pvvfBuffersIn
    //     points to a newly allocated vector of input sound data buffers
    // m_pvvfBuffersOut
    //     points to a newly allocated vector of output sound data buffers (one
    //     for each active output channel, data format is float)
    // m_iPreparedBuffersize
    //     is set to iBufferSize
    // m_nStopTimeout
    //     is set to the expected time (in milliseconds) period of the
    //     buffer switch callback (rounded up, plus 5 additional millisecond
    //     for safety).
    // m_psxSoundDataExchanger
    //     is allocated.
    // Finally, the state is changed to PREPARED using the SetState method.

    if (GetState() != INITIALIZED)
    {
        throw EStateError("CAsio::CreateBuffers",
                          "Asio driver is not in state INITIALIZED",
                          INITIALIZED, INITIALIZED, GetState());
    }

    // Check that vector parameters do not contain an activation bit
    // for some channel with an index >= number of hardware channels.
    // (vectors larger than number of hardware channels are ok as long
    // as all activation bits outside the range of hardware channels
    // are not selected).  Resize vectors to number of hardware
    // channels for easier access to acivation bits (vectors may be
    // smaller than number of hardware channels)
    const size_t nHardwareChannelsIn = (size_t)HardwareChannels(INPUT);
    const size_t nHardwareChannelsOut = (size_t)HardwareChannels(OUTPUT);
    m_vvbActiveChMasks.resize(2);
    m_vvbActiveChMasks[INPUT] =
        ResizeBitVector(vbChannelMaskIn, nHardwareChannelsIn);
    m_vvbActiveChMasks[OUTPUT] =
        ResizeBitVector(vbChannelMaskOut, nHardwareChannelsOut);

    /* removed check for support of buffersize: let ASIOCreateBuffers fail

    // Check that requested buffer size is supported
    std::vector<long> viBufferSizes = BufferSizes();
    if (std::find(viBufferSizes.begin(), viBufferSizes.end(), iBufferSize)
        == viBufferSizes.end())
    {
        throw EModeError("CAsio::CreateBuffers",
                         "Requested buffer size is not currently"
                         " supported by the ASIO driver.");
    }
    */

    // Check that the state of this object is consistent.
    if (m_rgabiBufferInfos != 0)
    {
        throw EInconsistentStateError(__FILE__, __LINE__,
                                      "CAsio::CreateBuffers",
                                      "m_rgabiBufferInfos",
                                      this,
                                      reinterpret_cast<size_t>(m_rgabiBufferInfos));
    }

    if (m_iPreparedBuffersize != 0)
    {
        throw EInconsistentStateError(__FILE__, __LINE__,
                                      "CAsio::CreateBuffers",
                                      "m_iPreparedBuffersize",
                                      this,
                                      m_iPreparedBuffersize);
    }
    if (m_nActiveChannelsIn != 0)
    {
        throw EInconsistentStateError(__FILE__, __LINE__,
                                      "CAsio::CreateBuffers",
                                      "m_nActiveChannelsIn",
                                      this,
                                      m_nActiveChannelsIn);
    }
    if (m_nActiveChannelsOut != 0)
    {
        throw EInconsistentStateError(__FILE__, __LINE__,
                                      "CAsio::CreateBuffers",
                                      "m_nActiveChannelsOut",
                                      this,
                                      m_nActiveChannelsOut);
    }
    if (sm_etExceptionTrigger ==
        CREATE_BUFFERS_THROWS_INCONSISTENT_STATE_ERROR)
    {
        throw EInconsistentStateError(__FILE__, __LINE__,
                                      "CAsio::CreateBuffers",
                                      "sm_etExceptionTrigger",
                                      this,
                                      sm_etExceptionTrigger);
    }
    // Collect activated channels.
    std::vector<ASIOBufferInfo> vabiBufferInfos;
    m_vviActiveChIndices.resize(2);
    m_vvsActiveChNames.resize(2);
    m_vviActiveChDataTypes.resize(2);
    m_vvfAciveChMaxValues.resize(2);
    m_vvfAciveChMinValues.resize(2);
    m_vviActiveChIndices[INPUT].clear();
    m_vvsActiveChNames[INPUT].clear();
    m_vviActiveChDataTypes[INPUT].clear();
    m_vvfAciveChMaxValues[INPUT].clear();
    m_vvfAciveChMinValues[INPUT].clear();
    size_t nChIdx;
    for (nChIdx = 0; nChIdx < vbChannelMaskIn.size(); ++nChIdx)
    {
        if (vbChannelMaskIn[nChIdx])
        {
            ASIOBufferInfo abiBufferInfo;
            abiBufferInfo.isInput = ASIOTrue;
            abiBufferInfo.channelNum = (long)nChIdx;
            abiBufferInfo.buffers[0] = 0;
            abiBufferInfo.buffers[1] = 0;
            vabiBufferInfos.push_back(abiBufferInfo);
            m_vviActiveChIndices[INPUT].push_back((long)nChIdx);
            m_vvsActiveChNames[INPUT].push_back(ChannelName((long)nChIdx, INPUT));
            long nDataType = ChannelDataType((long)nChIdx, INPUT);
            m_vviActiveChDataTypes[INPUT].push_back(nDataType);
            m_vvfAciveChMaxValues[INPUT].push_back(MaxFloatSample(nDataType));
            m_vvfAciveChMinValues[INPUT].push_back(MinFloatSample(nDataType));
            ++m_nActiveChannelsIn;
        }
    }
    m_vviActiveChIndices[OUTPUT].clear();
    m_vvsActiveChNames[OUTPUT].clear();
    m_vviActiveChDataTypes[OUTPUT].clear();
    m_vvfAciveChMaxValues[OUTPUT].clear();
    m_vvfAciveChMinValues[OUTPUT].clear();
    for (nChIdx = 0; nChIdx < vbChannelMaskOut.size(); ++nChIdx)
    {
        if (vbChannelMaskOut[nChIdx])
        {
            ASIOBufferInfo abiBufferInfo;
            abiBufferInfo.isInput = ASIOFalse;
            abiBufferInfo.channelNum = (long)nChIdx;
            abiBufferInfo.buffers[0] = 0;
            abiBufferInfo.buffers[1] = 0;
            vabiBufferInfos.push_back(abiBufferInfo);
            m_vviActiveChIndices[OUTPUT].push_back((long)nChIdx);
            m_vvsActiveChNames[OUTPUT].push_back(ChannelName((long)nChIdx, OUTPUT));
            long nDataType = (long)ChannelDataType((long)nChIdx, OUTPUT);
            m_vviActiveChDataTypes[OUTPUT].push_back(nDataType);
            m_vvfAciveChMaxValues[OUTPUT].push_back(MaxFloatSample(nDataType));
            m_vvfAciveChMinValues[OUTPUT].push_back(MinFloatSample(nDataType));
            ++m_nActiveChannelsOut;
        }
    }
    m_nStopTimeout = ComputeStopTimeout((unsigned int)iBufferSize, SampleRate(), 5);

    m_psxSoundDataExchanger = new SoundDataExchanger(
            (unsigned int)iProcQueueBuffers, (unsigned int)iDoneQueueBuffers,
            (unsigned int)m_nActiveChannelsIn, (unsigned int)m_nActiveChannelsOut, (unsigned int)iBufferSize);
    ++sm_nObjects;

    m_pacbCallbacks->bufferSwitch = &SoundDataExchanger::StaticBufferSwitch;
    m_pacbCallbacks->bufferSwitchTimeInfo = 0;
    m_pacbCallbacks->sampleRateDidChange = &CAsio::StaticSampleRateDidChange;
    m_pacbCallbacks->asioMessage = &CAsio::StaticAsioMessage;

    ASIOError aeErr;
    // switch off clang warning: not all enum values handled by pupose
    #pragma clang diagnostic push
    #pragma clang diagnostic ignored "-Wswitch-enum"
    switch (sm_etExceptionTrigger)
    {
    case CREATE_BUFFERS_THROWS_NO_MEMORY_ERROR:
        aeErr = ASE_NoMemory;
        break;
    case CREATE_BUFFERS_THROWS_UNAVAILABLE_ERROR:
        aeErr = ASE_NotPresent;
        break;
    case CREATE_BUFFERS_THROWS_UNEXPECTED_ERROR_MODE:
        aeErr = ASE_InvalidMode;
        break;
    default:
        aeErr = ASIOCreateBuffers(&vabiBufferInfos[0],
                                  (long)vabiBufferInfos.size(),
                                  iBufferSize,
                                  m_pacbCallbacks);
    }
    #pragma clang diagnostic pop
    try
    {
        switch (aeErr)
        {
        case ASE_NoMemory:
            throw ENoMemoryError("CAsio::createBuffers",
                                 "Driver could not allocate sound buffers");
        case ASE_NotPresent:
            throw EUnavailableError("CAsio::createBuffers",
                                    "No input/output present error, cannot"
                                    " prepare ASIO driver's sound buffers");
        case ASE_InvalidMode:
            // We checked the settings before calling ASIOCreateBuffers,
            // therefore, this is unexpected
            throw EUnexpectedError(__FILE__, __LINE__, "CAsio::createBuffers",
                                   "Error code indicates 'invalid mode',"
                                   " although the settings were checked before"
                                   " calling into the driver",
                                   this, aeErr);
        case ASE_OK:
            break;
        default:
            throw EUnexpectedError(__FILE__, __LINE__, "CAsio::createBuffers",
                                   "Driver returned unexpected error code",
                                   this, aeErr);
        }
    }
    catch (EAsioError & )
    {
        // cleanup state
        m_nActiveChannelsOut = m_nActiveChannelsIn = 0;
        m_vviActiveChIndices.clear();
        m_vvsActiveChNames.clear();
        m_vviActiveChDataTypes.clear();
        delete m_psxSoundDataExchanger;
        m_psxSoundDataExchanger = 0;
        --sm_nObjects;
        throw;
    }

    m_rgabiBufferInfos = new ASIOBufferInfo[vabiBufferInfos.size()];
    std::copy(vabiBufferInfos.begin(),
              vabiBufferInfos.end(),
              m_rgabiBufferInfos);
    ++CAsio::sm_nObjects;
    m_iPreparedBuffersize = iBufferSize;
    m_bDataNeedsRealtimeProcessing = false; // clear any old values
    SetState(PREPARED);
}


void CAsio::DisposeBuffers()
{
    AssertInitialized("Cannot release ASIO driver's sound buffers", PREPARED);
    if (GetState() != PREPARED)
    {
        // stop and wait until processing really is stopped
        StopAndWait();
    }

    ASIOError aeErr = ASIODisposeBuffers();
    if (aeErr == ASE_OK)
    {
        // switch off clang warning: not all enum values handled by pupose
        #pragma clang diagnostic push
        #pragma clang diagnostic ignored "-Wswitch"
        switch (sm_etExceptionTrigger)
        {
        case DISPOSE_BUFFERS_TRIGGERS_UNAVAILABLE_WARNING:
            aeErr = ASE_NotPresent;
            break;
        case DISPOSE_BUFFERS_TRIGGERS_MODE_WARNING:
            aeErr = ASE_InvalidMode;
            break;
        case DISPOSE_BUFFERS_TRIGGERS_UNEXPECTED_WARNING:
            aeErr = ASIO_UNEXPECTED;
            break;
        }
        #pragma clang diagnostic push
    }
    switch (aeErr) {
    case ASE_NotPresent:
        TriggerWarning("CAsio::DisposeBuffers: "
                       "No input/output present, cannot dispose buffers");
        break;
    case ASE_InvalidMode:
        TriggerWarning("CAsio::DisposeBuffers: "
                       "ASIODisposeBuffers returned ASE_InvalidMode"
                       " although the driver was in prepared state"
                       " before the call");
        break;
    case ASE_OK:
        break;
    default:
        TriggerWarning("CAsio::DisposeBuffers: "
                       "ASIODisposeBuffers returned unexpected Error code");
    }
    if (m_psxSoundDataExchanger)
    {
        delete m_psxSoundDataExchanger;
        --CAsio::sm_nObjects;
        m_psxSoundDataExchanger = 0;
    }
    if (m_rgabiBufferInfos)
    {
        delete [] m_rgabiBufferInfos;
        --CAsio::sm_nObjects;
        m_rgabiBufferInfos = 0;
    }
    m_iPreparedBuffersize = 0;
    m_nActiveChannelsIn = 0;
    m_nActiveChannelsOut = 0;
    m_vviActiveChIndices.clear();
    m_vvsActiveChNames.clear();
    m_vviActiveChDataTypes.clear();
    SetState(INITIALIZED);
}

size_t CAsio::ActiveChannels(Direction adDirection) const
{
    // Test should check if this is the same as
    // m_vviActiveChIndices[INPUT].size() etc.
    if (adDirection == INPUT)
    {
        return m_nActiveChannelsIn;
    }
    return m_nActiveChannelsOut;
}

long CAsio::ActiveChannelDataType(Direction adDirection,
                                  size_t nChannel) const
{
    AssertInitialized("No active channels are prepared", PREPARED);
    if (nChannel >= ActiveChannels(adDirection))
    {
        throw EIndexError("CAsio::ActiveChannelDataType",
                          "Invalid index for active channel",
                          0, (long)ActiveChannels(adDirection), (long)nChannel);
    }
    return m_vviActiveChDataTypes[adDirection][nChannel];
}

void CAsio::Start()
{
    // If the Sound buffers are prepared, and the undelying Asio start function
    // is successful, sets the state to RUNNING and clears the m_hStoppedEvent.
    // The Stop process indicator, m_bStopping is also cleared.

    State asState = GetState();
    if (asState != PREPARED)
    {
        throw EStateError("CAsio::start",
                          "Driver must be in state PREPARED when starting",
                          PREPARED, PREPARED, asState);
    }

    // These flags are flipped at different times during stopping. Reset them.
    m_bStopping = false;
    m_bDoneLoopWaitsWhenDoneQueuesEmpty = true;

    PrefillPlaybackBuffers();

    ASIOError aeErr;
    // switch off clang warning: not all enum values handled by pupose
    #pragma clang diagnostic push
    #pragma clang diagnostic ignored "-Wswitch-enum"
    switch (sm_etExceptionTrigger)
    {
    case START_THROWS_UNAVAILABLE_ERROR:
        aeErr = ASE_NotPresent;
        break;
    case START_THROWS_UNAVAILABLE_ERROR_HW:
        aeErr = ASE_HWMalfunction;
        break;
    case START_THROWS_UNEXPECTED_ERROR:
        aeErr = ASIO_UNEXPECTED;
        break;
    default:
        ResetEvent(m_phProcEvents[PROC_STOP]);
        ResetEvent(m_phDoneEvents[DONE_STOP]);
        SetEvent(m_phProcEvents[PROC_START]);
        SetEvent(m_phDoneEvents[DONE_START]);
        SetState(RUNNING);
        aeErr = ASIOStart();
    }
    #pragma clang diagnostic pop
    try
    {
        switch (aeErr)
        {
        case ASE_NotPresent:
            throw EUnavailableError("CAsio::Start", "No input/output present");
        case ASE_HWMalfunction:
            throw EUnavailableError("CAsio::Start", "Hardware malfunction");
        case ASE_OK:
            break;
        default:
            throw EUnexpectedError(__FILE__, __LINE__, "CAsio::Start",
                                   "Driver returned unexpected error code",
                                   this, aeErr);
        }
    }
    catch (...)
    {
        // reset state, terminate inner loops, clear data from prefill
        SetState(PREPARED);
        SetEvent(m_phProcEvents[PROC_STOP]);
        SetEvent(m_phDoneEvents[DONE_STOP]);
        GetSoundDataExchanger()->ClearQueues();
        throw;
    }
    ResetEvent(m_hStoppedEvent);
    SetEvent(m_phCbEvents[OBSERVE]);
}

void CAsio::PrefillPlaybackBuffers()
{
    if (GetSoundDataExchanger()->IsRealTime() == false)
    { // Prefilling only needed for non-realtime processing
       SoundData sdFakeCapture;
       sdFakeCapture.Reinitialize((size_t)m_nActiveChannelsIn, (size_t)m_iPreparedBuffersize);
       SoundData sdPlayback;
       sdPlayback.Reinitialize((size_t)m_nActiveChannelsOut, (size_t)m_iPreparedBuffersize);

       while (GetSoundDataExchanger()->ProcPlaybackNumEmptyBuffers() > 0U
               && sdPlayback.m_bIsLast == false)
        {
            sdFakeCapture.Clear();
            sdPlayback.Clear();
            Process(sdFakeCapture, sdPlayback, 0, true);
            GetSoundDataExchanger()->Prefill(sdPlayback);
        }
    }
}

void CAsio::Stop()
{
    if (GetState() == RUNNING && m_psxSoundDataExchanger == 0)
    {
        // We cannot really be running if there is no SoundDataExchanger.
        // Assume that IO has stopped.
        SetEvent(m_phDoneEvents[DONE_STOP]);
        SetState(PREPARED);
        m_bStopping = false;
        SetEvent(m_hStoppedEvent);
        return;
    }

    // Some devices repeat the last few output buffers of a stopped device
    // ad infinitum. This has been observed in early drivers of the RME
    // multiface soundcard. Other soundcards like the RME DIGI 96 PAD 8 card
    // will stop sounding when the device is stopped, but replay the last two
    // output buffers while the device is used later to play sound through
    // different channels using the MME interface.
    //
    // We solve this problem by ensuring that the last two (or n)
    // buffers contain only silence. Because of this, and because it
    // should be permitted that the stop trigger comes from inside the
    // signal processing thread, stopping the device had to become an
    // asynchronous operation. After the application calls CAsio::Stop,
    // the device will continue to run and this wrapper will output
    // silence for two (or n) more buffer switches, before the device
    // is actually closed. The application can observe the closure of
    // the device by waiting on the Windows Event object m_hStoppedEvent.
    // The number of buffer switches waited is detemined by m_nStopSwitches.
    if (GetState() == RUNNING)
    {
        SetEvent(m_phStopEvents[STOP_BEGIN]);
    }
}

void CAsio::StopAndWait()
{
    Stop();
    WaitForSingleObject(m_hStoppedEvent, INFINITE);
}

void CAsio::Process(SoundData & sdBuffersIn,
                    SoundData & sdBuffersOut,
                    unsigned nBuffersInWaiting,
                    bool bPreloading)
{
    m_nTestingProcessChannelsIn = sdBuffersIn.m_vvfData.size();
    m_nTestingProcessChannelsOut = sdBuffersOut.m_vvfData.size();
    if (m_nTestingProcessChannelsIn != 0)
    {
        m_nTestingProcessFramesIn = sdBuffersIn.m_vvfData[0].size();
    }
    else
    {
        m_nTestingProcessFramesIn = 0;
    }
    if (m_nTestingProcessChannelsOut != 0)
    {
        m_nTestingProcessFramesOut = sdBuffersOut.m_vvfData[0].size();
    }
    else
    {
        m_nTestingProcessFramesOut = 0;
    }
    m_nTestingProcessBuffersInWaiting = nBuffersInWaiting;
    m_bTestingProcessPreloading = bPreloading;
}

void CAsio::SignalXrun(XrunType xtXrunType)
{
    // We have an XRun. Signal the appropriate XRun event
    SetEvent(m_phCbEvents[XRUN + xtXrunType]);
}

bool CAsio::CheckForRealtimeXrun()
{
    if (m_bDataNeedsRealtimeProcessing == false)
    {
        if (TryEnterCriticalSection(&m_csLock))
        {
            m_bDataNeedsRealtimeProcessing = true;
            LeaveCriticalSection(&m_csLock);
            return false;
        }
        if (m_bStopping)
        {
            /// Xruns during stopping do not count
            return false;
        }
    }
    SignalXrun(XR_RT);
    return true;
}

void CAsio::DeallocateSDK()
{
    // clean-up: enable creation of new CAsio instance
    InterlockedCompareExchangePointer(reinterpret_cast<void**>(&asioDrivers),
                                      0, m_padAsioDrivers);
    delete m_padAsioDrivers;
    --CAsio::sm_nObjects;
    m_padAsioDrivers = 0;
    delete m_padiDriverInfo;
    --CAsio::sm_nObjects;
    m_padiDriverInfo = 0;
    delete m_pacbCallbacks;
    --CAsio::sm_nObjects;
    m_pacbCallbacks = 0;
    InterlockedCompareExchangePointer(reinterpret_cast<void**>(&sm_pcaInstance),
                                      0, this);
}

void CAsio::InitEvents()
{
    /// Clear memory to be able to detect uninitialized events
    memset(m_rghEvents, 0, sizeof(m_rghEvents));
    m_hStoppedEvent = NULL;

    /// Initialize Event pointers
    m_phCbEvents = m_rghEvents;
    m_phStopEvents = m_phCbEvents + CB_EVENTS;
    m_phProcEvents = m_phStopEvents + STOP_EVENTS;
    m_phDoneEvents = m_phProcEvents + PROC_EVENTS;

    // Index of next uninitialized Event in array m_rghEvents.
    int nEventIndex;
    // Create Event in manual-reset mode or not
    bool bManualReset;
    // Create all events in one loop.
    for (nEventIndex = 0;
         nEventIndex < (CB_EVENTS + STOP_EVENTS + PROC_EVENTS + DONE_EVENTS);
         ++nEventIndex)
    {
        // First event (QUIT) for each thread is alays manual-reset,
        // other events are auto-reset.
        bManualReset = (nEventIndex == (m_phCbEvents - m_rghEvents) ||
                        nEventIndex == (m_phStopEvents - m_rghEvents) ||
                        nEventIndex == (m_phProcEvents - m_rghEvents) ||
                        nEventIndex == (m_phDoneEvents - m_rghEvents));

        if (sm_etExceptionTrigger != INIT_EVENTS_THROWS_WIN32_ERROR_IN_LOOP)
        {
            m_rghEvents[nEventIndex] = CreateEvent(0, bManualReset, false, 0);
        }
        if (m_rghEvents[nEventIndex] == 0)  // Error during Event creation
        {
            throw EWin32Error("CAsio::InitEvents",
                              "Could not allocate Event Object",
                              (int)GetLastError());
        }
        ++CAsio::sm_nEvents;
    }

    if (sm_etExceptionTrigger != INIT_EVENTS_THROWS_WIN32_ERROR_AFTER_LOOP)
    {
        m_hStoppedEvent = CreateEvent(0, true, true, 0);
    }
    if (m_hStoppedEvent == 0)
    {
        throw EWin32Error("CAsio::InitEvents",
                          "Could not allocate m_hStoppedEvent",
                          (int)GetLastError());
    }
    ++CAsio::sm_nEvents;
}

void CAsio::DestroyEvents()
{
    // close all allocated Events
    int nCloseIndex;
    for (nCloseIndex = 0;
         nCloseIndex < (CB_EVENTS + STOP_EVENTS + PROC_EVENTS + DONE_EVENTS);
         ++nCloseIndex)
    {
        if (m_rghEvents[nCloseIndex])
        {
            CloseHandle(m_rghEvents[nCloseIndex]);
            --CAsio::sm_nEvents;
            m_rghEvents[nCloseIndex] = 0;
        }
    }
    if (m_hStoppedEvent)
    {
        CloseHandle(m_hStoppedEvent);
        --CAsio::sm_nEvents;
        m_hStoppedEvent = 0;
    }
}

void CAsio::StartThreads()
{
    m_hCbThread = m_hStopThread = m_hProcThread = m_hDoneThread = NULL;

    // Reset all events
    int nEventIndex;
    for (nEventIndex = 0;
         nEventIndex < (CB_EVENTS + STOP_EVENTS + PROC_EVENTS + DONE_EVENTS);
         ++nEventIndex)
    {
        ResetEvent(m_rghEvents[nEventIndex]);
    }

    // Start threads
    m_hCbThread =
        CreateThread(0,0, CAsio::CbStartFunction, this, 0,0);
    if (m_hCbThread == 0)
    {
        throw EWin32Error("CAsio::StartThread", "Cannot create thread",
                          (int)GetLastError());
    }
    ++sm_nThreads;

    // Test exception
    if (sm_etExceptionTrigger != START_THREADS_THROWS_WIN32_ERROR)
    {
        m_hStopThread = CreateThread(0,0, CAsio::StopStartFunction, this, 0,0);
    }
    if (m_hStopThread == 0)
    {
        DWORD nErr = GetLastError();
        StopThreads();
        throw EWin32Error("CAsio::StartThreads", "Cannot create thread", (int)nErr);
    }
    ++sm_nThreads;

    m_hProcThread =
        CreateThread(0,0, CAsio::ProcStartFunction, this, 0,0);
    if (m_hProcThread == 0)
    {
        DWORD nErr = GetLastError();
        StopThreads();
        throw EWin32Error("CAsio::StartThreads",
                          "Cannot create processing thread", (int)nErr);
    }
    ++sm_nThreads;

    m_hDoneThread =
        CreateThread(0,0, CAsio::DoneStartFunction, this, 0,0);
    if (m_hDoneThread == 0)
    {
        DWORD nErr = GetLastError();
        StopThreads();
        throw EWin32Error("CAsio::StartThreads",
                          "Cannot create \"done\" thread", (int)nErr);
    }
    ++sm_nThreads;
}

void CAsio::StopThreads()
{
    // signal all threads to terminate
    SetEvent(m_phDoneEvents[QUIT]);
    SetEvent(m_phProcEvents[QUIT]);
    SetEvent(m_phStopEvents[QUIT]);
    SetEvent(m_phCbEvents[QUIT]);
    if (m_hDoneThread)
    {
        WaitForSingleObject(m_hDoneThread, INFINITE);
        CloseHandle(m_hDoneThread);
        --sm_nThreads;
        m_hDoneThread = 0;
    }
    if (m_hProcThread)
    {
        WaitForSingleObject(m_hProcThread, INFINITE);
        CloseHandle(m_hProcThread);
        --sm_nThreads;
        m_hProcThread = 0;
    }
    if (m_hStopThread)
    {
        WaitForSingleObject(m_hStopThread, INFINITE);
        CloseHandle(m_hStopThread);
        --sm_nThreads;
        m_hStopThread = 0;
    }
    if (m_hCbThread)
    {
        WaitForSingleObject(m_hCbThread, INFINITE);
        CloseHandle(m_hCbThread);
        --sm_nThreads;
        m_hCbThread = 0;
    }
}

DWORD WINAPI CAsio::CbStartFunction(void * pvCAsio)
{
    // Flag only used in unit test
    InterlockedIncrement(&sm_anThreadStartFunctionFlags[CB_THREAD]);

    Asio::CAsio * pcaCAsio = static_cast<Asio::CAsio *>(pvCAsio);
    pcaCAsio->CbMain();

    InterlockedDecrement(&sm_anThreadStartFunctionFlags[CB_THREAD]);
    return 0;
}

void CAsio::CbMain()
{
    // Main loop of the callback thread.
    //
    // The callback thread executes callbacks triggered from the
    // signal processing thread asynchroneously, so they do not have to be
    // executed in the high-priority signal processing thread and delay
    // the real purpose of that thread, signal processing. These callbacks are:
    // OnFatalError, OnXrun.  The signal processing thread signals the events
    // m_phCbEvents[PROC_ERROR] or m_phCbEvents[XRUN+xtXrunType], the "Callback
    // thread" wakes up on these events and executes the appropriate callback.
    //
    // The callback also acts as a watchdog while sound i/o is active,
    // and detects situations where the Asio driver is no longer
    // triggering BufferSwitch callbacks. When this situation is
    // detected, the callback OnHang is executed in this callback
    // thread.
    //
    // The event m_phCbEvents[OBSERVE] is used to switch the watchdog
    // functionality on (event set by the Start method) and off (event
    // set in the stop helper thread).
    //
    // The event m_phCbEvents[QUIT] is set from the StopThreads
    // method.  The callback thread terminates when it receives this
    // event.

    // Flag only used in unit test
    InterlockedIncrement(&sm_anThreadMainFunctionFlags[CB_THREAD]);


    for(;;)
    {
        DWORD nTimeout = INFINITE;
        size_t nOldProcBufferSwitches = m_nProcBufferSwitches;
        if (GetState() == RUNNING)
        {
            nTimeout = m_nWatchdogTimeout;
        }
        DWORD nWaitResult =
            WaitForMultipleObjects(CB_EVENTS, m_phCbEvents, false, nTimeout);

        // In case multiple events in the m_phCbEvents array were active,
        // the return value of WFMO indicates only the active event that has
        // the lowest index in that array. Because of that, the events are
        // intentionally sorted from most important to least important.
        // The activated events that are masked by another event with lower
        // array index will be signalled by WFMO in a later iteration.
        // A CPPUNIT test case (test_WFMO_postpones) ensures that shadowed
        // events are signalled later.
        switch (nWaitResult)
        {
        case (WAIT_OBJECT_0 + DWORD(QUIT)):
            // terminate this thread.
            InterlockedDecrement(&sm_anThreadMainFunctionFlags[CB_THREAD]);
            return;
        case WAIT_TIMEOUT:
            // sound i/o watchdog
            ++m_nWatchdogWakeups;
            if (GetState() == RUNNING &&
                nOldProcBufferSwitches == m_nProcBufferSwitches)
            {
                try
                {
                    OnHang();
                }
                catch(...)
                {   // discard all exceptions raised in callback
                }
            }
            break;
        case (WAIT_OBJECT_0 + PROC_ERROR):
            // An exception was caught in signal processing thread
            try
            {
                OnFatalError();
            }
            catch(...)
            {   // discard all exceptions raised in callback
            }
            break;
        case (WAIT_OBJECT_0 + XRUN_PROC):
        case (WAIT_OBJECT_0 + XRUN_DONE):
        case (WAIT_OBJECT_0 + XRUN_RT):
            try
            {
                OnXrun(XrunType(nWaitResult - WAIT_OBJECT_0 - XRUN));
            }
            catch(...)
            {   // discard all exceptions raised in callback
            }
            break;
        case (WAIT_OBJECT_0 + OBSERVE):
            // Set timeout next time as apropriate.
            // This event is fired to switch the timeout for
            // WaitForMultipleObjects form INFINITE to
            // m_nWatchdogTimeout if state state changed to RUNNING,
            // or vice versa if state changed _from_ running
            break;
        }
    }
}

DWORD WINAPI CAsio::StopStartFunction(void * pvCAsio)
{
    // Flag only used in unit test
    InterlockedIncrement(&sm_anThreadStartFunctionFlags[STOP_THREAD]);

    Asio::CAsio * pcaCAsio = static_cast<Asio::CAsio *>(pvCAsio);
    pcaCAsio->StopMain();

    InterlockedDecrement(&sm_anThreadStartFunctionFlags[STOP_THREAD]);

    return 0;
}

void CAsio::StopMain()
{
    // Main loop of the stop helper thread.
    //
    // The stop helper thread's main purpose is to help stopping the
    // audio device.  Stopping the audio device has to be
    // asynchroneous (See comment in Stop()). When a stop request is
    // made by the application by executing the Stop() method from any
    // thread, the Stop method sets the m_phStopEvents[STOP_BEGIN]
    // event.  When the "stop helper thread" wakes on the STOP_BEGIN event, it
    // sets the flag m_bStopping to true, if it is not already set.
    // When this flag is set to true, then the casio will
    // output silent buffers from the driver notification thread, and set the
    // STOP_CONTINUE Event when it does so (see #SilenceOutputsIfStopping).
    // When the "stop helper thread" wakes on the
    // STOP_CONTINUE event and m_bStopping is true, this indicates
    // that a buffer of silence has been handed over to the driver.
    // This thread counts how many silence buffers have been output,
    // and stops the device for real when the requested number of
    // silence output buffers has been reached.  After the device has
    // been stopped for real, the done thread is notified that no further
    // data will arrive. The done thread will finish processing all remaining
    // data and call OnDoneLoopStopped. The stop thread
    // waits for the done thread to finish, and thereafter sets
    // the Event m_hStoppedEvent to signalled.
    // The application can observe this event to know when the stopping
    // has finished.  It is possible that the Asio driver has
    // encountered an internal error and no longer calls the
    // BufferSwitch callback. To ensure that the stopping procedure
    // terminates in this case, timeouts are used by this thread while
    // stopping.  These timeouts are considerably smaller than the
    // timeouts used by the callback thread when it acts as watchdog.
    // The CreateBuffers method sets these timeouts to the duration of
    // one buffer switch + 5 milliseconds.
    //
    // When the event m_phStopEvents[QUIT] is set from the StopThreads
    // method then this thread terminates.

    // Flag only used in unit test
    InterlockedIncrement(&sm_anThreadMainFunctionFlags[STOP_THREAD]);

    for(;;)
    {
        DWORD nTimeout = INFINITE;
        if (m_bStopping && GetState() == RUNNING)
        {   // during stopping, do not wait longer than the length of
            // a single buffer switch + uncertainty grace time.
            nTimeout = m_nStopTimeout;
        }
        DWORD nWaitResult =
            WaitForMultipleObjects(STOP_EVENTS, m_phStopEvents,
                                   false, nTimeout);
        // In case multiple events in the m_phStopEvents array were active,
        // the return value of WFMO indicates only the active event that has
        // the lowest index in that array. Because of that, the events are
        // intentionally sorted from most important to least important.
        // The activated events that are masked by another event with lower
        // array index will be signalled by WFMO in a later iteration.
        // A CPPUNIT test case (test_WFMO_postpones) ensures that shadowed
        // events are signalled later.
        // switch off clang warning: fallthrough by purpose
        #pragma clang diagnostic push
        #pragma clang diagnostic ignored "-Wimplicit-fallthrough"
        switch (nWaitResult)
        {
        case (WAIT_OBJECT_0 + DWORD(QUIT)):
            // thread exiting.
            InterlockedDecrement(&sm_anThreadMainFunctionFlags[STOP_THREAD]);
            return;
        case WAIT_TIMEOUT: // next buffer switch did not occur in time
            ++m_nStopTimeoutsWaited;
            // FALL THROUGH
        case (WAIT_OBJECT_0 + STOP_CONTINUE): // next silence buffer is written
            // continue stop procedure
            if (GetState() != RUNNING || m_bStopping == false)
            {
                break;
            }
            ++m_nStopSwitchesWaited;
            if (m_nStopSwitchesWaited >= m_nStopSwitches)
            {
                ASIOError aeErr = ASIOStop();
                switch (aeErr)
                {
                case ASE_NotPresent:
                    // since unloading the driver should always succeed,
                    // this is only a warning
                    TriggerWarning("ASIOStop: No input/output present.");
                    break;
                case ASE_OK:
                    break;
                default:
                    // since unloading the driver should always succeed,
                    // this is only a warning
                    TriggerWarning("ASIOStop: unexpected error code");
                }

                // The "done" thread is still waiting for more data to come.
                // Wake it up and inform it that no more data is coming.
                SetEvent(m_phDoneEvents[DONE_STOP]);

                // Just in the unlikely case that there may be an xrun so severe
                // that #Process is still executing even after the stop
                // procedure has finished, perform next step (clearing sound
                // data queues) only after aquiring the lock that is held during
                // Processing.
                EnterCriticalSection(&m_csLock);
                if (GetSoundDataExchanger()) // Needed to protect against faked
                {                            // states in tests.
                    // will wait for Done thread if that is still active
                    GetSoundDataExchanger()->ClearQueues();
                }
                LeaveCriticalSection(&m_csLock);
                SetState(PREPARED);
                m_bStopping = false;
                SetEvent(m_hStoppedEvent);
                SetEvent(m_phCbEvents[OBSERVE]);
            }
            break;
        case (WAIT_OBJECT_0 + STOP_BEGIN):
            // initiate stop procedure
            if (GetState() == RUNNING && m_bStopping == false)
            {
                m_bStopping = true;
                m_nStopSwitchesWaited = m_nStopTimeoutsWaited = 0;
                SetEvent(m_phProcEvents[PROC_STOP]);
                // At this point, i/o activity caused by the last bufferswitch
                // may still be in progress, which can result in more data for
                // the "done" thread. Therefore, DONE_STOP will be signalled at
                // the end of the stop procedure, not now.
            }
            break;
        }
        #pragma clang diagnostic pop
    }
}

DWORD WINAPI CAsio::ProcStartFunction(void * pvCAsio)
{
    // Flag only used in unit test
    InterlockedIncrement(&sm_anThreadStartFunctionFlags[PROC_THREAD]);

    Asio::CAsio * pcaCAsio = static_cast<Asio::CAsio *>(pvCAsio);
    pcaCAsio->ProcMain();
    InterlockedDecrement(&sm_anThreadStartFunctionFlags[PROC_THREAD]);
    return 0;
}

void CAsio::ProcMain()
{
    // Flag only used in unit test
    InterlockedIncrement(&sm_anThreadMainFunctionFlags[PROC_THREAD]);
    int iOrigThreadPriority;
    for(;;) // main loop
    {
        DWORD nWaitResult =
            WaitForMultipleObjects(PROC_EVENTS_WITHOUT_STOP, m_phProcEvents,
                                   false, INFINITE);
        switch (nWaitResult)
        {
        case (WAIT_OBJECT_0 + DWORD(QUIT)):
            InterlockedDecrement(&sm_anThreadMainFunctionFlags[PROC_THREAD]);
            return;
        case (WAIT_OBJECT_0 + DWORD(PROC_START)):
            iOrigThreadPriority = GetThreadPriority(GetCurrentThread());
            SetThreadPriority(GetCurrentThread(),
                              THREAD_PRIORITY_TIME_CRITICAL);
            ProcLoop();
            SetThreadPriority(GetCurrentThread(), iOrigThreadPriority);
        }
    }
}

void CAsio::ProcLoop()
{
    for (;;) // processing loop
    {
        try // protect processing loop
        {
            // Check if the loop has to terminate.
            if (ShouldProcLoopTerminate())
            {
                return; // no more signal processing if quitting or stopping.
            }

            unsigned nBuffersWaiting = BuffersWaitingForProcLoop();
            if (nBuffersWaiting == 0)
            {   /// Quit or Stop event was set
                return;
            }
            EnterCriticalSection(&m_csLock);
            try // protect critical section
            {
                HandleProcLoopData(nBuffersWaiting);
            }
            catch (...) // should never happen
            {
                SetEvent(m_phCbEvents[PROC_ERROR]);
            }
            LeaveCriticalSection(&m_csLock);
        }
        catch (...) // should never happen
        {
            SetEvent(m_phCbEvents[PROC_ERROR]);
        }
    }
}

bool CAsio::ShouldProcLoopTerminate()
{
    m_bShouldProcLoopTerminateCalled = true;
    return (WaitForSingleObject(m_phProcEvents[QUIT], 0) == WAIT_OBJECT_0) ||
        (WaitForSingleObject(m_phProcEvents[PROC_STOP], 0) == WAIT_OBJECT_0);
}

unsigned CAsio::BuffersWaitingForProcLoop()
{
    m_bBuffersWaitingForProcLoopCalled = true;
    unsigned nBuffersWaiting = GetSoundDataExchanger()->ProcNumClientBuffers();
    if (nBuffersWaiting == 0)
    {
        nBuffersWaiting = WaitForProc();
        /// may be 0 if Quit or Stop event was set
    }
    return nBuffersWaiting;
}

void CAsio::HandleProcLoopData(unsigned nBuffersWaiting)
{
    m_bHandleProcLoopDataCalled = true;
    SoundData * psdCapture = 0;
    SoundData * psdPlayback = 0;
    GetSoundDataExchanger()->GetProcBuffers(&psdCapture, &psdPlayback);
    try // protect Process callback
    {
        Process(*psdCapture, *psdPlayback, nBuffersWaiting, false);
    }
    catch(...)
    {
        psdPlayback->Clear();
        SetEvent(m_phCbEvents[PROC_ERROR]);
    }
    GetSoundDataExchanger()->ProcPut();
    if (GetSoundDataExchanger()->IsRealTime())
    {
        GetSoundDataExchanger()->HandlePlaybackData();
        m_bDataNeedsRealtimeProcessing = false;
    }
    ++m_nProcBufferSwitches;
}

unsigned CAsio::WaitForProc()
{
    enum LocalEvents {L_QUIT, L_CAPTURE, L_PLAYBACK, L_STOP, L_EVENTS};
    HANDLE rghEvents[L_EVENTS];
    rghEvents[L_QUIT] = m_phProcEvents[QUIT];
    rghEvents[L_CAPTURE] =
        GetSoundDataExchanger()->GetProcCaptureDataEvent();
    rghEvents[L_PLAYBACK] =
        GetSoundDataExchanger()->GetProcPlaybackSpaceEvent();
    rghEvents[L_STOP] = m_phProcEvents[PROC_STOP];
    for (;;)
    {
        unsigned nNumBuffersWaiting;
        DWORD nWaitResult =
            WaitForMultipleObjects(L_EVENTS, rghEvents, false, INFINITE);
        switch (nWaitResult)
        {
        case DWORD(WAIT_OBJECT_0 + L_STOP):
        case DWORD(WAIT_OBJECT_0 + L_QUIT):
            return 0;
        case DWORD(WAIT_OBJECT_0 + L_CAPTURE):
        case DWORD(WAIT_OBJECT_0 + L_PLAYBACK):
            // When waiting for the next processing cycle, the Capture event
            // is signalled when data becomes available in the capture queue,
            // and the playback event is signalled when space
            // becomes available in the playback queue.
            // Both have to happen before ProcNumClientBuffers returns >0,
            // but order does not matter. Therefore, we wait for each event
            // but check after occurrence if the next processing cycle can
            // progress already.
            nNumBuffersWaiting =
                GetSoundDataExchanger()->ProcNumClientBuffers();
            if (nNumBuffersWaiting > 0U)
            {
                return nNumBuffersWaiting;
            }
        }
    }
}

DWORD WINAPI CAsio::DoneStartFunction(void * pvCAsio)
{
    // Flag only used in unit test
    InterlockedIncrement(&sm_anThreadStartFunctionFlags[DONE_THREAD]);

    Asio::CAsio * pcaCAsio = static_cast<Asio::CAsio *>(pvCAsio);
    pcaCAsio->DoneMain();

    InterlockedDecrement(&sm_anThreadStartFunctionFlags[DONE_THREAD]);
    return 0;
}

void CAsio::DoneMain()
{
    // Flag only used in unit test
    InterlockedIncrement(&sm_anThreadMainFunctionFlags[DONE_THREAD]);
    for(;;) // main loop
    {
        DWORD nWaitResult =
            WaitForMultipleObjects(DONE_EVENTS_WITHOUT_STOP, m_phDoneEvents,
                                   false, INFINITE);
        switch (nWaitResult)
        {
        case (WAIT_OBJECT_0 + DWORD(QUIT)):
            InterlockedDecrement(&sm_anThreadMainFunctionFlags[DONE_THREAD]);
            return;
        case (WAIT_OBJECT_0 + DWORD(DONE_START)):
            m_bDoneLoopActive = true;
            try
            {
                DoneLoop();
            }
            catch(...)
            {
                SetEvent(m_phCbEvents[DONE_ERROR]);
            }
            m_bDoneLoopActive = false;
            OnDoneLoopStopped();
        }
    }
}

bool CAsio::IsDoneLoopActive()
{
    return m_bDoneLoopActive;
}

void CAsio::DoneLoop()
{
    // The done loop calls client code in OnBufferDone for visualization / HD
    // recording. This call is protected against exceptions from client code
    // with try/catch.
    // The remainder of this method should never cause an exception.
    // But if it does, the exception will be caught by the calling method,
    // DoneMain.
    SoundData * psdCapture = 0;
    SoundData * psdPlayback = 0;
    if (GetSoundDataExchanger()->HasDoneQueue() == false)
    {
        // no need to activate the done loop when there is no done queue.
        return;
    }
    for (;;) // "done" loop during sound i/o
    {
        // Calls OnBufferDone whenever there is at least one SoundData buffer
        // in each (capture & playback) of the "Done" queues

        if (WaitForSingleObject(m_phDoneEvents[QUIT], 0) == WAIT_OBJECT_0)
        {
            // QUIT causes immediate return of the done loop, DONE_STOP
            // is not handled here, since it allows for the done processing to
            // finish all data currently available in the "done" queues.
            return;
        }

        unsigned nBuffersWaiting =
            GetSoundDataExchanger()->DoneNumFilledBuffers();
        if (nBuffersWaiting == 0U)
        {
            if (m_bDoneLoopWaitsWhenDoneQueuesEmpty == false)
            {
                // no more data will come
                return;
            }
            nBuffersWaiting = WaitForDoneData();
            if (nBuffersWaiting == 0U)
            {
                /// if WaitForDoneData returns empty, then
                /// (1) the QUIT event was set, or
                /// (2) the DONE_STOP event was set and there is no more data.
                return;
            }
        }
        GetSoundDataExchanger()->GetDoneBuffers(&psdCapture, &psdPlayback);
        try
        {
            OnBufferDone(*psdCapture, *psdPlayback, (long)nBuffersWaiting);
        }
        catch(...)
        {
            SetEvent(m_phCbEvents[DONE_ERROR]);
        }
        GetSoundDataExchanger()->PopDoneBuffers();
    }
}

/// Called from #DoneLoop when no "done" buffers are present.
/// Waits until either data becomes available in the "done" queues,
/// or until m_phDoneEvents[QUIT] or m_phDoneEvents[DONE_STOP] become
/// set.
/// @return The number of buffers present in the "done" queues at
///         return time.
unsigned CAsio::WaitForDoneData()
{
    m_bWaitForDoneDataActive = true; // Function entered mark
    enum LocalEvents
    {
        L_QUIT, L_CAPTURE, L_PLAYBACK, L_STOP, L_EVENTS
    };
    HANDLE rghEvents[L_EVENTS];
    rghEvents[L_QUIT] = m_phDoneEvents[QUIT];
    rghEvents[L_CAPTURE] = GetSoundDataExchanger()->GetDoneCaptureDataEvent();
    rghEvents[L_PLAYBACK] = GetSoundDataExchanger()->GetDonePlaybackDataEvent();
    rghEvents[L_STOP] = m_phDoneEvents[DONE_STOP];
    for (;;)
    {
        unsigned nNumBuffersWaiting;
        DWORD nWaitResult =
            WaitForMultipleObjects(L_EVENTS, rghEvents, false, INFINITE);
        switch (nWaitResult)
        {
        case DWORD(WAIT_OBJECT_0 + L_QUIT):
            // no further processing if the thread is told to terminate, or
            // if sound i/o has finished completely
            m_bWaitForDoneDataActive = false; // Leaving function mark
            return 0;
        case DWORD(WAIT_OBJECT_0 + L_STOP):
            // no further waiting necessary at the end of the stop procedure
            m_bWaitForDoneDataActive = false; // Leaving function mark
            return GetSoundDataExchanger()->DoneNumFilledBuffers();
        case DWORD(WAIT_OBJECT_0 + L_CAPTURE):
        case DWORD(WAIT_OBJECT_0 + L_PLAYBACK):
            // Both are signalled once before new data can be handled in the
            // done thread. It does not matter which comes first.
            nNumBuffersWaiting =
                GetSoundDataExchanger()->DoneNumFilledBuffers();
            if (nNumBuffersWaiting > 0U)
            {   // new data can be handled
                m_bWaitForDoneDataActive = false; // Leaving function mark
                return nNumBuffersWaiting;
            }
            // else continue waiting for the other event (or for QUIT, ...)
        }
    }
}

unsigned int CAsio::ComputeStopTimeout(unsigned int nBufsize,
                                       double dSrate,
                                       unsigned int nTolerance)
{
    if (dSrate < 1000.0)
    {
        throw EUnsupportedParamsError("CAsio::StopTimeout", "dSrate < 1000");
    }

    // this cannot overflow since dSrate is >= 1000.0
    unsigned int nMillisecondsPerBuffer =
        static_cast<unsigned int>(ceil(nBufsize / dSrate * 1000.0));

    unsigned int nStopTimeout = 2U*nMillisecondsPerBuffer + nTolerance;
    // Check for overflow
    if (   2U*nMillisecondsPerBuffer < nMillisecondsPerBuffer
        || nStopTimeout < 2U*nMillisecondsPerBuffer
        || nStopTimeout < nTolerance)
    {
        throw EUnsupportedParamsError("CAsio::StopTimeout", "Overflow");
    }
    return nStopTimeout;
}

void CAsio::SilenceOutputs(long nDoubleBufferIndex)
{
    size_t nChannel;
    for (nChannel = 0; nChannel < m_nActiveChannelsOut; ++nChannel)
    {
        size_t nBytesPerSample;
        switch (m_vviActiveChDataTypes[OUTPUT][nChannel])
        {
        case ASIOSTInt16MSB:
        case ASIOSTInt16LSB:
            nBytesPerSample = 2;
            break;
        case ASIOSTInt24MSB:
        case ASIOSTInt24LSB:
            nBytesPerSample = 3;
            break;
        case ASIOSTFloat64LSB:
        case ASIOSTFloat64MSB:
            nBytesPerSample = 8;
            break;
        default:
            nBytesPerSample = 4;
        }

        // Array m_rgabiBufferInfos contains buffer information of both, input
        // and output channels.
        size_t nBufferInfoIndex = nChannel + m_nActiveChannelsIn;
        memset(m_rgabiBufferInfos[nBufferInfoIndex].buffers[nDoubleBufferIndex],
               0,
               nBytesPerSample * (unsigned long)m_iPreparedBuffersize);
    }
    if (m_bPostOutput)
    {   // if driver wants to know, tell that output buffers have been written
        ASIOOutputReady();
    }
}

void CAsio::LoadDriver(AnsiString sDriverName, size_t nIndex, void * lpSysRef)
{
    if (DriverNameAtIndex(nIndex) != sDriverName)
    {
        throw ELoadError("Asio::LoadDriver", "No such driver.");
    }
    UnloadDriver();

    // prepare driverInfo struct
    m_padiDriverInfo->asioVersion = SUPPORTED_ASIO_VERSION;
    m_padiDriverInfo->driverVersion = -1;
    memset(m_padiDriverInfo->name,
           0,
           sizeof(m_padiDriverInfo->name));
    memset(m_padiDriverInfo->errorMessage,
           0,
           sizeof(m_padiDriverInfo->errorMessage));
    m_padiDriverInfo->sysRef = lpSysRef;

    if (CAsio::sm_etExceptionTrigger == CAsio::LOAD_DRIVER_THROWS_LOAD_ERROR_LD
        ||
        m_padAsioDrivers->loadDriver(const_cast<char*>(sDriverName.c_str()))
        == false)
    {
        throw ELoadError("CAsio::LoadDriver", "Cannot load Asio driver");
    }

    // Ensure the strings in the ASIODriverInfo struct terminate
    m_padiDriverInfo->name[sizeof(m_padiDriverInfo->name) - 1] = '\0';
    m_padiDriverInfo->errorMessage[sizeof(m_padiDriverInfo->errorMessage)
                                   - 1] = '\0';

    m_nCurrentDriverIndex = (long)nIndex;
    SetState(LOADED);

    if (DriverInfoAsioVersion() != SUPPORTED_ASIO_VERSION
        || sm_etExceptionTrigger == LOAD_DRIVER_THROWS_VERSION_ERROR) {
        throw EVersionError("CAsio::LoadDriver",
                            "Driver supports wrong ASIO version",
                            SUPPORTED_ASIO_VERSION,
                            DriverInfoAsioVersion());
    }

    ASIOError aseErr;
    // switch off clang warning: not all enum values handled by pupose
    #pragma clang diagnostic push
    #pragma clang diagnostic ignored "-Wswitch-enum"
    #pragma clang diagnostic ignored "-Wdisabled-macro-expansion"
    switch (sm_etExceptionTrigger)
    {
    case LOAD_DRIVER_THROWS_LOAD_ERROR_NP:
        aseErr = ASE_NotPresent;
        break;
    case LOAD_DRIVER_THROWS_LOAD_ERROR_HW:
        aseErr = ASE_HWMalfunction;
        break;
    case LOAD_DRIVER_THROWS_NO_MEMORY_ERROR:
        aseErr = ASE_NoMemory;
        break;
    case LOAD_DRIVER_THROWS_UNEXPECTED_ERROR:
        aseErr = ASIO_UNEXPECTED;
        break;
    default:
        aseErr = ASIOInit(m_padiDriverInfo);
        break;
    }
    #pragma clang diagnostic pop

    switch(aseErr)
    {
    case ASE_NotPresent:
        throw ELoadError("CAsio::LoadDriver",
                         "No input/output present while initializing Asio"
                         " driver");
    case ASE_HWMalfunction:
        throw ELoadError("CAsio::LoadDriver",
                         "Hardware malfunction while initializing Asio driver");
    case ASE_NoMemory:
        throw ENoMemoryError("CAsio::LoadDriver",
                             "Not enough memory while initializing Asio"
                             " driver");
    case ASE_OK:
        SetState(INITIALIZED);
        break;
    default:
        throw EUnexpectedError(__FILE__,__LINE__,
                               "CAsio::LoadDriver",
                               "Driver returned unkown error code from"
                               "init function",
                               this,
                               aseErr);
    }

    // call private function to determine and store fix number of input and
    // output channels
    HardwareChannelsInternal();
    m_bPostOutput = (ASIOOutputReady() == ASE_OK);
}

long CAsio::AsioBufsizeQuery(long * piMin,
                             long * piMax,
                             long * piGran) const
{
    // return particular values for test of CAsio::BufferSizes()
    switch (m_nTestAsioBufsizeQuery)
    {
       // - return iBufsizeMin != iBufsizeMax AND Granularity == 0.
       case 1: if (piMin)  *piMin   = 1;
               if (piMax)  *piMax   = 8;
               if (piGran) *piGran  = 0;
               return 5;
       // - return iBufsizeMin != iBufsizeMax and a Granularity NOT module
       //   iBufsizeMax - iBufsizeMin.
       case 2: if (piMin)  *piMin   = 2;
               if (piMax)  *piMax   = 9;
               if (piGran) *piGran  = 2;
               return 5;
       // - return valid values, but BufsizeBest != iBufsizeMin + n*Granularity
       case 3: if (piMin)  *piMin   = 2;
               if (piMax)  *piMax   = 6;
               if (piGran) *piGran  = 2;
               return 5;
       // - return valid values, BufsizeBest == iBufsizeMin + n*Granularity
       case 4: if (piMin)  *piMin   = 2;
               if (piMax)  *piMax   = 6;
               if (piGran) *piGran  = 2;
               return 4;
       default: break;
    }

    // Initialize with invalid values.
    long iMin = -1;
    long iMax = -1;
    long iBest = -1;
    long iGran = -2;

    // Get buffersize parameters
    AssertInitialized("Cannot query for ASIO driver's buffer size parameters");
    ASIOError aeErr;
    // switch off clang warning: not all enum values handled by pupose
    #pragma clang diagnostic push
    #pragma clang diagnostic ignored "-Wswitch-enum"
    switch (sm_etExceptionTrigger)
    {
    case ASIO_BUFSIZE_QUERY_THROWS_UNAVAILABLE_ERROR:
        aeErr = ASE_NotPresent;
        break;
    case ASIO_BUFSIZE_QUERY_THROWS_UNEXPECTED_ERROR:
        aeErr = ASIO_UNEXPECTED;
        break;
    default:
        aeErr = ASIOGetBufferSize(&iMin, &iMax, &iBest, &iGran);
        if (piMax) *piMax = iMax;
        if (piMin) *piMin = iMin;
        if (piGran) *piGran = iGran;
    }
    #pragma clang diagnostic pop
    switch (aeErr)
    {
    case ASE_OK:
        break;
    case ASE_NotPresent:
        throw EUnavailableError("CAsio::AsioBufsizeQuery",
                                "Cannot query for Asio driver's buffer size"
                                "parameters: No input/output present error");
    default:
        throw EUnexpectedError(__FILE__,__LINE__,
                               "CAsio::AsioBufsizeQuery",
                               "Driver returned unexpected error code",
                               this, aeErr);

    }

    // NOTE: we don't need to check this, since we ALWAYS use bufsizebest. Thus
    // removed this check (some drivers are 'lying' here!)
    #ifdef CHECK_BUFSIZES_FROM_DRIVER
    CheckBufsizeInterval(iMin, iBest, iGran);
    CheckBufsizeInterval(iBest, iMax, iGran);
    #endif

    return iBest;
}

/// Validity check on buffer size parameters reported by the alsa
/// driver.  Driver reports 3 buffersizes (minimum, preferred, and
/// maximum), and a "granularity" value.  0 < minimum <= preferred <=
/// maximum has to be true, granularity, if positive, is the
/// difference between valid buffer sizes.  If granularity is -1, then
/// valid buffer sizes differ by a factor 2.  This method checks if
/// the granularity is consistent with the given interval sizes. The
/// granularity can be checked int the two passes, in intervals
/// [minimum, preferred] and [preferred, maximum].  the given buffer
/// size interval borders.
/// \param iLower
///    minimum buffer size or preferred buffer size
/// \param iUpper
///    preferred buffer size or maximum buffer size
/// \param iGran
///    buffer size granularity.
/// \throw EUnexpectedError
///    if the parameters are inconsistent.
void CAsio::CheckBufsizeInterval(long iLower, long iUpper, long iGran)
{
   AnsiString asErrorMessage;

    // Check buffersize parameters for consistency
    if (iLower > iUpper || iLower <= 0)
    {
        asErrorMessage.printf("Inconsistent buffer sizes (1): %d, %d, %d", iLower, iUpper, iGran);    // Check buffersize parameters for consistency
        throw EUnexpectedError(__FILE__, __LINE__,
                               "CAsio::CheckBufsizeInterval",
                               asErrorMessage.c_str());
    }
    if (iLower != iUpper && iGran != 0)
    {
        if (iGran > 0)
        {
            // iUpper-iLower must be n*iGran
            if ((iUpper - iLower) % iGran != 0)
            {
                 asErrorMessage.printf("Inconsistent buffer sizes (2): %d, %d, %d", iLower, iUpper, iGran);    // Check buffersize parameters for consistency
                 throw EUnexpectedError(__FILE__, __LINE__,
                                        "CAsio::CheckBufsizeInterval",
                                        asErrorMessage.c_str());
            }
        }
        else if (iGran == -1)
        {
            // iUpper / iLower must be a power of 2
            if (iUpper % iLower != 0 ||
                ! IsPowerOf2((unsigned long)(iUpper / iLower)))
            {
                 asErrorMessage.printf("Inconsistent buffer sizes (3): %d, %d, %d", iLower, iUpper, iGran);    // Check buffersize parameters for consistency
                 throw EUnexpectedError(__FILE__, __LINE__,
                                        "CAsio::CheckBufsizeInterval",
                                        asErrorMessage.c_str());
            }
        }
        else
        {
            // other values for bufsizeGranularity are not permitted when
            // iUpper != iLower
            throw EUnexpectedError(__FILE__, __LINE__,
                                   "CAsio::CheckBufsizeInterval",
                                   "Invalid buffer size granularity",
                                   0, iGran);
        }
    }
}

bool CAsio::IsPowerOf2(unsigned long iValue)
{
    unsigned long iTestbit;
    bool bFoundBit = false;
    for (iTestbit = 1; (iTestbit <= iValue) && iTestbit; iTestbit <<= 1)
    {
        if (iValue & iTestbit)
        {
            if (bFoundBit)
            {
                // found 2nd bit, no power of 2
                return false;
            }
            bFoundBit = true;
        }
    }
    return bFoundBit;
}

void CAsio::GetChannelInfo(long iChannelIndex,
                           Direction adDirection,
                           struct ASIOChannelInfo * paciInfo) const
{
    AssertInitialized("Cannot get channel info");

    if (iChannelIndex < 0 || iChannelIndex >= HardwareChannels(adDirection))
    {
        throw EIndexError("CAsio::getChannelInfo",
                          "Channel index is out of bounds",
                          0, HardwareChannels(adDirection), iChannelIndex);
    }

    paciInfo->channel = iChannelIndex;
    paciInfo->isInput = (adDirection == INPUT) ? ASIOTrue : ASIOFalse;
    ASIOError aeErr;
    // switch off clang warning: not all enum values handled by pupose
    #pragma clang diagnostic push
    #pragma clang diagnostic ignored "-Wswitch-enum"
    switch(sm_etExceptionTrigger)
    {
    case GET_CHANNEL_INFO_THROWS_UNAVAILABLE_ERROR:
        aeErr = ASE_NotPresent;
        break;
    case GET_CHANNEL_INFO_THROWS_UNEXPECTED_ERROR:
        aeErr = ASIO_UNEXPECTED;
        break;
    default:
        aeErr = ASIOGetChannelInfo(paciInfo);
    }
    #pragma clang diagnostic pop
    switch (aeErr)
    {
    case ASE_OK:
        return;
    case ASE_NotPresent:
        throw EUnavailableError("CAsio::getChannelInfo",
                                "No input/output present error");
    }
    throw EUnexpectedError(__FILE__, __LINE__, "CAsio::getChannelInfo",
                           "Unexpected error code from driver", this, aeErr);
}

void CAsio::GetClockSources(std::vector<ASIOClockSource> & vacsClocks) const
{
    AssertInitialized("Cannot query clock sources");

    // Find out how many clocks sources there are
    long nClocks = MAX_CLOCKS;
    vacsClocks.resize(MAX_CLOCKS);
    ASIOError aeErr;
    // switch off clang warning: not all enum values handled by pupose
    #pragma clang diagnostic push
    #pragma clang diagnostic ignored "-Wswitch-enum"
    switch (sm_etExceptionTrigger)
    {
    case GET_CLOCK_SOURCES_THROWS_UNAVAILABLE_ERROR:
        aeErr = ASE_NotPresent;
        break;
    case GET_CLOCK_SOURCES_THROWS_UNEXPECTED_ERROR_CODE:
        aeErr = ASIO_UNEXPECTED;
        break;
    default:
        aeErr = ASIOGetClockSources(&vacsClocks[0], &nClocks);
    }
    #pragma clang diagnostic pop
    switch (aeErr)
    {
    case ASE_OK:
        break;
    case ASE_NotPresent:
        throw EUnavailableError("CAsio::GetClockSources",
                                "No Input/output present");
    default:
        throw EUnexpectedError(__FILE__, __LINE__, "CAsio::GetClockSources",
                               "Driver returned unknown Error code",
                               this, aeErr);
    }
    if (nClocks <= 0 ||
        sm_etExceptionTrigger == GET_CLOCK_SOURCES_THROWS_NEGATICE_COUNT_ERROR)
    {
        throw ENegativeCountError(__FILE__, __LINE__, "CAsio::GetClockSources",
                                  "No clock sources or negative number of clock sources",
                                  this, nClocks);
    }
    if (nClocks > MAX_CLOCKS ||
        sm_etExceptionTrigger == GET_CLOCK_SOURCES_THROWS_LIMIT_EXCEEDED_ERROR)
    {
        throw ELimitExceededError(__FILE__,__LINE__, "CAsio::GetClockSources",
                                  "Driver has more clock sources than CAsio"
                                  " can handle",
                                  this,
                                  MAX_CLOCKS,
                                  nClocks);
    }
    vacsClocks.resize((unsigned int)nClocks);

    // validity check
    long iClockSourceIndex;
    for (iClockSourceIndex=0; iClockSourceIndex < nClocks; ++iClockSourceIndex)
    {
        if (vacsClocks[(unsigned int)iClockSourceIndex].index != (long)iClockSourceIndex
            || (sm_etExceptionTrigger == 
                GET_CLOCK_SOURCES_THROWS_UNEXPECTED_ERROR_NUMBERING))
        {
            throw EUnexpectedError(__FILE__, __LINE__, "CAsio::GetClockSources",
                                   "Array of clock sources for this ASIO driver"
                                   " has conflicting clock source indices",
                                   this, iClockSourceIndex);
        }
    }
}

void CAsio::SetState(State asState)
{
    if (m_asState != asState)
    {
        m_asState = asState;
        try
        {
            OnStateChange(asState);
        }
        catch (...)
        {   // Callback should not raise exceptions.
            TriggerWarning("Asio::SetState: OnStateChange raised Exception");
        }
    }
}

void CAsio::TriggerWarning(const char * lpcszWarning)
{
    try
    {
        OnWarning(lpcszWarning);
    }
    catch (...)
    {
        // not much sense in triggering another warning because of OnWarning
        // raising an exception
    }
}


bool CAsio::SilenceOutputsIfStopping(long nDoubleBufferIndex)
{
    bool bStopping = m_bStopping;
    if (bStopping)
    {
        SilenceOutputs(nDoubleBufferIndex); // clear sound output buffers
        SetEvent(m_phStopEvents[STOP_CONTINUE]); // progress in stopping
    }
    return bStopping;
}

void CAsio::ConvertInputsToFloat(long nDoubleBufferIndex,
                                 std::vector<std::valarray<float> > & vvfBuf)
{
    AssertInitialized("CAsio::ConvertInputsToFloat may only be called from"
                      " bufferSwitch handling", RUNNING);
    if (vvfBuf.size() != m_nActiveChannelsIn)
    {
        throw EUnexpectedError(__FILE__,
                               __LINE__,
                               "CAsio::ConvertInputsToFloat",
                               "Channel count mismatch",
                               this,
                               vvfBuf.size());
    }
    size_t nChannel;
    for (nChannel = 0; nChannel < m_nActiveChannelsIn; ++nChannel){
        if (vvfBuf[nChannel].size() != size_t(BufsizeCurrent()))
        {
            throw EUnexpectedError(__FILE__,
                                   __LINE__,
                                   "CAsio::ConvertInputsToFloat",
                                   "Frames count mismatch",
                                   this,
                                   vvfBuf[nChannel].size());
        }
        Asio2Float(m_rgabiBufferInfos[nChannel].buffers[nDoubleBufferIndex],
                   m_vviActiveChDataTypes[INPUT][nChannel],
                   vvfBuf[nChannel]);
    }
}

void CAsio::ConvertFloatToOutputs(long nDoubleBufferIndex,
                                  std::vector<std::valarray<float> > & vvfBuf)
{
    AssertInitialized("CAsio::ConvertFloatToOutputs may only be called from"
                      " bufferSwitch handling", RUNNING);
    if (vvfBuf.size() != m_nActiveChannelsOut)
    {
        throw EUnexpectedError(__FILE__,
                               __LINE__,
                               "CAsio::ConvertFloatToOutputs",
                               "Channel count mismatch",
                               this,
                               vvfBuf.size());
    }
    size_t nChannel;
    for (nChannel = 0; nChannel < m_nActiveChannelsOut; ++nChannel){
        if (vvfBuf[nChannel].size() != size_t(BufsizeCurrent()))
        {
            throw EUnexpectedError(__FILE__,
                                   __LINE__,
                                   "CAsio::ConvertFloatToOutputs",
                                   "Frames count mismatch",
                                   this,
                                   vvfBuf[nChannel].size());
        }
        Float2Asio(vvfBuf[nChannel],
                   m_rgabiBufferInfos[nChannel + m_nActiveChannelsIn].buffers[nDoubleBufferIndex],
                   m_vviActiveChDataTypes[OUTPUT][nChannel]);
    }
    if (m_bPostOutput)
    {
        // if driver wants to know, tell that output buffers have been written
        ASIOOutputReady();
    }
}

SoundDataExchanger * CAsio::GetSoundDataExchanger()
{
    return m_psxSoundDataExchanger;
}

void CAsio::StaticSampleRateDidChange(double dSrate)
{
    CAsio * pcaInst = Instance();
    if (pcaInst)
    {
        pcaInst->OnRateChange(dSrate);
    }
}

long CAsio::StaticAsioMessage(long iSelector,
                              long iValue,
                              void* pvMessage,
                              double* pdOpt)

{
    CAsio * pcaInst = Instance();

    // This is the minimum required implementation of the Asio host callback
    // asioMessage.
    (void) pvMessage;
    (void) pdOpt;
    switch (iSelector)
    {
    case kAsioSelectorSupported:
        if (iValue == kAsioSelectorSupported ||
            iValue == kAsioEngineVersion
            || iValue == kAsioSupportsInputMonitor
            )
        {
            return 1L;
        }
        return 0;
    case kAsioEngineVersion:
        return 2;
    case kAsioResetRequest:
       if (pcaInst)
        pcaInst->OnResetRequest();
       return 0;
    case kAsioBufferSizeChange:
       if (pcaInst)
        pcaInst->OnBufferSizeChangeChange();
       return 0;
    }
    return 0;
}

std::vector<bool>
CAsio::ResizeBitVector(const std::vector<bool> & vbIn, size_t nSize)
{
    size_t nIndex;
    for (nIndex = nSize; nIndex < vbIn.size(); ++nIndex)
    {
        if (vbIn[nIndex])
        {
            throw EModeError("CAsio::ResizeBitVector",
                             "Truncate range contains at least 1 'true' bit");
        }
    }
    std::vector<bool> vbOut(nSize, false);
    for (nIndex = 0; nIndex < vbIn.size() && nIndex < nSize; ++nIndex)
    {
        vbOut[nIndex] = vbIn[nIndex];
    }
    return vbOut;
}

#define COPY_BYTESWAP(pchDest, pchSrc, nBytes) \
    do \
    { \
        for (nByte = 0; nByte < (nBytes); ++nByte) \
        { \
           *((pchDest) - nByte) = *((pchSrc) + nByte); \
        } \
    } while(0)
#define COPY_BYTESWAP_SIMPLE(nBytes) \
    COPY_BYTESWAP(&uValue.rgch[3], &pchSrc[nFrame*(nBytes)], (nBytes))

union UTypes {
    __int32 i;
    int64_t l;
    signed char rgch[8];
    float f;
    double d;
};

void CAsio::Asio2Float(const void * pvSrc, long iDataType,
                       std::valarray<float> & vfDest)
{
    const float fIntAmplitude = 2147483648.0f;
    const signed char * const pchSrc = static_cast<const signed char *>(pvSrc);
    unsigned nFrame;
    unsigned nByte;
    union UTypes uValue;
    for (nFrame = 0; nFrame < vfDest.size(); ++nFrame)
    {
        uValue.l = 0;
        switch(ASIOSampleType(iDataType)) {
        case ASIOSTInt16MSB: // Big Endian signed 16 Bits
            COPY_BYTESWAP_SIMPLE(2);
            break;
        case ASIOSTInt24MSB: // Big Endian packed 24 Bits
            COPY_BYTESWAP_SIMPLE(3);
            break;
        case ASIOSTInt32MSB: // Big Endian 32 Bits
            COPY_BYTESWAP_SIMPLE(4);
            break;
        case ASIOSTInt32MSB16: // only least 16 Bits carry value
            COPY_BYTESWAP(&uValue.rgch[3], &pchSrc[nFrame*4+2], 2);
            break;
        case ASIOSTInt32MSB18: // only least 18 Bits carry value
            COPY_BYTESWAP(&uValue.rgch[3], &pchSrc[nFrame*4+1], 3);
            uValue.i <<= 6;
            break;
        case ASIOSTInt32MSB20: // only least 20 Bits carry value
            COPY_BYTESWAP(&uValue.rgch[3], &pchSrc[nFrame*4+1], 3);
            uValue.i <<= 4;
            break;
        case ASIOSTInt32MSB24: // only least 24 Bits carry value
            COPY_BYTESWAP(&uValue.rgch[3], &pchSrc[nFrame*4+1], 3);
            break;
        case ASIOSTInt16LSB: // Big Endian signed 16 Bits
            uValue.i = static_cast<const __int16 *>(pvSrc)[nFrame];
            uValue.i <<= 16;
            break;
        case ASIOSTInt24LSB: // Big Endian packed 24 Bits
            uValue.rgch[1] = pchSrc[nFrame*3];
            uValue.rgch[2] = pchSrc[nFrame*3+1];
            uValue.rgch[3] = pchSrc[nFrame*3+2];
            break;
        case ASIOSTInt32LSB: // Little Endian 32 Bits
            uValue.i = static_cast<const __int32 *>(pvSrc)[nFrame];
            break;
        case ASIOSTInt32LSB16: // only least 16 Bits carry value
            uValue.i = static_cast<const __int32 *>(pvSrc)[nFrame];
            uValue.i <<= 16;
            break;
        case ASIOSTInt32LSB18: // only least 18 Bits carry value
            uValue.i = static_cast<const __int32 *>(pvSrc)[nFrame];
            uValue.i <<= 14;
            break;
        case ASIOSTInt32LSB20: // only least 20 Bits carry value
            uValue.i = static_cast<const __int32 *>(pvSrc)[nFrame];
            uValue.i <<= 12;
            break;
        case ASIOSTInt32LSB24: // only least 24 Bits carry value
            uValue.i = static_cast<const __int32 *>(pvSrc)[nFrame];
            uValue.i <<= 8;
            break;
        case ASIOSTFloat32LSB:
            vfDest[nFrame] = static_cast<const float *>(pvSrc)[nFrame];
            continue;
        case ASIOSTFloat64LSB:
            vfDest[nFrame] = (float)static_cast<const double *>(pvSrc)[nFrame];
            continue;
        case ASIOSTFloat32MSB:
            COPY_BYTESWAP_SIMPLE(4);
            vfDest[nFrame] = uValue.f;
            continue;
        case ASIOSTFloat64MSB:
            COPY_BYTESWAP(&uValue.rgch[7], &pchSrc[nFrame*8], 8);
            vfDest[nFrame] = (float)uValue.d;
            continue;
        default:
            vfDest[nFrame] = 0;
            continue;
        }
        vfDest[nFrame] = uValue.i / fIntAmplitude;
    }
}

void CAsio::Float2Asio(const std::valarray<float> & vfSrc,
                       void * pvDest, long iDataType)
{
    const float fIntAmplitude = float(1u<<31);
    const float fIntMin = -fIntAmplitude;
    const float fIntMax =
        fIntAmplitude * (1 - std::numeric_limits<float>::epsilon());

    signed char * const pchDest = static_cast<signed char *>(pvDest);
    unsigned nFrame;
    unsigned nByte;
    union UTypes uValue;
    float fValue;
    for (nFrame = 0; nFrame < vfSrc.size(); ++nFrame)
    {
        uValue.l = 0;
        switch (iDataType) {
        case ASIOSTFloat32LSB:
        case ASIOSTFloat64LSB:
        case ASIOSTFloat32MSB:
        case ASIOSTFloat64MSB:
            fValue = vfSrc[nFrame];
            if (fValue < -1.0f)
            {
                fValue = -1.0f;
            }
            else if (fValue > 1.0f)
            {
                fValue = 1.0f;
            }
            break;
        default:
            fValue = vfSrc[nFrame] * fIntAmplitude;
            if (fValue < fIntMin)
            {
                fValue = fIntMin;
            }
            else if (fValue > fIntMax)
            {
                fValue = fIntMax;
            }
            uValue.i = static_cast<int>(fValue);
        }
       // switch off clang warning: fallthrough by purpose
       #pragma clang diagnostic push
       #pragma clang diagnostic ignored "-Wimplicit-fallthrough"
        switch(iDataType) {
        case ASIOSTInt16MSB: // Big Endian signed 16 Bits
            COPY_BYTESWAP(&pchDest[nFrame*2+1], &uValue.rgch[2], 2);
            break;
        case ASIOSTInt24MSB: // Big Endian packed 24 Bits
            COPY_BYTESWAP(&pchDest[nFrame*3+2], &uValue.rgch[1], 3);
            break;
        case ASIOSTInt32MSB16: // only least 16 Bits carry value
            uValue.i >>= 2;
            // FALL THROUGH
        case ASIOSTInt32MSB18: // only least 18 Bits carry value
            uValue.i >>= 2;
            // FALL THROUGH
        case ASIOSTInt32MSB20: // only least 20 Bits carry value
            uValue.i >>= 4;
            // FALL THROUGH
        case ASIOSTInt32MSB24: // only least 24 Bits carry value
            uValue.i >>= 8;
        case ASIOSTInt32MSB: // Big Endian 32 Bits
            COPY_BYTESWAP(&pchDest[nFrame*4+3], &uValue.rgch[0], 4);
            break;
        case ASIOSTInt16LSB: // Big Endian signed 16 Bits
            static_cast<__int16 *>(pvDest)[nFrame] =
                static_cast<__int16>(uValue.i >> 16);
            break;
        case ASIOSTInt24LSB: // Big Endian packed 24 Bits
            pchDest[nFrame*3]   = uValue.rgch[1];

            pchDest[nFrame*3+1] = uValue.rgch[2];
            pchDest[nFrame*3+2] = uValue.rgch[3];
            break;
        case ASIOSTInt32LSB: // Big Endian 32 Bits
            static_cast<__int32 *>(pvDest)[nFrame] = uValue.i;
            break;
        case ASIOSTInt32LSB16: // only least 16 Bits carry value
            static_cast<__int32 *>(pvDest)[nFrame] = uValue.i >> 16;
            break;
        case ASIOSTInt32LSB18: // only least 18 Bits carry value
            static_cast<__int32 *>(pvDest)[nFrame] = uValue.i >> 14;
            break;
        case ASIOSTInt32LSB20: // only least 20 Bits carry value
            static_cast<__int32 *>(pvDest)[nFrame] = uValue.i >> 12;
            break;
        case ASIOSTInt32LSB24: // only least 24 Bits carry value
            static_cast<__int32 *>(pvDest)[nFrame] = uValue.i >> 8;
            break;
        case ASIOSTFloat32LSB:
            static_cast<float *>(pvDest)[nFrame] = fValue;
            break;
        case ASIOSTFloat64LSB:
            static_cast<double *>(pvDest)[nFrame] = (double)fValue;
            break;
        case ASIOSTFloat32MSB:
            uValue.f = fValue;
            COPY_BYTESWAP(&pchDest[nFrame*4+3], &uValue.rgch[0], 4);
            break;
        case ASIOSTFloat64MSB:
            uValue.d = (double)fValue;
            COPY_BYTESWAP(&pchDest[nFrame*8+7], &uValue.rgch[0], 8);
            break;
        }
       #pragma clang diagnostic pop
    }
}
float CAsio::MaxFloatSample(long iDataType)
{
    return RoundTripFloatSample(iDataType, 1.0f);
}
float CAsio::MinFloatSample(long iDataType)
{
    return RoundTripFloatSample(iDataType, -1.0f);
}
float CAsio::RoundTripFloatSample(long iDataType, float fSampleValue)
{
    std::valarray<float> vfBuf(fSampleValue, 1);
    char rgchAsioMemory[16];
    Float2Asio(vfBuf, rgchAsioMemory, iDataType);
    Asio2Float(rgchAsioMemory, iDataType, vfBuf);
    return vfBuf[0];
}
void  CAsio::OnBufferDone(SoundData & sdBuffersIn,
                          SoundData & sdBuffersOut,
                          long nBuffersWaiting)
{
    // This method sets a testing flag that is checked in test_DoneLoop
    m_lpcszTestingLastCallback = "OnBufferDone";
    m_nTestingBufferDoneChannelsIn = sdBuffersIn.m_vvfData.size();
    m_nTestingBufferDoneChannelsOut = sdBuffersOut.m_vvfData.size();
    m_nTestingDoneBuffersWaiting = (size_t)nBuffersWaiting;
    if (m_nTestingBufferDoneChannelsIn != 0U &&
        sdBuffersIn.m_vvfData[0].size() != 0U)
    {
        m_fTestingBufferDoneCaptureSample = sdBuffersIn.m_vvfData[0][0];
    }
    if (m_nTestingBufferDoneChannelsOut != 0U &&
        sdBuffersOut.m_vvfData[0].size() != 0U)
    {
        m_fTestingBufferDonePlaybackSample = sdBuffersOut.m_vvfData[0][0];
    }
}

// Local Variables:
// mode: c++
// c-file-style: "stroustrup"
// c-basic-offset: 4
// indent-tabs-mode: nil
// End:
