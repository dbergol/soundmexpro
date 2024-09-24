//------------------------------------------------------------------------------
/// \file SoundDllPro_SoundClassAsio.h
/// \author Berg
/// \brief Interface of ASIO sound class for SoundMexPro. Inherits from CAsio
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
#ifndef SoundDllPro_SoundClassAsioH
#define SoundDllPro_SoundClassAsioH
//---------------------------------------------------------------------------
#include "SoundDllPro_SoundClassBase.h"
#include <casio.h>
#define ASIODRIVER_SYSREF        NULL
#pragma clang diagnostic push
#pragma clang diagnostic ignored "-Wheader-hygiene"
using namespace Asio;
#pragma clang diagnostic pop

//------------------------------------------------------------------------------
/// \class SoundClassAsio. Inherits from CAsio AND SoundClassBase.
/// Implements all abstract functions for use with ASIO and overloads all necessary 
/// functions of CAsio class
//------------------------------------------------------------------------------
class SoundClassAsio : public SoundClassBase, CAsio
{
   public:
      SoundClassAsio();
      virtual void         SoundSetSaveProcessedCaptureData(bool b);
      virtual void         SoundInit(TStringList* psl);
      virtual bool         SoundInitialized();
      virtual void         SoundLoadDriverByIndex(size_t nIndex);
      virtual void         SoundLoadDriverByName(AnsiString strName);
      virtual void         SoundUnloadDriver();
      virtual size_t       SoundNumDrivers();
      virtual AnsiString   SoundDriverName(unsigned int iIndex);
      virtual unsigned int SoundCurrentDriverIndex(void);
      virtual long         SoundChannels(Direction adDirection);
      virtual AnsiString   SoundChannelName(unsigned int iChannelIndex, Direction adDirection);
      virtual void         SoundStart();
      virtual void         SoundStop(bool bWaitForStopDone);
      virtual bool         SoundIsRunning();
      virtual bool         SoundIsStopping();
      virtual size_t       SoundActiveChannels(Direction adDirection);
      virtual AnsiString   SoundGetActiveChannelName(unsigned int iChannelIndex, Direction adDirection);
      virtual long         SoundBufsizeCurrent();
      virtual long         SoundBufsizeBest();
      virtual long         SoundNumBufOut();
      virtual void         SoundShowControlPanel(void);
      virtual void         SoundSetSampleRate(double dSampleRate);
      virtual bool         SoundCanSampleRate(double dSampleRate);
      virtual double       SoundGetSampleRate(void);
      virtual long         SoundGetLatency(Direction adDirection);
      virtual float        SoundActiveChannelMaxValue(Direction adDirection, size_t nChannel);
      virtual float        SoundActiveChannelMinValue(Direction adDirection, size_t nChannel);
      virtual unsigned int SoundGetWatchdogTimeout(void);
      virtual AnsiString   SoundFormatString();
      // overloaded CAsio functions 
      virtual void OnBufferSizeChangeChange();
      virtual void OnResetRequest();
      virtual void OnRateChange(double dSrate);
      virtual void OnDoneLoopStopped();
      virtual void OnHang();
      virtual void OnFatalError();
      virtual void OnStateChange(State asState);
      virtual void OnXrun(XrunType xtXrunType);
      virtual void Process(SoundData & sdBuffersIn,
                           SoundData & sdBuffersOut,
                           unsigned nBuffersInWaiting,
                           bool bPreloading);
      virtual void OnBufferPlay(SoundData & sdBuffersOut);
      virtual void OnBufferDone( SoundData & sdBuffersIn,
                                 SoundData & sdBuffersOut,
                                 long nBuffersWaiting);
   private:
      unsigned int m_nNumProcBufs;
      bool m_bCaptureDoneProcessed;
      bool m_bFreezeSampleRateOnStart; ///< flag, if samplerate is assumed to be fix after start
      double m_dSampleRate;
};
//------------------------------------------------------------------------------
#endif
