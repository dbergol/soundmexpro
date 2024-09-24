//------------------------------------------------------------------------------
/// \file AHtVSTConv.cpp
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
#include "AHtVSTConv.h"

#include <stdio.h>
#include <windows.h>
#include <math.h>


//--------------------------------------------------------------------------
/// dB to linear factor conversion
//--------------------------------------------------------------------------
static float dBToFactor(const float f4dB)
{
   return (float)pow(10,(double)f4dB/20);
}
//--------------------------------------------------------------------------

//--------------------------------------------------------------------------
/// constructor: calls base class and initializes members
//--------------------------------------------------------------------------
CHtVSTConvolver::CHtVSTConvolver (audioMasterCallback audioMaster)
   :  AudioEffectX (audioMaster, 1, 2), // programs, parameters
      m_bIsValid(false),
      m_hConvolver(NULL),
      m_fGain(1.0f),
      m_fEnabled(1.0f),
      m_bSkip(false),
      m_hLibrary(NULL),
      m_lpfnConvInit(NULL),
      m_lpfnConvExit(NULL),
      m_lpfnConvProcess(NULL)
{
   InitializeCriticalSection(&m_csDataSection);
   setNumInputs (MAX_CHANNELS);
   setNumOutputs (MAX_CHANNELS);
   setUniqueID (CCONST('H','t','C','v'));
   canProcessReplacing ();    // supports both accumulating and replacing output
   try
      {
      char c[2*MAX_PATH];
      ZeroMemory(c, 2*MAX_PATH);
      GetModuleFileName(hInstance, c, 2*MAX_PATH);
      // generate the name of the number-crunching-DLL
      AnsiString asLib = ChangeFileExt(c, "lib") + ".dll";
      m_hLibrary = LoadLibrary(asLib.c_str());
      if (!m_hLibrary)
         throw Exception("cannot load library '" + asLib + "'");
      m_lpfnConvInit = (LPFNCONVINIT)GetProcAddress(m_hLibrary, "ConvInit");
      if (!m_lpfnConvInit)
         m_lpfnConvInit = (LPFNCONVINIT)GetProcAddress(m_hLibrary, "_ConvInit");
      if (!m_lpfnConvInit)
         throw Exception("cannot load 'ConvInit' function from library '" + asLib + "'");


      m_lpfnConvExit = (LPFNCONVEXIT)GetProcAddress(m_hLibrary, "ConvExit");
      if (!m_lpfnConvExit)
         m_lpfnConvExit = (LPFNCONVEXIT)GetProcAddress(m_hLibrary, "_ConvExit");
      if (!m_lpfnConvExit)
         throw Exception("cannot load 'ConvExit' function from library '" + asLib + "'");

      m_lpfnConvProcess = (LPFNCONVPROCESS)GetProcAddress(m_hLibrary, "ConvProcess");
      if (!m_lpfnConvProcess)
         m_lpfnConvProcess = (LPFNCONVPROCESS)GetProcAddress(m_hLibrary, "_ConvProcess");
      if (!m_lpfnConvProcess)
         throw Exception("cannot load 'ConvProcess' function from library '" + asLib + "'");

      // set 'valid flag' (used in _main)
      m_bIsValid = true;
      }
   catch (Exception &e)
      {
      if (!!m_hLibrary)
         FreeLibrary(m_hLibrary);
      m_hLibrary = NULL;
      MessageBoxW(0, e.Message.c_str(), L"Error", 0);
      }
}
//--------------------------------------------------------------------------

//--------------------------------------------------------------------------
/// destructor
//--------------------------------------------------------------------------
CHtVSTConvolver::~CHtVSTConvolver ()
{
   ExitConv();
   try
      {
      if (m_hLibrary)
         FreeLibrary(m_hLibrary);
      }
   catch (...)
      {
      }
   m_hLibrary = NULL;
   DeleteCriticalSection(&m_csDataSection);

}
//--------------------------------------------------------------------------

//--------------------------------------------------------------------------
/// (Re-)initializes convolution DLL
//--------------------------------------------------------------------------
void CHtVSTConvolver::InitConv(AnsiString str)
{
   if (!m_hLibrary)
      throw Exception("Fatal error: lib not loaded");
   m_bSkip = true;
   EnterCriticalSection(&m_csDataSection);
   try
      {
      ExitConv();
      CWaveFileReader wfr;
      try
         {
         wfr.Load(str.c_str());
         }
      catch (LPCTSTR lpcsz)
         {
         MessageBox(0, lpcsz, "Error", 0);
         return;
         }

      #pragma clang diagnostic push
      #pragma clang diagnostic ignored "-Wcast-qual"
      m_hConvolver = m_lpfnConvInit((unsigned int)blockSize, MAX_CHANNELS, wfr.m_nSize, (const float**)wfr.m_lpData);
      #pragma clang diagnostic pop      
      m_strImpulseResponse = str;
      m_bSkip = false;
      }
   __finally
      {
      LeaveCriticalSection(&m_csDataSection);
      }
}
//--------------------------------------------------------------------------

//--------------------------------------------------------------------------
/// exits convolution DLL
//--------------------------------------------------------------------------
void CHtVSTConvolver::ExitConv()
{
   if (!m_hLibrary)
      return;
   try
      {
      EnterCriticalSection(&m_csDataSection);
      try
         {
         if (m_hConvolver)
            m_lpfnConvExit(m_hConvolver);
         m_hConvolver = NULL;
         }
      __finally
         {
         LeaveCriticalSection(&m_csDataSection);
         }
      }
   catch (...)
      {
      }
}
//--------------------------------------------------------------------------

