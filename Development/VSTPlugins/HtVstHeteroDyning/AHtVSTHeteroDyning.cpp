//------------------------------------------------------------------------------
/// \file AHtVSTHeteroDyning.cpp
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
#include "AHtVSTHeteroDyning.h"
#include <stdio.h>
#include <windows.h>
#include <math.h>
//--------------------------------------------------------------------------



//--------------------------------------------------------------------------
/// constructor: calls base class and initializes members
//--------------------------------------------------------------------------
CHtVSTHeteroDyning::CHtVSTHeteroDyning (audioMasterCallback audioMaster)
   :  AudioEffectX (audioMaster, 1, MAX_CHANNELS + 1), // progams/parameters
      m_bIsValid(false),
      m_fEnabled(1.0f)

{
   setNumInputs (MAX_CHANNELS);
   setNumOutputs (MAX_CHANNELS);
   #ifndef NO_HT_RESTRICTION
   setUniqueID (CCONST('H','t','H','d'));
   #else
   setUniqueID (CCONST('H','t','D','y'));
   #endif
   canProcessReplacing ();    // supports both accumulating and replacing output


   for (int i = 0; i < MAX_CHANNELS; i++)
      m_faModFreq[i] = 4.0f/44100.0f;
   Init();

   // set 'valid flag' (used in _main)
   m_bIsValid = true;
}
//--------------------------------------------------------------------------

//--------------------------------------------------------------------------
/// destructor
//--------------------------------------------------------------------------
CHtVSTHeteroDyning::~CHtVSTHeteroDyning ()
{
   // nothing to do
}
//--------------------------------------------------------------------------


//--------------------------------------------------------------------------
/// returns actual program name
//--------------------------------------------------------------------------
void CHtVSTHeteroDyning::getProgramName (char *name)
{
   strcpy (name, "default");
}
//--------------------------------------------------------------------------


//--------------------------------------------------------------------------
/// sets a parameter value
//--------------------------------------------------------------------------
void CHtVSTHeteroDyning::setParameter (VstInt32 index, float value)
{

   switch (index)
      {
      case 0:  m_fEnabled = value; break;
      default: m_faModFreq[index-1] = value; Init(index-1);break;
      }
}
//--------------------------------------------------------------------------

//--------------------------------------------------------------------------
/// returns a parameter value
//--------------------------------------------------------------------------
float CHtVSTHeteroDyning::getParameter (VstInt32 index)
{
   switch (index)
      {
      case 0:     return m_fEnabled;
      default:    return m_faModFreq[index-1];
      }
}
//--------------------------------------------------------------------------

//--------------------------------------------------------------------------
/// returns a parameter name
//--------------------------------------------------------------------------
void CHtVSTHeteroDyning::getParameterName (VstInt32 index, char *label)
{
   switch (index)
      {
      case 0:     strcpy (label, "enabled"); break;
      default:    sprintf(label, "frequency_%d", index-1); break;
      }

}
//--------------------------------------------------------------------------

//--------------------------------------------------------------------------
/// returns a parameter display value
//--------------------------------------------------------------------------
void CHtVSTHeteroDyning::getParameterDisplay (VstInt32 index, char *text)
{
   switch (index)
      {
      case 0:  sprintf(text, m_fEnabled > 0.5f ? "true" : "false");break;
      default: sprintf(text, "%.1f", (double)(m_faModFreq[index-1]*sampleRate)); break;
      }
}
//--------------------------------------------------------------------------

//--------------------------------------------------------------------------
/// returns a parameter  label (e.g. unit)
//--------------------------------------------------------------------------
#pragma argsused
void CHtVSTHeteroDyning::getParameterLabel(VstInt32 index, char *label)
{
   switch (index)
      {
      case 0:  strcpy (label, " "); break;
      default:  strcpy (label, "Hz"); break;
      }
}
//--------------------------------------------------------------------------

