// ------------------------------------------------------------------------------
/// \file AHtVSTSine.cpp
/// \author Berg
/// \brief Implementation of class CHtVSTSine.
///
/// Project SoundMexPro
/// Module  HtVSTSine.dll
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
// ------------------------------------------------------------------------------
#include "AHtVSTSine.h"
#include <stdio.h>
#include <windows.h>
#include <math.h>
// --------------------------------------------------------------------------

// --------------------------------------------------------------------------
/// constructor: calls base class and initializes members
// --------------------------------------------------------------------------
CHtVSTSine::CHtVSTSine(audioMasterCallback audioMaster)
    : AudioEffectX(audioMaster, 1, 2), // progams/parameters
    m_bIsValid(false), m_fEnabled(1.0f)

{
   setNumInputs(1);
   setNumOutputs(1);
   setUniqueID(CCONST('H', 't', 'S', 'i'));
   canProcessReplacing(); // supports both accumulating and replacing output

   m_dFreqNormalized = 2.0 * 440.0 / (double)sampleRate;

   // set 'valid flag' (used in _main)
   m_bIsValid = true;
}
// --------------------------------------------------------------------------

// --------------------------------------------------------------------------
/// destructor
// --------------------------------------------------------------------------
CHtVSTSine::~CHtVSTSine() 
{
   // nothing to do
}
// --------------------------------------------------------------------------

// --------------------------------------------------------------------------
/// returns actual program name
// --------------------------------------------------------------------------
void CHtVSTSine::getProgramName(char *name) 
{
   strcpy(name, "default");
}
// --------------------------------------------------------------------------

// --------------------------------------------------------------------------
/// sets a parameter value
// --------------------------------------------------------------------------
void CHtVSTSine::setParameter(VstInt32 index, float value) 
{

   switch (index) 
      {
      case 0:
         m_fEnabled = value;
         break;
      case 1:
         m_dFreqNormalized = (double)value;
         break;
      }
}
// --------------------------------------------------------------------------

// --------------------------------------------------------------------------
/// returns a parameter value
// --------------------------------------------------------------------------
float CHtVSTSine::getParameter(VstInt32 index) 
{
   switch (index) 
      {
      case 0:
         return m_fEnabled;
      default:
         return (float)m_dFreqNormalized;
      }
}
// --------------------------------------------------------------------------

// --------------------------------------------------------------------------
/// returns a parameter name
// --------------------------------------------------------------------------
void CHtVSTSine::getParameterName(VstInt32 index, char *label) 
{
   switch (index) 
      {
      case 0:
         strcpy(label, "enabled");
         break;
      case 1:
         strcpy(label, "frequency");
         break;
      }

}
// --------------------------------------------------------------------------

// --------------------------------------------------------------------------
/// returns a parameter display value
// --------------------------------------------------------------------------
void CHtVSTSine::getParameterDisplay(VstInt32 index, char *text) 
{
   switch (index) 
      {
      case 0:
         sprintf(text, m_fEnabled > 0.5f ? "true" : "false");
         break;

      default:
         sprintf(text, "%.1f", m_dFreqNormalized*(double)sampleRate / 2.0);
         break;
      }
}
// --------------------------------------------------------------------------

// --------------------------------------------------------------------------
/// returns a parameter  label (e.g. unit)
// --------------------------------------------------------------------------
#pragma argsused

void CHtVSTSine::getParameterLabel(VstInt32 index, char *label) 
{
   switch (index) 
      {
      case 0:
         strcpy(label, " ");
         break;
      default:
         strcpy(label, "Hz");
         break;
      }
}
// --------------------------------------------------------------------------

// --------------------------------------------------------------------------
/// returns effect name
// --------------------------------------------------------------------------
bool CHtVSTSine::getEffectName(char* name) 
{
   strcpy(name, "HtVSTSine generator");
   return true;
}
// --------------------------------------------------------------------------

// --------------------------------------------------------------------------
/// returns effect product string
// --------------------------------------------------------------------------
bool CHtVSTSine::getProductString(char* text) 
{
   strcpy(text, "HtVSTSine");
   return true;
}
// --------------------------------------------------------------------------

// --------------------------------------------------------------------------
/// returns effect vendor string
// --------------------------------------------------------------------------
bool CHtVSTSine::getVendorString(char* text) 
{
   strcpy(text, "Uni Ol");
   return true;
}
// --------------------------------------------------------------------------

// --------------------------------------------------------------------------
/// inits ola filter
// --------------------------------------------------------------------------
VstInt32 CHtVSTSine::startProcess() 
{
   return 0;
}
// --------------------------------------------------------------------------

// --------------------------------------------------------------------------
///
// --------------------------------------------------------------------------
void CHtVSTSine::resume() 
{
}
// --------------------------------------------------------------------------

// --------------------------------------------------------------------------
/// Processing routine called by process and processReplacing
// --------------------------------------------------------------------------
#pragma argsused
void CHtVSTSine::DoProcess(float **inputs, float **outputs, VstInt32 sampleFrames, bool bReplace) 
{
   if (m_fEnabled <= 0.5f || !outputs[0])
      return;
   m_dPhaseInc = m_dFreqNormalized / 2.0;

   for (int nSampleIndex = 0; nSampleIndex < sampleFrames; nSampleIndex++)
      {
      m_dPhase += m_dPhaseInc;
      m_dPhase = m_dPhase - floor(m_dPhase);
      outputs[0][nSampleIndex] = (float)(sin(2*M_PI*m_dPhase) * 0.5);
      }
}
// --------------------------------------------------------------------------

// --------------------------------------------------------------------------
/// accumulating processing routine
// --------------------------------------------------------------------------
void CHtVSTSine::process(float **inputs, float **outputs, VstInt32 sampleFrames)
{
   DoProcess(inputs, outputs, sampleFrames, false);
}
// --------------------------------------------------------------------------

// --------------------------------------------------------------------------
/// replacing processing routine
// --------------------------------------------------------------------------
void CHtVSTSine::processReplacing(float **inputs, float **outputs, VstInt32 sampleFrames) 
{
   DoProcess(inputs, outputs, sampleFrames, true);
}
// --------------------------------------------------------------------------


