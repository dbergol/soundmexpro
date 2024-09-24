//------------------------------------------------------------------------------
/// \file AHtVSTConv.h
/// \author Berg
/// \brief Implementation of class CHtVSTConvolver.
///
/// Project SoundMexPro
/// Module  HtVSTConv.dll
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
#ifndef AHtVSTConvH
#define AHtVSTConvH

#include <vcl.h>

// avoid warnings from VST-SDK
#pragma clang diagnostic push
#pragma clang diagnostic ignored "-Wreserved-id-macro"
#pragma clang diagnostic ignored "-Wunused-parameter"
#pragma clang diagnostic ignored "-Wundef"
#pragma clang diagnostic ignored "-Wshadow"
#pragma clang diagnostic ignored "-Wdocumentation-unknown-command"
#include "audioeffectx.h"
#pragma clang diagnostic pop

#include "WaveFileReader.h"
#include "ConvLibDefines.h"

#define MAX_CHANNELS    2


void EnsureFFTW();


extern HINSTANCE__* hInstance;

//------------------------------------------------------------------------------
/// CHtVSTConvolver VST plugin for fast parttitioned convolution
//------------------------------------------------------------------------------
class CHtVSTConvolver : public AudioEffectX
{
   public:
      CHtVSTConvolver (audioMasterCallback audioMaster);
      ~CHtVSTConvolver ();

      bool         m_bIsValid;

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


      virtual void setBlockSize (VstInt32 nBlockSize);

      virtual VstPlugCategory getPlugCategory () { return kPlugCategEffect; }

   private:
      _RTL_CRITICAL_SECTION   m_csDataSection;
      void*                   m_hConvolver;
      float                   m_fGain;
      float                   m_fEnabled;
      bool                    m_bSkip;
      HINSTANCE               m_hLibrary;
      AnsiString              m_strImpulseResponse;
      LPFNCONVINIT            m_lpfnConvInit;
      LPFNCONVEXIT            m_lpfnConvExit;
      LPFNCONVPROCESS         m_lpfnConvProcess;
      void                    InitConv(AnsiString str);
      void                    ExitConv();

};
//------------------------------------------------------------------------------
#endif // AHtVSTConvH