//--------------------------------------------------------------------------
/// sets block size
//--------------------------------------------------------------------------
void CHtVSTConvolver::setBlockSize (VstInt32 nBlockSize)
{
   if (blockSize != nBlockSize || !m_hConvolver)
      {
      EnterCriticalSection(&m_csDataSection);
      try
         {
         bool bWasInit = !m_strImpulseResponse.IsEmpty();
         ExitConv();

         blockSize = nBlockSize;

         if (bWasInit)
            InitConv(m_strImpulseResponse);
         }
      __finally
         {
         LeaveCriticalSection(&m_csDataSection);
         }
      }
}
//--------------------------------------------------------------------------

//--------------------------------------------------------------------------
/// sets program name
//--------------------------------------------------------------------------
void CHtVSTConvolver::setProgramName (char *name)
{
   AnsiString str = name;
   if (!FileExists(str))
      return;
   InitConv(str);
}
//--------------------------------------------------------------------------

//--------------------------------------------------------------------------
/// returns actual program name
//--------------------------------------------------------------------------
void CHtVSTConvolver::getProgramName (char *name)
{
   if (m_strImpulseResponse.IsEmpty())
      strcpy (name, "default");
   else
      strcpy (name, m_strImpulseResponse.c_str());
}
//--------------------------------------------------------------------------

//--------------------------------------------------------------------------
/// sets a parameter value
//--------------------------------------------------------------------------
void CHtVSTConvolver::setParameter (VstInt32 index, float value)
{
   if (index == 0)
      m_fEnabled = value;
   else
      m_fGain = value;
}
//--------------------------------------------------------------------------

//--------------------------------------------------------------------------
/// returns a parameter value
//--------------------------------------------------------------------------
float CHtVSTConvolver::getParameter (VstInt32 index)
{
   return (index == 0) ? m_fEnabled : m_fGain;
}
//--------------------------------------------------------------------------

//--------------------------------------------------------------------------
/// returns a parameter name
//--------------------------------------------------------------------------
void CHtVSTConvolver::getParameterName (VstInt32 index, char *label)
{
   if (index == 0)
      strcpy(label, "enabled");
   else
      strcpy(label, "gain");
}
//--------------------------------------------------------------------------

//--------------------------------------------------------------------------
/// returns a parameter display value
//--------------------------------------------------------------------------
void CHtVSTConvolver::getParameterDisplay (VstInt32 index, char *text)
{
   if (index == 0)
      sprintf(text, m_fEnabled > 0.5f ? "true" : "false");
   else
      sprintf(text, "%.2f", 50.0 * (double)m_fGain - 50.0);
}
//--------------------------------------------------------------------------

//--------------------------------------------------------------------------
/// returns a parameter  label (e.g. unit)
//--------------------------------------------------------------------------
#pragma argsused
void CHtVSTConvolver::getParameterLabel(VstInt32 index, char *label)
{
   if (index == 0)
      strcpy (label, " ");
   else
      strcpy (label, "dB");
}
//--------------------------------------------------------------------------

//--------------------------------------------------------------------------
/// returns effect name
//--------------------------------------------------------------------------
bool CHtVSTConvolver::getEffectName (char* name)
{
   strcpy (name, "HtVSTConv");
   return true;
}
//--------------------------------------------------------------------------

//--------------------------------------------------------------------------
/// returns effect product string
//--------------------------------------------------------------------------
bool CHtVSTConvolver::getProductString (char* text)
{
   strcpy (text, "HtVSTConv");
   return true;
}
//--------------------------------------------------------------------------

//--------------------------------------------------------------------------
/// returns effect vendor string
//--------------------------------------------------------------------------
bool CHtVSTConvolver::getVendorString (char* text)
{
   strcpy (text, "Uni Ol");
   return true;
}
//--------------------------------------------------------------------------



//--------------------------------------------------------------------------
/// Processing routine called by process and processReplacing
//--------------------------------------------------------------------------
#pragma argsused
void CHtVSTConvolver::DoProcess (float **inputs, float **outputs, VstInt32 sampleFrames, bool bReplace)
{
   if (m_bSkip || !m_hConvolver || m_fEnabled <= 0.5f)
      {
      MoveMemory(outputs[0], inputs[0], (unsigned int)sampleFrames*sizeof(float));
      MoveMemory(outputs[1], inputs[1], (unsigned int)sampleFrames*sizeof(float));
      return;
      }

   EnterCriticalSection(&m_csDataSection);
   try
      {

      m_lpfnConvProcess(m_hConvolver, (unsigned int)blockSize, MAX_CHANNELS, MAX_CHANNELS, inputs, outputs);


      if (m_fGain != 1.0f)
         {
         float fGain = dBToFactor(50.0f * m_fGain - 50.0f);
         float *lpf;
         int nSamples;
         for (int nIndex = 0; nIndex < MAX_CHANNELS; nIndex++)
            {
            if (!outputs[nIndex])
               continue;
            nSamples = sampleFrames;
            lpf = &outputs[nIndex][0];
            while(nSamples--)
               {
               *lpf++ *= fGain;
               }
            }
         }
      }
   __finally
      {
      LeaveCriticalSection(&m_csDataSection);
      }
}
//--------------------------------------------------------------------------

//--------------------------------------------------------------------------
/// accumulating processing routine
//--------------------------------------------------------------------------
void CHtVSTConvolver::process (float **inputs, float **outputs, VstInt32 sampleFrames)
{
   DoProcess(inputs, outputs, sampleFrames, false);
}
//--------------------------------------------------------------------------

//--------------------------------------------------------------------------
/// replacing processing routine
//--------------------------------------------------------------------------
void CHtVSTConvolver::processReplacing (float **inputs, float **outputs, VstInt32 sampleFrames)
{
   DoProcess(inputs, outputs, sampleFrames, true);
}
//--------------------------------------------------------------------------





