//------------------------------------------------------------------------------
/// \file SoundDllPro_SoundClassBase.h
/// \author Berg
/// \brief Interface of abstract base class SoundClassBase. Used by
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
#ifndef SoundDllPro_SoundClassBaseH
#define SoundDllPro_SoundClassBaseH
//---------------------------------------------------------------------------

#include <casio.h>


//using namespace Asio;

//------------------------------------------------------------------------------
/// \typedef vvf vector of float valarrays
//------------------------------------------------------------------------------
typedef std::vector<std::valarray<float> > vvf;
//------------------------------------------------------------------------------
/// \typedef LPFNSOUNDSTATECHANGE state change callback for sound modules
//------------------------------------------------------------------------------
typedef void (__closure *LPFNSOUNDSTATECHANGE)(Asio::State asState);
//------------------------------------------------------------------------------
/// \typedef LPFNSOUNDXRUN Xrun callback for sound modules
//------------------------------------------------------------------------------
typedef void (__closure *LPFNSOUNDXRUN)(Asio::XrunType xtXrunType);
//------------------------------------------------------------------------------
/// \typedef LPFNSOUNDVOIDFUNC callback for sound modules
//------------------------------------------------------------------------------
typedef void (__closure *LPFNSOUNDVOIDFUNC)(void);
//------------------------------------------------------------------------------
/// \typedef LPFNSOUNDPROCESS processing callback for sound modules
//------------------------------------------------------------------------------
typedef void (__closure *LPFNSOUNDPROCESS)(vvf& vvfIn, vvf & vvfOut, bool& bIsLast);
//------------------------------------------------------------------------------
/// \typedef LPFNSOUNDPLAY onplay callback for sound modules
//------------------------------------------------------------------------------
typedef void (__closure *LPFNSOUNDPLAY)(vvf & vvfOut);

//------------------------------------------------------------------------------
/// \class SoundClassBase. Abstract base class for SoundMexPro  
//------------------------------------------------------------------------------
class SoundClassBase 
{
   public:
      AnsiString           m_strFatalError;
      SoundClassBase();
      virtual ~SoundClassBase(void);
      // virtual, non-abstract functions
      virtual void         SoundSetSaveProcessedCaptureData(bool b);
      virtual long         SoundGetLatency(Asio::Direction adDirection);

      // virtual, abstract functions
      virtual void         SoundInit(TStringList* psl) = 0;
      virtual bool         SoundInitialized() = 0;
      virtual void         SoundLoadDriverByIndex(size_t nIndex) = 0;
      virtual void         SoundLoadDriverByName(AnsiString strName) = 0;
      virtual void         SoundUnloadDriver() = 0;
      virtual size_t       SoundNumDrivers() = 0;
      virtual AnsiString   SoundDriverName(unsigned int iIndex) = 0;
      virtual unsigned int SoundCurrentDriverIndex(void) = 0;
      virtual long         SoundChannels(Asio::Direction adDirection) = 0;
      virtual AnsiString   SoundChannelName(unsigned int iChannelIndex, Asio::Direction adDirection) = 0;
      virtual void         SoundStart() = 0;
      virtual void         SoundStop(bool bWaitForStopDone = false) = 0;
      virtual bool         SoundIsRunning()   = 0;
      virtual bool         SoundIsStopping()  = 0;
      virtual size_t       SoundActiveChannels(Asio::Direction adDirection) = 0;
      virtual AnsiString   SoundGetActiveChannelName(unsigned int iChannelIndex, Asio::Direction adDirection) = 0;
      virtual long         SoundBufsizeCurrent()   = 0;
      virtual long         SoundBufsizeBest()      = 0;
      virtual long         SoundNumBufOut()   = 0;
      virtual void         SoundShowControlPanel(void);
      virtual void         SoundSetSampleRate(double dSampleRate) = 0;
      virtual bool         SoundCanSampleRate(double dSampleRate) = 0;
      virtual double       SoundGetSampleRate(void) = 0;
      virtual float        SoundActiveChannelMaxValue(Asio::Direction adDirection, size_t nChannel) = 0;
      virtual float        SoundActiveChannelMinValue(Asio::Direction adDirection, size_t nChannel) = 0;
      virtual unsigned int SoundGetWatchdogTimeout(void);
      virtual AnsiString   SoundFormatString() = 0;

      void                 SetOnHang(LPFNSOUNDVOIDFUNC lpfn);
      void                 SetOnStopComplete(LPFNSOUNDVOIDFUNC lpfn);
      void                 SetOnError(LPFNSOUNDVOIDFUNC lpfn);
      void                 SetOnStateChange(LPFNSOUNDSTATECHANGE lpfn);
      void                 SetOnXrun(LPFNSOUNDXRUN lpfn);
      void                 SetOnProcess(LPFNSOUNDPROCESS lpfn);
      void                 SetOnBufferPlay(LPFNSOUNDPLAY lpfn);
      void                 SetOnBufferDone(LPFNSOUNDPROCESS lpfn);
      unsigned int         ProcessCalls();
   protected:
      LPFNSOUNDVOIDFUNC    m_lpfnOnHang;
      LPFNSOUNDVOIDFUNC    m_lpfnOnStopComplete;
      LPFNSOUNDVOIDFUNC    m_lpfnOnError;
      LPFNSOUNDSTATECHANGE m_lpfnOnStateChange;
      LPFNSOUNDXRUN        m_lpfnOnXrun;
      LPFNSOUNDPROCESS     m_lpfnOnProcess;
      LPFNSOUNDPLAY        m_lpfnOnBufferPlay;
      LPFNSOUNDPROCESS     m_lpfnOnBufferDone;
      unsigned int         m_nProcessCalls;     
};
//------------------------------------------------------------------------------

#endif
