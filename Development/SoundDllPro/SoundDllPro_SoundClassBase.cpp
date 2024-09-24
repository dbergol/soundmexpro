//------------------------------------------------------------------------------
/// \file SoundDllPro_SoundClassBase.cpp
/// \author Berg
/// \brief Implementation of abstract base class SoundClassBase. Used by
/// SoundMexPro main class SoundDllProMain. To be inherited for special
/// driver models (ASIO, WDM ...)
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

#include "SoundDllPro_SoundClassBase.h"

//---------------------------------------------------------------------------

#pragma package(smart_init)
//------------------------------------------------------------------------------

//------------------------------------------------------------------------------
/// contructor, initializes members
//------------------------------------------------------------------------------
SoundClassBase::SoundClassBase() :
    m_lpfnOnHang(NULL), m_lpfnOnStopComplete(NULL), m_lpfnOnError(NULL),
    m_lpfnOnStateChange(NULL), m_lpfnOnXrun(NULL), m_lpfnOnProcess(NULL),
    m_lpfnOnBufferPlay(NULL), m_lpfnOnBufferDone(NULL), m_nProcessCalls(0)
{
}
//------------------------------------------------------------------------------

//------------------------------------------------------------------------------
/// destructor: empty in base class
//------------------------------------------------------------------------------
SoundClassBase::~SoundClassBase(void) {}
//------------------------------------------------------------------------------

//------------------------------------------------------------------------------
/// tell sound class to pass processed data to done-queue. Empty in base class
/// to be overloaded in ASIO
//------------------------------------------------------------------------------
#pragma argsused
void SoundClassBase::SoundSetSaveProcessedCaptureData(bool b)
{
    // empty in base class
}

//------------------------------------------------------------------------------
/// Intended for showing control panel: throws exception in base class
//------------------------------------------------------------------------------
void SoundClassBase::SoundShowControlPanel(void)
{
    throw Exception("control panel not implemented for current driver model");
}
//------------------------------------------------------------------------------

//------------------------------------------------------------------------------
/// returns 'watch-dog-time': returns 500 in base class
//------------------------------------------------------------------------------
unsigned int SoundClassBase::SoundGetWatchdogTimeout(void)
{
    return 500;
}
//------------------------------------------------------------------------------

//------------------------------------------------------------------------------
/// returns latency for a direction. BAse class returns -1 always
//------------------------------------------------------------------------------
#pragma argsused
long SoundClassBase::SoundGetLatency(Asio::Direction adDirection)
{
    return -1;
}
//------------------------------------------------------------------------------

//------------------------------------------------------------------------------
/// \brief attachs a callback function to m_lpfnOnHang
/// \param[in] lpfn pointer to callback function
/// \exception Exception if device is running
//------------------------------------------------------------------------------
void SoundClassBase::SetOnHang(LPFNSOUNDVOIDFUNC lpfn)
{
    if (SoundIsRunning())
        throw Exception("cannot change callback while device is running");
    m_lpfnOnHang = lpfn;
}
//------------------------------------------------------------------------------

//------------------------------------------------------------------------------
/// \brief attachs a callback function to m_lpfnOnStopComplete
/// \param[in] lpfn pointer to callback function
/// \exception Exception if device is running
//------------------------------------------------------------------------------
void SoundClassBase::SetOnStopComplete(LPFNSOUNDVOIDFUNC lpfn)
{
    if (SoundIsRunning())
        throw Exception("cannot change callback while device is running");
    m_lpfnOnStopComplete = lpfn;
}
//------------------------------------------------------------------------------

//------------------------------------------------------------------------------
/// \brief attachs a callback function to m_lpfnOnError
/// \param[in] lpfn pointer to callback function
/// \exception Exception if device is running
//------------------------------------------------------------------------------
void SoundClassBase::SetOnError(LPFNSOUNDVOIDFUNC lpfn)
{
    if (SoundIsRunning())
        throw Exception("cannot change callback while device is running");
    m_lpfnOnError = lpfn;
}
//------------------------------------------------------------------------------

//------------------------------------------------------------------------------
/// \brief attachs a callback function to m_lpfnOnStateChange
/// \param[in] lpfn pointer to callback function
/// \exception Exception if device is running
//------------------------------------------------------------------------------
void SoundClassBase::SetOnStateChange(LPFNSOUNDSTATECHANGE lpfn)
{
    if (SoundIsRunning())
        throw Exception("cannot change callback while device is running");
    m_lpfnOnStateChange = lpfn;
}
//------------------------------------------------------------------------------

//------------------------------------------------------------------------------
/// \brief attachs a callback function to m_lpfnOnXrun
/// \param[in] lpfn pointer to callback function
/// \exception Exception if device is running
//------------------------------------------------------------------------------
void SoundClassBase::SetOnXrun(LPFNSOUNDXRUN lpfn)
{
    if (SoundIsRunning())
        throw Exception("cannot change callback while device is running");
    m_lpfnOnXrun = lpfn;
}
//------------------------------------------------------------------------------

//------------------------------------------------------------------------------
/// \brief attachs a callback function to m_lpfnOnProcess
/// \param[in] lpfn pointer to callback function
/// \exception Exception if device is running
//------------------------------------------------------------------------------
void SoundClassBase::SetOnProcess(LPFNSOUNDPROCESS lpfn)
{
    if (SoundIsRunning())
        throw Exception("cannot change callback while device is running");
    m_lpfnOnProcess = lpfn;
}
//------------------------------------------------------------------------------

//------------------------------------------------------------------------------
/// \brief attachs a callback function to m_lpfnOnBufferPlay
/// \param[in] lpfn pointer to callback function
/// \exception Exception if device is running
//------------------------------------------------------------------------------
void SoundClassBase::SetOnBufferPlay(LPFNSOUNDPLAY lpfn)
{
    if (SoundIsRunning())
        throw Exception("cannot change callback while device is running");
    m_lpfnOnBufferPlay = lpfn;
}
//------------------------------------------------------------------------------

//------------------------------------------------------------------------------
/// \brief attachs a callback function to m_lpfnOnBufferDone
/// \param[in] lpfn pointer to callback function
/// \exception Exception if device is running
//------------------------------------------------------------------------------
void SoundClassBase::SetOnBufferDone(LPFNSOUNDPROCESS lpfn)
{
    if (SoundIsRunning())
        throw Exception("cannot change callback while device is running");
    m_lpfnOnBufferDone = lpfn;
}
//------------------------------------------------------------------------------

//------------------------------------------------------------------------------
/// \retval number of processing calls up to now
//------------------------------------------------------------------------------
unsigned int SoundClassBase::ProcessCalls()
{
    return m_nProcessCalls;
}
//------------------------------------------------------------------------------

