//------------------------------------------------------------------------------
/// \file AHtVSTEqualizer.h
/// \author Berg
/// \brief Implementation of class CHtVSTEqualizer.
///
/// Project SoundMexPro
/// Module  HtVSTEqualizer.dll
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
#ifndef __AHTVSTVISUALIZE_H
#define __AHTVSTVISUALIZE_H

#include <vcl.h>
#include "audioeffectx.h"
#include "FloatOlaFlt.h"
//--------------------------------------------------------------------------
int         round(float f4Value);
float       dBToFactor(const float f4dB);
float       FactorTodB(const float f4Factor);


class TfrmEqInput;
//--------------------------------------------------------------------------
///
//--------------------------------------------------------------------------
class CHtVSTEqualizer : public AudioEffectX
{
   public:
      CHtVSTEqualizer (audioMasterCallback audioMaster);
      ~CHtVSTEqualizer ();
      bool m_bIsValid;
      int  m_nUpdateInterval;


      // Processes
      virtual void process (float **inputs, float **outputs, long sampleFrames);
      virtual void processReplacing (float **inputs, float **outputs, long sampleFrames);
      virtual void DoProcess (float **inputs, float **outputs, long sampleFrames, bool bReplace);

      // Program
      virtual void setProgramName (char *name);
      virtual void getProgramName (char *name);

      // Parameters
      virtual void setParameter (long index, float value);
      virtual float getParameter (long index);
      virtual void getParameterLabel (long index, char *label);
      virtual void getParameterDisplay (long index, char *text);
      virtual void getParameterName (long index, char *text);

      virtual bool getEffectName (char* name);
      virtual bool getVendorString (char* text);
      virtual bool getProductString (char* text);
      virtual long getVendorVersion () { return 1000; }
      virtual VstPlugCategory getPlugCategory () { return kPlugCategEffect; }

      virtual long startProcess ();
      virtual long stopProcess ();
      virtual void resume ();
      virtual void suspend ();
      void ReadFilterNameFromIni();
      virtual bool __fastcall LoadFilter(AnsiString sFileName);
      virtual bool __fastcall SaveFilter(AnsiString sFileName);
      virtual bool __fastcall ChartToFilter();
      virtual void __fastcall FilterToChart();

      unsigned int            m_nFFTLen;
      unsigned int            m_nWindowLen;
      TMMFloatOlaWindowFeed   m_nFWindowFeed;
      bool                    m_bComplex;
      float                   m_fEnabled;
      float                   m_fVisible;
      float                   m_fEdit;
      bool                    m_bMuted;
      TMemoryStream*          m_pmsFilter;

   protected:
      TfrmEqInput*            m_pfrmVisual;
      _RTL_CRITICAL_SECTION   csDataSection;
      AnsiString              m_strFilterFile;
      float*                  m_pfBuffer;       // buffer for interleaved data
      TMemoryStream*          m_pmsFileFilter;
      PFloatOlaFlt            m_pFilter;

      void InitFilter();
      void InitOLA();
      void ExitOLA();
      virtual void __fastcall ResampleFilter();
      virtual void __fastcall FloatOlaSpectrumWaveReady(TfCplx *lpSpectrum,
            DWORD dwBins, float *lpWaveData, DWORD dwSamples, bool &bUseWave);
};
#endif
