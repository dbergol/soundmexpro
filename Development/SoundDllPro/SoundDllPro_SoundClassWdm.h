//------------------------------------------------------------------------------
/// \file SoundDllPro_SoundClassWdm.h
/// \author Berg
/// \brief Interface of sound class for SoundMexPro. Inherits form
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
#ifndef SoundDllPro_SoundClassWdmH
#define SoundDllPro_SoundClassWdmH
//---------------------------------------------------------------------------

#include "SoundDllPro_SoundClassBase.h"
#include <casioenums.h>
#include "MMWavOut.hpp"
#include "MMRingBf.hpp"


//------------------------------------------------------------------------------
/// \class SoundClassWdm. Inherits SoundClassBase.
/// Implements all abstract functions for use with Wdm
//------------------------------------------------------------------------------
class SoundClassWdm : public SoundClassBase
{
   private:
      unsigned int               m_nSamplesPlayed;
      unsigned int               m_nSampleStopPos;
      CRITICAL_SECTION           m_csProcess;         ///< critical section processing
      // device members
      TMMWaveOut*                m_pwo;               ///< MMTools wave out component
      TMMRingBuffer*             m_prb;               ///< MMTools ring buffer component
      tWAVEFORMATEX              m_wfx;               ///< internal wave format
      vvf                        m_vvfBuffer;         ///< internal float buffer
      vvf                        m_vvfDummy;         ///< internal float buffer
      bool                       m_bInitialized;
      TStringList*               m_pslDeviceNames;
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
      void              DeleteDevice();
      void              CreateDevice();
      // callbacks for TMMWaveOut
      void  __fastcall  OnStop(TObject *Sender);
      void  __fastcall  OnStart(TObject *Sender);
      void  __fastcall  WdmProcess(TObject *Sender, PWaveHdr lpWaveHdr);
      void  __fastcall  WdmOnFatalError(TObject *Sender);
      void __fastcall   OnXRun(TObject *Sender);
      void __fastcall   OnError(TObject *Sender);
   public:
      SoundClassWdm();
      ~SoundClassWdm();
      virtual void         SoundInit(TStringList* psl);
      virtual bool         SoundInitialized();
      virtual void         SoundLoadDriverByIndex(size_t nIndex);
      virtual void         SoundLoadDriverByName(AnsiString strName);
      virtual void         SoundUnloadDriver();
      virtual size_t       SoundNumDrivers();
      virtual AnsiString   SoundDriverName(unsigned int iIndex);
      virtual unsigned int SoundCurrentDriverIndex(void);
      virtual size_t       SoundChannels(Direction adDirection);
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
      virtual void         SoundSetSampleRate(double dSampleRate);
      virtual bool         SoundCanSampleRate(double dSampleRate);
      virtual double       SoundGetSampleRate(void);
      virtual float        SoundActiveChannelMaxValue(Direction adDirection, size_t nChannel);
      virtual float        SoundActiveChannelMinValue(Direction adDirection, size_t nChannel);
      virtual AnsiString   SoundFormatString();

};
//------------------------------------------------------------------------------

#endif
