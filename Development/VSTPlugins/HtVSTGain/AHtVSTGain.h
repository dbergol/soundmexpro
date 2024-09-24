//------------------------------------------------------------------------------
/// \file AHtVSTGain.h
/// \author Berg
/// \brief Implementation of class CHtVSTGain. Simple VST dummy plugin with 8
/// inputs, 8 outputs. Does 'cyclic mixdown' to 8, 4, 2, or 1 channel
///
/// Project SoundMexPro
/// Module  HtVSTGain.dll
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
#ifndef AHtVSTGainH
#define AHtVSTGainH

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

#define MAX_CHANNELS    8
#define NUM_PROGRAMS    2
#define PROGRAMNAME_MAXLEN    255
//--------------------------------------------------------------------------

float dBToFactor(const float f4dB);
//--------------------------------------------------------------------------
///
//--------------------------------------------------------------------------
class CHtVSTGain : public AudioEffectX
{
   public:
      CHtVSTGain (audioMasterCallback audioMaster);
      ~CHtVSTGain ();
      bool m_bIsValid;

      // Processes
      virtual void process (float **inputs, float **outputs, VstInt32 sampleFrames);
      virtual void processReplacing (float **inputs, float **outputs, int sampleFrames);
      virtual void DoProcess (float **inputs, float **outputs, VstInt32 sampleFrames, bool bReplace);

      // Program
      virtual void setProgramName (char *name);
      virtual void getProgramName (char *name);
      virtual bool getProgramNameIndexed (VstInt32 category, VstInt32 index, char* text);

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
      float m_fGains[MAX_CHANNELS];
      float m_fGainsCalc[MAX_CHANNELS];
      char  m_lpszPrograms[2][PROGRAMNAME_MAXLEN];

};
//--------------------------------------------------------------------------
#endif // AHtVSTGainH
