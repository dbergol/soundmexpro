//------------------------------------------------------------------------------
/// \file AHtVSTGain.cpp
/// \author Berg
/// \brief Implementation of class CHtVSTGain. Simple VST dummy plugin with
/// 8 inputs, 8 outputs applying simple gains
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
#include "AHtVSTGain.h"
#include <stdio.h>
#include <windows.h>
#include <math.h>
//--------------------------------------------------------------------------

//--------------------------------------------------------------------------
/// dB to linear factor conversion
//--------------------------------------------------------------------------
float dBToFactor(const float f4dB)
{
   return (float)pow(10,f4dB/20);
}
//--------------------------------------------------------------------------

//--------------------------------------------------------------------------
/// constructor: calls base class and initializes members
//--------------------------------------------------------------------------
CHtVSTGain::CHtVSTGain (audioMasterCallback audioMaster)
   :  AudioEffectX (audioMaster, 2, MAX_CHANNELS), // two programs, MAX_CHANNELS parameters (gains)
      m_bIsValid(false)
{
   setNumInputs (MAX_CHANNELS);
   setNumOutputs (MAX_CHANNELS);
   setUniqueID (CCONST('H','t','G','n'));
   canProcessReplacing ();    // supports both accumulating and replacing output

   // set default gain (factor) to 1
   for (int i = 0; i < MAX_CHANNELS; i++)
      m_fGains[i] = 1.0f;

   strcpy (m_lpszPrograms[0], "lin");
   strcpy (m_lpszPrograms[1], "log");

   // set 'valid flag' (used in _main)



   m_bIsValid = true;
}
//--------------------------------------------------------------------------

//--------------------------------------------------------------------------
/// destructor
//--------------------------------------------------------------------------
CHtVSTGain::~CHtVSTGain ()
{
   // nothing to do
}
//--------------------------------------------------------------------------

//--------------------------------------------------------------------------
/// sets program name
//--------------------------------------------------------------------------
void CHtVSTGain::setProgramName (char *name)
{
   if (0 == strcmp(name, m_lpszPrograms[1]))
      curProgram = 1;
   else
      curProgram = 0;
}
//--------------------------------------------------------------------------

//--------------------------------------------------------------------------
/// returns actual program name
//--------------------------------------------------------------------------
void CHtVSTGain::getProgramName (char *name)
{
   strcpy (name, m_lpszPrograms[curProgram]);
}
//--------------------------------------------------------------------------

//--------------------------------------------------------------------------
/// returns program name by index
//--------------------------------------------------------------------------
#pragma argsused
bool CHtVSTGain::getProgramNameIndexed (VstInt32 category, VstInt32 index, char* text)
{
   if (index >= 0 && index < NUM_PROGRAMS)
      {
      strcpy (text, m_lpszPrograms[index]);
      return true;
      }
   return false;
}
//--------------------------------------------------------------------------

//--------------------------------------------------------------------------
/// sets a parameter value
//--------------------------------------------------------------------------
void CHtVSTGain::setParameter (VstInt32 index, float value)
{
   if (index >= 0 && index < MAX_CHANNELS)
      m_fGains[index] = value;
}
//--------------------------------------------------------------------------

//--------------------------------------------------------------------------
/// returns a parameter value
//--------------------------------------------------------------------------
float CHtVSTGain::getParameter (VstInt32 index)
{
   if (index >= 0 && index < MAX_CHANNELS)
      return m_fGains[index];
   return 0.0f;
}
//--------------------------------------------------------------------------

//--------------------------------------------------------------------------
/// returns a parameter name
//--------------------------------------------------------------------------
void CHtVSTGain::getParameterName (VstInt32 index, char *label)
{
   if (index >= 0 && index < MAX_CHANNELS)
      sprintf(label, "gain_%d", index);
}
//--------------------------------------------------------------------------

//--------------------------------------------------------------------------
/// returns a parameter display value
//--------------------------------------------------------------------------
void CHtVSTGain::getParameterDisplay (VstInt32 index, char *text)
{
   if (index >= 0 && index < MAX_CHANNELS)
      {
      if (curProgram == 1)
         sprintf(text, "%.2f", 50.0 * (double)m_fGains[index] - 50.0);
      else
         sprintf(text, "%.2f", (double)m_fGains[index]);
      }

}
//--------------------------------------------------------------------------

//--------------------------------------------------------------------------
/// returns a parameter  label (e.g. unit)
//--------------------------------------------------------------------------
#pragma argsused
void CHtVSTGain::getParameterLabel(VstInt32 index, char *label)
{
   if (curProgram == 1)
      strcpy (label, "dB");
   else
      strcpy (label, "factor");
}
//--------------------------------------------------------------------------

//--------------------------------------------------------------------------
/// returns effect name
//--------------------------------------------------------------------------
bool CHtVSTGain::getEffectName (char* name)
{
   strcpy (name, "Simple Gain");
   return true;
}
//--------------------------------------------------------------------------

//--------------------------------------------------------------------------
/// returns effect product string
//--------------------------------------------------------------------------
bool CHtVSTGain::getProductString (char* text)
{
   strcpy (text, "HtVSTGain");
   return true;
}
//--------------------------------------------------------------------------

//--------------------------------------------------------------------------
/// returns effect vendor string
//--------------------------------------------------------------------------
bool CHtVSTGain::getVendorString (char* text)
{
   strcpy (text, "Uni Ol");
   return true;
}
//--------------------------------------------------------------------------

//--------------------------------------------------------------------------
/// Processing routine called by process and processReplacing
//--------------------------------------------------------------------------
void CHtVSTGain::DoProcess (float **inputs, float **outputs, VstInt32 sampleFrames, bool bReplace)
{
    for (int nIndex = 0; nIndex < MAX_CHANNELS; nIndex++)
      {
      if (curProgram == 1)
         m_fGainsCalc[nIndex] = dBToFactor(50.0f * m_fGains[nIndex] - 50.0f);
      else
         m_fGainsCalc[nIndex] = m_fGains[nIndex];

      if (!outputs[nIndex] || !inputs[nIndex])
         continue;
      if (bReplace)
         memset(outputs[nIndex], 0, (size_t)sampleFrames * sizeof (float));
      for (int nSampleIndex = 0; nSampleIndex < sampleFrames; nSampleIndex++)
         {
         outputs[nIndex][nSampleIndex] += inputs[nIndex][nSampleIndex] * m_fGainsCalc[nIndex];
         }
      }
}
//--------------------------------------------------------------------------

//--------------------------------------------------------------------------
/// accumulating processing routine                                                                   
//--------------------------------------------------------------------------
void CHtVSTGain::process (float **inputs, float **outputs, VstInt32 sampleFrames)
{
   DoProcess(inputs, outputs, sampleFrames, false);
}
//--------------------------------------------------------------------------

//--------------------------------------------------------------------------
/// replacing processing routine
//--------------------------------------------------------------------------
void CHtVSTGain::processReplacing (float **inputs, float **outputs, int sampleFrames)
{
   DoProcess(inputs, outputs, sampleFrames, true);
}
//--------------------------------------------------------------------------





