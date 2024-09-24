//------------------------------------------------------------------------------
/// \file AHtVSTHeteroDyning.h
/// \author Berg
/// \brief Implementation of class CHtVSTHeteroDyning. 
///
/// Project SoundMexPro
/// Module  HtVSTHeteroDyning.dll
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
#ifndef AHtVSTHeteroDyningH
#define AHtVSTHeteroDyningH

// avoid warnings from VST-SDK
#pragma clang diagnostic push
#pragma clang diagnostic ignored "-Wreserved-id-macro"
#pragma clang diagnostic ignored "-Wunused-parameter"
#pragma clang diagnostic ignored "-Wundef"
#pragma clang diagnostic ignored "-Wshadow"
#pragma clang diagnostic ignored "-Wdocumentation-unknown-command"
#include "audioeffectx.h"
#pragma clang diagnostic pop

#include "string.h"

#define MAX_CHANNELS    2
//--------------------------------------------------------------------------



//--------------------------------------------------------------------------
///
//--------------------------------------------------------------------------
class CHtVSTHeteroDyning : public AudioEffectX
{
   public:
      CHtVSTHeteroDyning (audioMasterCallback audioMaster);
      ~CHtVSTHeteroDyning ();
      bool m_bIsValid;

      virtual VstInt32 startProcess ();
      virtual void resume ();

      // Processes
      virtual void process (float **inputs, float **outputs, VstInt32 sampleFrames);
      virtual void processReplacing (float **inputs, float **outputs, VstInt32 sampleFrames);
      virtual void DoProcess (float **inputs, float **outputs, VstInt32 sampleFrames, bool bReplace);

      // Program
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

   protected:
      float m_faModFreq[MAX_CHANNELS];
      float m_fEnabled;
      float m_faPhase[MAX_CHANNELS];
      double m_daPhaseIncrementBy2pi[MAX_CHANNELS];
      void Init(int nIndex = -1);

};
//--------------------------------------------------------------------------
#endif // AHtVSTHeteroDyningH