//--------------------------------------------------------------------------
/// returns effect name
//--------------------------------------------------------------------------
bool CHtVSTHeteroDyning::getEffectName (char* name)
{
   strcpy (name, "HeteroDyning filter");
   return true;
}
//--------------------------------------------------------------------------

//--------------------------------------------------------------------------
/// returns effect product string
//--------------------------------------------------------------------------
bool CHtVSTHeteroDyning::getProductString (char* text)
{
   strcpy (text, "HtVSTHeteroDyning");
   return true;
}
//--------------------------------------------------------------------------

//--------------------------------------------------------------------------
/// returns effect vendor string
//--------------------------------------------------------------------------
bool CHtVSTHeteroDyning::getVendorString (char* text)
{
   strcpy (text, "Uni Ol");
   return true;
}
//--------------------------------------------------------------------------


//--------------------------------------------------------------------------
/// inits ola filter
//--------------------------------------------------------------------------
VstInt32 CHtVSTHeteroDyning::startProcess ()
{
   Init();
   return 0;
}
//--------------------------------------------------------------------------

//--------------------------------------------------------------------------
///
//--------------------------------------------------------------------------
void CHtVSTHeteroDyning::resume ()
{
   Init();
}
//--------------------------------------------------------------------------

//--------------------------------------------------------------------------
/// Processing routine called by process and processReplacing
//--------------------------------------------------------------------------
void CHtVSTHeteroDyning::DoProcess (float **inputs, float **outputs, VstInt32 sampleFrames, bool bReplace)
{
    for (int nIndex = 0; nIndex < MAX_CHANNELS; nIndex++)
      {
      if (!outputs[nIndex] || !inputs[nIndex])
         continue;

      if (m_fEnabled <= 0.5f)
         {
         MoveMemory(outputs[nIndex], inputs[nIndex], (size_t)sampleFrames*sizeof(float));
         continue;
         }
      else if (bReplace)
         memset(outputs[nIndex], 0, (size_t)sampleFrames * sizeof (float));

      }
    if (m_fEnabled <= 0.5f)
      return;
   for (int nSampleIndex = 0; nSampleIndex < sampleFrames; nSampleIndex++)
      {
      for (int nIndex = 0; nIndex < MAX_CHANNELS; nIndex++)
         {
         if (!outputs[nIndex] || !inputs[nIndex])
            continue;
         m_faPhase[nIndex] += m_daPhaseIncrementBy2pi[nIndex];
         m_faPhase[nIndex] = m_faPhase[nIndex] - floor(m_faPhase[nIndex]);
         outputs[nIndex][nSampleIndex] += inputs[nIndex][nSampleIndex] * (float)sin(2.0*M_PI*(double)m_faPhase[nIndex]);
         }
      }
}
//--------------------------------------------------------------------------

//--------------------------------------------------------------------------
/// accumulating processing routine
//--------------------------------------------------------------------------
void CHtVSTHeteroDyning::process (float **inputs, float **outputs, VstInt32 sampleFrames)
{
   DoProcess(inputs, outputs, sampleFrames, false);
}
//--------------------------------------------------------------------------

//--------------------------------------------------------------------------
/// replacing processing routine
//--------------------------------------------------------------------------
void CHtVSTHeteroDyning::processReplacing (float **inputs, float **outputs, VstInt32 sampleFrames)
{
   DoProcess(inputs, outputs, sampleFrames, true);
}
//--------------------------------------------------------------------------

//--------------------------------------------------------------------------
/// initializes phases ind increments
//--------------------------------------------------------------------------
void CHtVSTHeteroDyning::Init(int nIndex )
{
   if (nIndex != -1)
      {
      m_faPhase[nIndex] = 0.0f;
      m_daPhaseIncrementBy2pi[nIndex] = (double)m_faModFreq[nIndex];
      return;
      }

   for (int i = 0; i < MAX_CHANNELS; i++)
      {
      m_faPhase[i] = 0.0f;
      m_daPhaseIncrementBy2pi[i] = (double)m_faModFreq[i];
      }
}
//--------------------------------------------------------------------------

