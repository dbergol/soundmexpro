//------------------------------------------------------------------------------
/// \file AHtVSTVisualize.cpp
/// \author Berg
/// \brief Implementation of class CHtVSTVisualize.
///
/// Project SoundMexPro
/// Module  HtVSTVisualize.dll
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
#include "AHtVSTVisualize.h"
#include <stdio.h>
#include <windows.h>
#include <math.h>
#include "visualform.h"
//--------------------------------------------------------------------------




//---------------------------------------------------------------------------
//---------------------------------------------------------------------------
int round(float f4Value)
{
if (  fabs(f4Value - floor(f4Value)) < 0.5)
   return (int)floor(f4Value);
else
   return (int)ceil(f4Value);
}
//---------------------------------------------------------------------------


//--------------------------------------------------------------------------
/// constructor: calls base class and initializes members
//--------------------------------------------------------------------------
CHtVSTVisualize::CHtVSTVisualize (audioMasterCallback audioMaster)
   :  AudioEffectX (audioMaster, 1, 1), // one programs, two parameters
      m_bIsValid(false),
      m_pfrmVisual(NULL),
      m_fVisible(1.0f)
{
   try
      {
      setNumInputs (1);
      setNumOutputs (1);
      setUniqueID (CCONST('H','t','V','i'));
      canProcessReplacing ();    // supports both accumulating and replacing output

      m_pfrmVisual = new TfrmVisual(NULL);
      m_pfrmVisual->Show();

      // set 'valid flag' (used in _main)
      m_bIsValid = true;
      }
   catch (...)
      {
      if (m_pfrmVisual)
         delete m_pfrmVisual;
      m_pfrmVisual = NULL;

      }
}
//--------------------------------------------------------------------------

//--------------------------------------------------------------------------
/// destructor
//--------------------------------------------------------------------------
CHtVSTVisualize::~CHtVSTVisualize ()
{
      if (m_pfrmVisual)
         delete m_pfrmVisual;
      m_pfrmVisual = NULL;
}
//--------------------------------------------------------------------------

//--------------------------------------------------------------------------
/// sets program name
//--------------------------------------------------------------------------
void CHtVSTVisualize::setProgramName (char *name)
{
   if (!m_pfrmVisual)
      return;
   m_pfrmVisual->ReadIni(name);
}
//--------------------------------------------------------------------------

//--------------------------------------------------------------------------
/// returns actual program name
//--------------------------------------------------------------------------
void CHtVSTVisualize::getProgramName (char *name)
{
   if (!m_pfrmVisual)
      return;
   if (m_pfrmVisual->m_strIniFile.IsEmpty())
      strcpy (name, "(empty)");
   else
      strcpy (name, AnsiString(ExtractFileName(m_pfrmVisual->m_strIniFile)).c_str());
}
//--------------------------------------------------------------------------


//--------------------------------------------------------------------------
/// sets a parameter value
//--------------------------------------------------------------------------
#pragma argsused
void CHtVSTVisualize::setParameter (long index, float value)
{
   if (!m_pfrmVisual)
      return;
   m_fVisible = value;
   m_pfrmVisual->Visible = m_fVisible > 0.5f ;
   if (m_pfrmVisual->Visible)
      m_pfrmVisual->BringToFront();

}
//--------------------------------------------------------------------------

//--------------------------------------------------------------------------
/// returns a parameter value
//--------------------------------------------------------------------------
#pragma argsused
float CHtVSTVisualize::getParameter (long index)
{
   return m_fVisible;
}
//--------------------------------------------------------------------------


//--------------------------------------------------------------------------
/// returns a parameter name
//--------------------------------------------------------------------------
#pragma argsused
void CHtVSTVisualize::getParameterName (long index, char *label)
{
   strcpy (label, "visible"); 
}
//--------------------------------------------------------------------------

//--------------------------------------------------------------------------
/// returns a parameter display value
//--------------------------------------------------------------------------
#pragma argsused
void CHtVSTVisualize::getParameterDisplay (long index, char *text)
{
   if (m_fVisible > 0.5f)
      strcpy (text, "true");
   else
      strcpy (text, "false");
}
//--------------------------------------------------------------------------

//--------------------------------------------------------------------------
/// returns a parameter  label (e.g. unit)
//--------------------------------------------------------------------------
#pragma argsused
void CHtVSTVisualize::getParameterLabel(long index, char *label)
{
   strcpy (label, " ");
}
//--------------------------------------------------------------------------

//--------------------------------------------------------------------------
/// returns effect name
//--------------------------------------------------------------------------
bool CHtVSTVisualize::getEffectName (char* name)
{
   strcpy (name, "Visualization");
   return true;
}
//--------------------------------------------------------------------------

//--------------------------------------------------------------------------
/// returns effect product string
//--------------------------------------------------------------------------
bool CHtVSTVisualize::getProductString (char* text)
{
   strcpy (text, "HtVSTVisualize");
   return true;
}
//--------------------------------------------------------------------------

//--------------------------------------------------------------------------
/// returns effect vendor string
//--------------------------------------------------------------------------
bool CHtVSTVisualize::getVendorString (char* text)
{
   strcpy (text, "HörTech");
   return true;
}
//--------------------------------------------------------------------------

//--------------------------------------------------------------------------
/// inits visualization buffer
//--------------------------------------------------------------------------
long CHtVSTVisualize::startProcess ()
{
   if (!m_pfrmVisual->BufferConnector->IsInit())
      {
      m_pfrmVisual->BufferConnector->SampleRate = sampleRate;
      m_pfrmVisual->BufferConnector->Init();
      }
   return 0;
}
//--------------------------------------------------------------------------

//--------------------------------------------------------------------------
/// exits visualization buffer
//--------------------------------------------------------------------------
long CHtVSTVisualize::stopProcess ()
{
   m_pfrmVisual->BufferConnector->Exit();
   return 0;
}
//--------------------------------------------------------------------------

//--------------------------------------------------------------------------
///
//--------------------------------------------------------------------------
void CHtVSTVisualize::resume ()
{
   startProcess();
}
//--------------------------------------------------------------------------

//--------------------------------------------------------------------------
///
//--------------------------------------------------------------------------
void CHtVSTVisualize::suspend ()
{
   stopProcess();
}
//--------------------------------------------------------------------------

//--------------------------------------------------------------------------
/// Processing routine called by process and processReplacing
//--------------------------------------------------------------------------
#pragma argsused
void CHtVSTVisualize::DoProcess (float **inputs, float **outputs, long sampleFrames, bool bReplace)
{
   if (!outputs[0] || !inputs[0])
      return;
   m_pfrmVisual->BufferConnector->BufferLoad(inputs[0], sampleFrames);
   MoveMemory(outputs[0], inputs[0], sampleFrames*sizeof(float));
}
//--------------------------------------------------------------------------

//--------------------------------------------------------------------------
/// accumulating processing routine
//--------------------------------------------------------------------------
void CHtVSTVisualize::process (float **inputs, float **outputs, long sampleFrames)
{
   DoProcess(inputs, outputs, sampleFrames, false);
}
//--------------------------------------------------------------------------

//--------------------------------------------------------------------------
/// replacing processing routine
//--------------------------------------------------------------------------
void CHtVSTVisualize::processReplacing (float **inputs, float **outputs, long sampleFrames)
{
   DoProcess(inputs, outputs, sampleFrames, true);
}
//--------------------------------------------------------------------------




