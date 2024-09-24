//------------------------------------------------------------------------------
/// \file AHtVSTEq.h
/// \author Berg
/// \brief Implementation of class CHtVSTEq.
///
/// Project SoundMexPro
/// Module  HtVSTEq.dll
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
#ifndef AHtVSTEqH
#define AHtVSTEqH

#include <vcl.h>
#include <inifiles.hpp>

// avoid warnings from VST-SDK
#pragma clang diagnostic push
#pragma clang diagnostic ignored "-Wreserved-id-macro"
#pragma clang diagnostic ignored "-Wunused-parameter"
#pragma clang diagnostic ignored "-Wundef"
#pragma clang diagnostic ignored "-Wshadow"
#pragma clang diagnostic ignored "-Wdocumentation-unknown-command"
#include "audioeffectx.h"
#pragma clang diagnostic pop

#include "HtOLA.h"

#define HT_VST_VENDOR_USERCONFIG    1234
#define HT_VST_VENDOR_PROCERROR     HT_VST_VENDOR_USERCONFIG+1
#define HT_VST_VENDOR_INTERNALNAME  HT_VST_VENDOR_USERCONFIG+100


//--------------------------------------------------------------------------
float       dBToFactor(const float f4dB);
float       FactorTodB(const float f4Factor);
void        EnsureFFTW();


enum TInterpolMode {
   IM_LIN_LIN = 0,
   IM_LOG_LIN,
   IM_LIN_DB,
   IM_LOG_DB
};

class TfrmEqInput;
//--------------------------------------------------------------------------
///
//--------------------------------------------------------------------------
class CHtVSTEq : public AudioEffectX
{
   public:
      CHtVSTEq (audioMasterCallback audioMaster);
      ~CHtVSTEq ();
      bool m_bIsValid;
      int  m_nUpdateInterval;


      // Processes
      virtual void process (float **inputs, float **outputs, VstInt32 sampleFrames);
      virtual void processReplacing (float **inputs, float **outputs, VstInt32 sampleFrames);
      virtual void DoProcess (float **inputs, float **outputs, VstInt32 sampleFrames, bool bReplace);

      // Program
      virtual void setProgramName (char *name);
      virtual void getProgramName (char *name);

      // Parameters
      virtual void setParameter (VstInt32 index, float value);
      virtual float getParameter (VstInt32 index);
      virtual void getParameterLabel (VstInt32 index, char *label);
      virtual void getParameterDisplay (VstInt32 index, char *text);
      virtual void getParameterName (VstInt32 index, char *text);

      virtual bool getEffectName (char* name);
      virtual bool getVendorString (char* text);
      virtual bool getProductString (char* text);
      virtual VstInt32 getVendorVersion () { return 1000; }
      virtual VstPlugCategory getPlugCategory () { return kPlugCategEffect; }

      virtual VstInt32 startProcess ();
      virtual VstInt32 stopProcess ();
      virtual void resume ();
      virtual void suspend ();
      void ReadFilterNameFromIni();
      virtual bool __fastcall LoadFilterFile(AnsiString sFileName);
      virtual bool __fastcall SaveFilter(AnsiString sFileName);
      virtual bool __fastcall LoadFilter(AnsiString sSectionName);
      virtual bool __fastcall ChartToFilter();
      virtual void __fastcall FilterToChart();

      bool                    m_bLog;
      bool                    m_bWriteDebug;
      unsigned int            m_nFFTLen;
      bool                    m_bComplex;
      TInterpolMode           m_imInterpolMode;
      float                   m_fEnabled;
      float                   m_fVisible;
      float                   m_fEdit;
      bool                    m_bMuted;
      vvac                    m_vvacFilter;
      vvac                    m_vvacSpecPreFilter;
      vvac                    m_vvacSpecPostFilter;
      vvac                    m_vvacSpecPreFilterDraw;
      vvac                    m_vvacSpecPostFilterDraw;

      void                    Update();

   protected:
      unsigned int            m_nNumChannels;
      TfrmEqInput*            m_pfrmVisual;
      _RTL_CRITICAL_SECTION   csDataSection;
      AnsiString              m_strFilterFile;
      TMemoryStream*          m_pmsFileFilter;
      vvaf                    m_vvafBuffer;
      CHtOLA*                 m_pOLA;
      float                   m_fVisSpecFactor;

      TMemIniFile *m_pIni;
      void InitFilter();
      void InitOLA();
      void ExitOLA();
      virtual void __fastcall ResampleFilter();
      void SpecProcessCallback(vvac & vvacSpectrum);
};
//--------------------------------------------------------------------------
#endif // AHtVSTEqH
