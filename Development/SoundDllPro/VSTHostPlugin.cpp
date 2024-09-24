//------------------------------------------------------------------------------
/// \file VSTHostPlugin.cpp
/// \author Berg
/// \brief Implementation of classes TVSTPluginEditor and TVSTHostPlugin.
/// Encapsulates one VST-plugin and a parameter editor for a VST-plugin
///
/// Project SoundMexPro
/// Module  SoundDllPro.dll
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

#pragma warn -pch

#include <windowsx.h>
#include <math.h>
#include <stdio.h>
#include <string>

#pragma hdrstop
#include "VSTHostPlugin.h"
#include "VSTHost.h"
#include "SoundDllPro_Tools.h"
//---------------------------------------------------------------------------
#pragma package(smart_init)
#pragma link "VSTParameterFrame"
#pragma resource "*.dfm"


//------------------------------------------------------------------------------
/// TVSTThread. thread class for a plugin
//------------------------------------------------------------------------------

TVSTThread::TVSTThread(TVSTHostPlugin* pVSTPlugin, TThreadPriority tp)
 :  TThread(false), m_pVSTPlugin(pVSTPlugin)
{
   if (!pVSTPlugin)
      throw Exception("invalid VST instance passed to VSTThread");
   Priority = tp;
}
//------------------------------------------------------------------------------

//------------------------------------------------------------------------------
/// threads execute routine. Waits for stop or process signal and calls DoProcess
/// After processing is complete the 'done' signal is set
//------------------------------------------------------------------------------
void __fastcall TVSTThread::Execute()
{
   while (1)
      {
      if (Terminated)
         break;
      DWORD nWaitResult = WaitForMultipleObjects(2, &m_pVSTPlugin->m_hProcEvents[0], false, 1000);
      switch (nWaitResult)
         {
         // first is 'proc'
         case  (WAIT_OBJECT_0):
            ResetEvent(m_pVSTPlugin->m_hProcEvents[PLUG_EVENT_DONE]);
            m_pVSTPlugin->DoProcess();
            SetEvent(m_pVSTPlugin->m_hProcEvents[PLUG_EVENT_DONE]);
            break;
         // second is 'stop'
         case (WAIT_OBJECT_0 + 1):
            break;
         }
      }
}
//------------------------------------------------------------------------------

//------------------------------------------------------------------------------
/// TVSTHostPlugin. Encapsulates handling of one VST-plugin.
//------------------------------------------------------------------------------

//------------------------------------------------------------------------------
/// constructor. Loads plugin by name (DLL) and retrieves it's properties
//------------------------------------------------------------------------------
TVSTHostPlugin::TVSTHostPlugin(  AnsiString                 strLibName,
                                 audioMasterCallback        HostCallback,
                                 float                      fSampleRate,
                                 int                        nBlockSize,
                                 const std::vector<TVSTNode>&  vMappingIn,
                                 const std::vector<int>&       viMappingOut,
                                 bool                       bUseThread,
                                 TThreadPriority            tpThreadPriority)
   :
      m_strLibName(strLibName),
      m_pEffect(NULL),
      m_pProcThread(NULL),
      m_lpcszUserConfig(NULL),
      m_bBypass(false),
      m_kPlugCategory(kPlugCategUnknown),
      m_pslPrograms(NULL),
      m_pslParams(NULL),
      m_hLib(NULL),
      m_pfrmEditor(NULL),
      m_pfrmProperties(NULL),
      m_nBlockSize(nBlockSize)

{
   try
      {
      m_bDebugOutputOnce = true;

      for (int i = 0; i < PLUG_EVENT_LAST; i++)
         m_hProcEvents[i] = 0;

      m_hLib = LoadLibrary(strLibName.c_str());
      if (!m_hLib)
         throw Exception("Failed to load VST Plugin library '" + strLibName + "'!");

      PluginEntryProc mainEntry = (PluginEntryProc)GetProcAddress (m_hLib, "VSTPluginMain");
      if (!mainEntry)
         mainEntry = (PluginEntryProc)GetProcAddress (m_hLib, "main");
      if (!mainEntry)
         throw Exception("VST Plugin main entry not found!");

      m_pEffect = mainEntry(HostCallback);
      if (!m_pEffect)
         throw Exception("Failed to create effect instance!");

      if (m_pEffect->magic != kEffectMagic)
         throw Exception("Library is not a valid VST plugin!");

      if (  m_pEffect->dispatcher(m_pEffect,effGetVstVersion,0,0,NULL,0.0f) >= 2
         && !!(m_pEffect->flags & effFlagsIsSynth)
         )
         throw Exception("VST synthesizer plugins not supported");

      // 23-03.2009: moved up and changed order: set samplerate and blocksize
      // before opening it!!
      m_pEffect->dispatcher(m_pEffect, effSetSampleRate, 0, 0, 0, fSampleRate);
      m_pEffect->dispatcher(m_pEffect, effSetBlockSize, 0, nBlockSize, 0, 0);
      m_pEffect->dispatcher(m_pEffect, effOpen, 0, 0, 0, 0);

      if ((int)vMappingIn.size() > m_pEffect->numInputs)
         throw Exception(  IntToStr((int)vMappingIn.size())
                        +  " input channels requested from VST plugin that has only "
                        +  IntToStr((int)m_pEffect->numInputs)
                        +  " inputs");

      if ((int)viMappingOut.size() > m_pEffect->numOutputs)
         throw Exception(  IntToStr((int)viMappingOut.size())
                        +  " output channels requested from VST plugin that has only "
                        +  IntToStr((int)m_pEffect->numOutputs)
                        +  " outputs");
      

      // set vector/valarray sizes
      m_vvafIn.resize((unsigned int)m_pEffect->numInputs);
      m_vapfIn.resize((unsigned int)m_pEffect->numInputs);
      m_vvafOut.resize((unsigned int)m_pEffect->numOutputs);
      m_vapfOut.resize((unsigned int)m_pEffect->numOutputs);

      // check mapping vectors
      unsigned int i;
      for (i = 0; i < viMappingOut.size(); i++)
         {
         if (viMappingOut[i] < 0)
            throw Exception("no negative output mapping channels allowed");
         }

      // copy I/O-mapping vectors
      m_vMappingIn.resize(vMappingIn.size());
      m_vMappingIn    = vMappingIn;
      m_viMappingOut.resize(viMappingOut.size());
      m_viMappingOut   = viMappingOut;

      // create float buffers
      SetBufferSize((unsigned int)m_nBlockSize);

      m_pslPrograms                 = new TStringList();
      m_pslPrograms->CaseSensitive  = false;
      m_pslPrograms->Delimiter      = ',';

      m_pslParams                   = new TStringList();
      m_pslParams->CaseSensitive    = false;
      m_pslParams->Delimiter        = ',';


      GetProperties();

      if (bUseThread)
         {
         m_hProcEvents[PLUG_EVENT_PROC]    = CreateEvent(NULL, FALSE, FALSE, NULL);
         m_hProcEvents[PLUG_EVENT_STOP]    = CreateEvent(NULL, FALSE, FALSE, NULL);
         // NOTE: done event must _not_ be autoreset (see ProcDone(), which polls that flag)
         m_hProcEvents[PLUG_EVENT_DONE]    = CreateEvent(NULL, TRUE, FALSE, NULL);
         m_pProcThread = new TVSTThread(this, tpThreadPriority);
         }

      }
   catch (Exception &e)
      {
      Cleanup();
      throw Exception("error loading plugin ' " + strLibName + "': " + e.Message);
      }
   catch (...)
      {
      Cleanup();
      throw Exception("unknown error loading plugin ' " + strLibName + "'");
      }
}
//------------------------------------------------------------------------------

//------------------------------------------------------------------------------
/// destructor. does cleanup
//------------------------------------------------------------------------------
TVSTHostPlugin::~TVSTHostPlugin()
{
   Cleanup();
}
//------------------------------------------------------------------------------


//------------------------------------------------------------------------------
/// calls vendor specific VST function passing a pointer to a string as first
/// long parameter. Return value is interpreted as pointer to string as well
//------------------------------------------------------------------------------
bool TVSTHostPlugin::SetUserConfig(AnsiString& as)
{
   const int nBufferSize = 20*USHRT_MAX;
   int nReturn = 0;
   if (m_pEffect)
      {
      if (!m_lpcszUserConfig)
         m_lpcszUserConfig = new char[nBufferSize];
      ZeroMemory(m_lpcszUserConfig, nBufferSize);
      // pass pointer to zero terminated string as input data and pointer to character array and length for return
      nReturn = (int)m_pEffect->dispatcher(m_pEffect, effVendorSpecific, HT_VST_VENDOR_USERCONFIG, (VstIntPtr)as.c_str(), m_lpcszUserConfig, nBufferSize-1);
      as = m_lpcszUserConfig;
      }
   return nReturn != 0;
}
//------------------------------------------------------------------------------

//------------------------------------------------------------------------------
/// deletes member classes, buffers. Deactivates plugin and unloads library
//------------------------------------------------------------------------------
void TVSTHostPlugin::Cleanup()
{
   TRYDELETENULL(m_pfrmEditor);
   TRYDELETENULL(m_pfrmProperties);
   TRYDELETENULL(m_pslPrograms);
   TRYDELETENULL(m_pslParams);
   try
      {
      if (m_hLib)
         {
         try
            {
            if (m_pEffect)
               {
               Stop();
               m_pEffect->dispatcher(m_pEffect, effClose, 0, 0, 0, 0);
               }
            }
         catch (...)
            {
            }
         FreeLibrary(m_hLib);
         }

      if (m_pProcThread)
         {
         m_pProcThread->Terminate();
         SetEvent(m_hProcEvents[PLUG_EVENT_STOP]);
         m_pProcThread->WaitFor();
         TRYDELETENULL(m_pProcThread);
         }

      for (int i = 0; i < PLUG_EVENT_LAST; i++)
         {
         if (m_hProcEvents[i] != NULL)
            {
            CloseHandle(m_hProcEvents[i]);
            m_hProcEvents[i] = NULL;
            }
         }
      }
   catch (...)
      {
      }
   m_hLib = NULL;
   m_pEffect = NULL;

   if (m_lpcszUserConfig)
      {
      try
         {
         delete [] m_lpcszUserConfig;
         }
      catch (...)
         {
         }
      }
}
//------------------------------------------------------------------------------

//------------------------------------------------------------------------------
/// returns number of inputs
//------------------------------------------------------------------------------
int TVSTHostPlugin::GetNumInputs()
{
   return m_pEffect->numInputs;
}
//------------------------------------------------------------------------------

//------------------------------------------------------------------------------
/// returns number of outputs
//------------------------------------------------------------------------------
int TVSTHostPlugin::GetNumOutputs()
{
   return m_pEffect->numOutputs;
}
//------------------------------------------------------------------------------

//------------------------------------------------------------------------------
/// returns number of inputs and outputs for a DLL
//------------------------------------------------------------------------------
void TVSTHostPlugin::GetNumChannels(AnsiString strLibName, int &nNumIn, int &nNumOut)
{
   HINSTANCE hLib = NULL;
   nNumIn = 0;
   nNumOut = 0;
   try
      {
      hLib = LoadLibrary(strLibName.c_str());
      if (!hLib)
         throw Exception("Failed to load VST Plugin library '" + strLibName + "'!");
      PluginEntryProc mainEntry = (PluginEntryProc)GetProcAddress (hLib, "TVSTHostPluginMain");
      if (!mainEntry)
         mainEntry = (PluginEntryProc)GetProcAddress (hLib, "main");
      if (!mainEntry)
         throw Exception("VST Plugin main entry not found!");

      AEffect* pEffect = mainEntry(TVSTHost::VSTHostCallback);
      if (!pEffect)
         throw Exception("Failed to create effect instance!");
      nNumIn   = pEffect->numInputs;
      nNumOut  = pEffect->numOutputs;
      // NOTE: effClose MUST be called to shut down plugin properly!!
      pEffect->dispatcher(pEffect, effClose, 0, 0, 0, 0);
      }
   __finally
      {
      try
         {
         if (hLib)
            {
            FreeLibrary(hLib);
            }
         }
      catch (...)
         {
         }
      }
}
//------------------------------------------------------------------------------


//------------------------------------------------------------------------------
/// Creates local buffers.
//------------------------------------------------------------------------
void TVSTHostPlugin::SetBufferSize(unsigned int nBufferSize)
{
   // adjust float buffer sizes and re-set pointers to internal buffers
   unsigned int i;
   for (i = 0; i < m_vvafIn.size(); i++)
      {
      m_vvafIn[i].resize(nBufferSize);
      m_vapfIn[i] = &m_vvafIn[i][0];
      }
   for (i = 0; i < m_vvafOut.size(); i++)
      {
      m_vvafOut[i].resize(nBufferSize);
      m_vapfOut[i] = &m_vvafOut[i][0];
      }
}
//------------------------------------------------------------------------------

//------------------------------------------------------------------------------
/// Queries plugin for parameters, programs and some more properties.
//------------------------------------------------------------------------------
void TVSTHostPlugin::GetProperties()
{
   m_pslPrograms->Clear();
   m_pslParams->Clear();

   // retrieve effect name, vendor and product
   char sz[256] = {0};
   m_pEffect->dispatcher(m_pEffect, effGetEffectName, 0, 0, sz, 0);
   m_strEffectName = sz;
   ZeroMemory(sz, 256);
   m_pEffect->dispatcher(m_pEffect, effGetVendorString, 0, 0, sz, 0);
   m_strVendorString = sz;
   ZeroMemory(sz, 256);
   m_pEffect->dispatcher(m_pEffect, effGetProductString, 0, 0, sz, 0);
   m_strProductString = sz;

   // NOTE: internally program names and property names are stored in comma-separated lists.
   // This would lead to problems, if these names contain commas. This never happened up to now
   // but to be sure we refuse to use a plugin in that case

   // Iterate programs and store names in string list
   AnsiString as;
   for (VstInt32 nProgIndex = 0; nProgIndex < m_pEffect->numPrograms; nProgIndex++)
      {
      ZeroMemory(sz, 256);
      if (!m_pEffect->dispatcher(m_pEffect, effGetProgramNameIndexed, nProgIndex, 0, sz, 0))
         {
         m_pEffect->dispatcher(m_pEffect, effSetProgram, 0, nProgIndex, 0, 0); // Note: old program not restored here!
         m_pEffect->dispatcher(m_pEffect, effGetProgramName, 0, 0, sz, 0);
         }
      as = Trim(AnsiString(sz));
      if (as.Pos(",") > 0)
         throw Exception("VST Plugin '" + m_strLibName + "' cannot be used (program names contain commas)");

      m_pslPrograms->Add(as);
      }
   // restore program 0
   m_pEffect->dispatcher(m_pEffect, effSetProgram, 0, 0, 0, 0);


   // Iterate parameters and store names in string list
   for (VstInt32 nParamIndex = 0; nParamIndex < m_pEffect->numParams; nParamIndex++)
      {
      ZeroMemory(sz, 256);
      m_pEffect->dispatcher(m_pEffect, effGetParamName, nParamIndex, 0, sz, 0);
      as = Trim(AnsiString(sz));
      if (as.Pos(",") > 0)
         throw Exception("VST Plugin '" + m_strLibName + "' cannot be used (parameter names contain commas)");
      m_pslParams->Add(Trim(AnsiString(sz)));
      }

   m_bCanReplacing = (m_pEffect->flags & effFlagsCanReplacing) != 0;


   m_kPlugCategory = (VstPlugCategory)m_pEffect->dispatcher(m_pEffect, effGetPlugCategory, 0, 0, 0, 0);
}
//------------------------------------------------------------------------------

//------------------------------------------------------------------------------
/// Re-allocates local buffers if necessary, sends current buffersize and samplerate
/// to plugin and activates it
//------------------------------------------------------------------------------
void TVSTHostPlugin::Start(float fSampleRate, int nBlockSize)
{
   m_bDebugOutputOnce = true;
   // if buffer size changed, adjust internal buffer size!
   if (nBlockSize != m_nBlockSize)
      {
      SetBufferSize((unsigned int)nBlockSize);
      m_nBlockSize = nBlockSize;
      }

   // send sample rate and block size again to plugin
   m_pEffect->dispatcher(m_pEffect, effSetSampleRate, 0, 0, 0, fSampleRate);
   m_pEffect->dispatcher(m_pEffect, effSetBlockSize, 0, nBlockSize, 0, 0);

   // call 'resume' with dispatcher
   m_pEffect->dispatcher(m_pEffect, effMainsChanged, 0, 1, 0, 0);

   if (!!m_pEffect->dispatcher(m_pEffect, effStartProcess, 0, 0, 0, 0))
      throw Exception("error starting processing in plugin");
}
//------------------------------------------------------------------------------

//------------------------------------------------------------------------------
/// Deactivates plugin
//------------------------------------------------------------------------------
void TVSTHostPlugin::Stop()
{
   if (!!m_hProcEvents[PLUG_EVENT_STOP])
      SetEvent(m_hProcEvents[PLUG_EVENT_STOP]);
   // call 'suspend' with dispatcher
   m_pEffect->dispatcher(m_pEffect, effStopProcess, 0, 0, 0, 0);
   m_pEffect->dispatcher(m_pEffect, effMainsChanged, 0, 0, 0, 0);
}
//------------------------------------------------------------------------------

//------------------------------------------------------------------------------
/// Creates editor (if necessary) and shows it
//------------------------------------------------------------------------------
void TVSTHostPlugin::ShowEditor()
{
   if (!m_pfrmEditor)
      m_pfrmEditor = new TVSTPluginEditor(this);
   m_pfrmEditor->Show();
}
//------------------------------------------------------------------------------

//------------------------------------------------------------------------------
/// Creates property dialog (if necessary) and shows it
//------------------------------------------------------------------------------
void TVSTHostPlugin::ShowProperties()
{
   if (!m_pfrmProperties)
      m_pfrmProperties = new TVSTPluginProperties(m_pEffect);
   m_pfrmProperties->Show();
}
//------------------------------------------------------------------------------

//------------------------------------------------------------------------------
/// returns capability of bypass
//------------------------------------------------------------------------------
bool TVSTHostPlugin::CanBypass()
{
   return (m_pEffect->numInputs == m_pEffect->numOutputs);
}
//------------------------------------------------------------------------------

//------------------------------------------------------------------------------
/// returns bypass
//------------------------------------------------------------------------------
bool TVSTHostPlugin::GetBypass()
{
   return m_bBypass;
}
//------------------------------------------------------------------------------

//------------------------------------------------------------------------------
/// Sets bypass
//------------------------------------------------------------------------------
void TVSTHostPlugin::SetBypass(bool bBypass)
{
   if (bBypass && !CanBypass())
      throw Exception("plugin does not support bypass");
   m_bBypass = bBypass;
}
//------------------------------------------------------------------------------


//------------------------------------------------------------------------------
/// returns processing error
//------------------------------------------------------------------------------
AnsiString TVSTHostPlugin::GetProcError()
{
   AnsiString asReturn;
   // pass pointer to zero terminated string as input data and pointer to an
   // AnsiString for receiving return data
   if (m_pEffect->dispatcher(m_pEffect, effVendorSpecific, HT_VST_VENDOR_PROCERROR, 0, &asReturn, 0))
      m_asProcError = asReturn;
   return m_asProcError;
}
//------------------------------------------------------------------------------

//------------------------------------------------------------------------------
/// returns program name
//------------------------------------------------------------------------------
AnsiString TVSTHostPlugin::GetProgramName()
{
   AnsiString str;
   // retrieve effect name, vendor and product
   char sz[256] = {0};
   ZeroMemory(sz, 256);
   m_pEffect->dispatcher(m_pEffect, effGetProgramName, 0, 0, sz, 0);
   str = sz;
   return str;
}
//------------------------------------------------------------------------------

//------------------------------------------------------------------------------
/// returns program index
//------------------------------------------------------------------------------
int TVSTHostPlugin::GetProgramIndex()
{
   return (int)m_pEffect->dispatcher(m_pEffect, effGetProgram, 0, 0, 0, 0);
}
//------------------------------------------------------------------------------

//------------------------------------------------------------------------------
/// returns program names as comma delimited string
//------------------------------------------------------------------------------
AnsiString TVSTHostPlugin::GetPrograms()
{
   return m_pslPrograms->DelimitedText;
}
//------------------------------------------------------------------------------

//------------------------------------------------------------------------------
/// Sets program by index
//------------------------------------------------------------------------------
void TVSTHostPlugin::SetProgram(int nIndex)
{
   if (nIndex < 0 || nIndex >= m_pEffect->numPrograms)
      throw Exception("program index out of range");
   m_pEffect->dispatcher(m_pEffect, effSetProgram, 0, nIndex, 0, 0);
   if (m_pfrmEditor)
      {
      m_pfrmEditor->UpdatePrograms();
      m_pfrmEditor->UpdateParameters();
      }
}
//------------------------------------------------------------------------------

//------------------------------------------------------------------------------
/// Sets program by name
//------------------------------------------------------------------------------
void TVSTHostPlugin::SetProgram(AnsiString strName)
{
   int nIndex = m_pslPrograms->IndexOf(strName);
   if (nIndex == -1)
      throw Exception("program '" + strName + "' not found");
   m_pEffect->dispatcher(m_pEffect, effSetProgram, 0, nIndex, 0, 0);
   if (m_pfrmEditor)
      {
      m_pfrmEditor->UpdatePrograms();
      m_pfrmEditor->UpdateParameters();
      }
}
//------------------------------------------------------------------------------

//------------------------------------------------------------------------------
/// Sets current program's name
//------------------------------------------------------------------------------
void TVSTHostPlugin::SetProgramName(AnsiString strName)
{
   m_pEffect->dispatcher(m_pEffect, effSetProgramName, 0, 0, strName.c_str(), 0);
   // check, if name was changed at all (i.e. if plugin itself supports renaming!)
   if (GetProgramName() != strName)
      throw Exception("setting program name failed!");
   m_pslPrograms->Strings[GetProgramIndex()] = strName;
   if (m_pfrmEditor)
      {
      m_pfrmEditor->UpdatePrograms();
      m_pfrmEditor->UpdateParameters();
      }
}
//------------------------------------------------------------------------------


//------------------------------------------------------------------------------
/// returns parameters and values as comma delimited string
//------------------------------------------------------------------------------
AnsiString TVSTHostPlugin::GetParameters(AnsiString strParams)
{
   // retrieve all values.
   AnsiString str;
   TStringList *pslParams = new TStringList();
   TStringList *pslValues = new TStringList();
   pslParams->Delimiter = ',';
   pslValues->Delimiter = ',';
   try
      {
      if (strParams.IsEmpty())
         strParams = m_pslParams->DelimitedText;
      pslParams->DelimitedText = strParams;
      int i;
      for (i = 0; i < pslParams->Count; i++)
         pslValues->Add(pslParams->Strings[i] + "=" + DoubleToStr((double)GetParameter(pslParams->Strings[i])));
      str = pslValues->DelimitedText;
      }
   __finally
      {
      TRYDELETENULL(pslParams);
      TRYDELETENULL(pslValues);
      }
   return str;
}
//------------------------------------------------------------------------------

//------------------------------------------------------------------------------
/// returns parameter names as comma delimited string
//------------------------------------------------------------------------------
AnsiString TVSTHostPlugin::GetParameterNames()
{
   return m_pslParams->DelimitedText;
}
//------------------------------------------------------------------------------

//------------------------------------------------------------------------------
/// returns parameters values as comma delimited string
//------------------------------------------------------------------------------
AnsiString TVSTHostPlugin::GetParameterValues(AnsiString strParams)
{
   // retrieve all values.
   AnsiString str;
   TStringList *pslParams = new TStringList();
   TStringList *pslValues = new TStringList();
   pslParams->Delimiter = ',';
   pslValues->Delimiter = ',';
   try
      {
      // if no param name passed, query all
      if (strParams.IsEmpty())
         strParams = m_pslParams->DelimitedText;
      pslParams->DelimitedText = strParams;

      int i, nIndex;
      AnsiString strParam;
      for (i = 0; i < pslParams->Count; i++)
         {
         strParam = pslParams->Strings[i];

         if (TryStrToInt(strParam, nIndex))
            pslValues->Add(DoubleToStr((double)GetParameter(nIndex)));
         else
            pslValues->Add(DoubleToStr((double)GetParameter(strParam)));
         }
      str = pslValues->DelimitedText;
      }
   __finally
      {
      TRYDELETENULL(pslParams);
      TRYDELETENULL(pslValues);
      }
   return str;
}
//------------------------------------------------------------------------------

//------------------------------------------------------------------------------
/// returns parameter value by name
//------------------------------------------------------------------------------
float TVSTHostPlugin::GetParameter(AnsiString strName)
{
   int nIndex = m_pslParams->IndexOf(strName);
   if (nIndex == -1)
      throw Exception("parameter '" + strName + "' not found");

   return m_pEffect->getParameter(m_pEffect, nIndex);
}
//------------------------------------------------------------------------------

//------------------------------------------------------------------------------
/// returns parameter value by index
//------------------------------------------------------------------------------
float TVSTHostPlugin::GetParameter(int nIndex)
{
   if (nIndex < 0 || nIndex >= m_pEffect->numParams)
      throw Exception("parameter index out of range");

   return m_pEffect->getParameter(m_pEffect, nIndex);
}
//------------------------------------------------------------------------------


//------------------------------------------------------------------------------
/// sets multiple parameters by name or by index passed as comma separated list:
///      par1=val1,par2=val2....
/// Break on first error!
/// NOTE: assignments with empty values are ignored!
//------------------------------------------------------------------------------
void TVSTHostPlugin::SetParameters(AnsiString strParamsAndValues)
{
   TStringList *psl = new TStringList();
   psl->Delimiter = ',';
   try
      {
      psl->DelimitedText = strParamsAndValues;
      int i, nIndex;
      AnsiString strValue;

      for(i = 0; i < psl->Count; i++)
         {
         strValue = psl->Values[psl->Names[i]];
         if (Trim(strValue).IsEmpty())
            continue;

         if (!IsDouble(strValue))
            throw Exception("invalid parameter value (0 <= value <= 1)");

         if (TryStrToInt(psl->Names[i], nIndex))
            SetParameter(nIndex, (float)StrToDouble(strValue), false);
         else
            SetParameter(psl->Names[i], (float)StrToDouble(strValue), false, false);
         }
      if (m_pfrmEditor)
         m_pfrmEditor->UpdateParameters();
      }
   __finally
      {
      TRYDELETENULL(psl);
      }
}
//------------------------------------------------------------------------------

//------------------------------------------------------------------------------
/// sets multiple parameters by name or by index passed as two comma separated
/// lists. Break on first error!
//------------------------------------------------------------------------------
void TVSTHostPlugin::SetParameters(AnsiString strParams, AnsiString strValues)
{
   TStringList *pslParams = new TStringList();
   TStringList *pslValues = new TStringList();
   pslParams->Delimiter = ',';
   pslValues->Delimiter = ',';
   try
      {
      if (strParams.IsEmpty())
         strParams = m_pslParams->DelimitedText;
      ParseValues(pslParams, strParams.c_str(), ',', true);
      ParseValues(pslValues, strValues.c_str(), ',', true);
      if (pslParams->Count != pslValues->Count)
         throw Exception("number of parameter names and values must be identical");


      int i, nIndex;
      for(i = 0; i < pslParams->Count; i++)
         {
         if (!IsDouble(pslValues->Strings[i]))
            throw Exception("invalid parameter value (0 <= value <= 1)");
         if (TryStrToInt(pslParams->Strings[i], nIndex))
            SetParameter(nIndex, (float)StrToDouble(pslValues->Strings[i]), false);
         else
            SetParameter(pslParams->Strings[i], (float)StrToDouble(pslValues->Strings[i]), false, false);
         }
      if (m_pfrmEditor)
         m_pfrmEditor->UpdateParameters();
      }
   __finally
      {
      TRYDELETENULL(pslParams);
      TRYDELETENULL(pslValues);
      }
}
//------------------------------------------------------------------------------

//------------------------------------------------------------------------------
/// sets parameter value by index
//------------------------------------------------------------------------------
void TVSTHostPlugin::SetParameter(int nIndex, float fValue, bool bUpdateGUI)
{
   if (nIndex < 0 || nIndex >= m_pEffect->numParams)
      throw Exception("parameter index out of range");
   if (fValue < 0.0f || fValue > 1.0f)
      throw Exception("parameter value out of range (0 <= value <= 1)");

   m_pEffect->setParameter(m_pEffect, nIndex, fValue);
 /* TODO : check success ??? */

   if (bUpdateGUI && m_pfrmEditor)
      m_pfrmEditor->UpdateParameters();

}
//------------------------------------------------------------------------------

//------------------------------------------------------------------------------
/// sets parameter value by name
//------------------------------------------------------------------------------
void TVSTHostPlugin::SetParameter(  AnsiString strName,
                                    float fValue,
                                    bool bIgnoreUnknownParameter,
                                    bool bUpdateGUI)
{
   int nIndex = m_pslParams->IndexOf(strName);
   if (nIndex == -1)
      {
      if (bIgnoreUnknownParameter)
         return;
      throw Exception("parameter '" + strName + "' not found");
      }
   if (fValue < 0.0f || fValue > 1.0f)
      throw Exception("parameter value out of range (0 <= value <= 1)");

   m_pEffect->setParameter(m_pEffect, nIndex, fValue);
 /* TODO : check success ??? */

   if (bUpdateGUI && m_pfrmEditor)
      m_pfrmEditor->UpdateParameters();
}
//------------------------------------------------------------------------------

//------------------------------------------------------------------------------
/// sets parameter value by name
//------------------------------------------------------------------------------
VstPlugCategory TVSTHostPlugin::GetCategory()
{
   return m_kPlugCategory;
}
//------------------------------------------------------------------------------


//------------------------------------------------------------------------------
/// Main processing routine. Calls process or processReplacing respectively.
/// Buffers are processed in the following way:
//------------------------------------------------------------------------------
void TVSTHostPlugin::Process(const vvfVST& vvfBuffer, const std::vector<vvfVST>& vvvfRecursion)
{
   unsigned int nChannels           = (unsigned int)vvfBuffer.size();
   unsigned int nMappedChannels     = (unsigned int)m_vMappingIn.size();
   unsigned int nInternalChannels   = (unsigned int)m_vvafIn.size();
   unsigned int nLayers             = (unsigned int)vvvfRecursion.size();
   unsigned int i;

   // NOTE: in TVSTNodes of m_vMappingIn negative values of channel and layer mean:
   // - negative m_nLayer: use real input channel (no recursion)
   // - negative m_nChannel: don't use this channel at all (zeroes on input)

   if (m_bBypass)
      {
       if (!CanBypass())
         throw Exception("bypass requested where impossible");

      // copy external input data with respect to channel mapping
      // directly to output
      for (i = 0; i < nInternalChannels; i++)
         {
         // channel mapped at all ?
         if (i < nMappedChannels)
            {
            TVSTNode& rNode = m_vMappingIn[i];
            // channel negative or out of range: fill with zeroes
            if (rNode.m_nChannel < 0 || rNode.m_nChannel >= (int)nChannels)
               m_vvafOut[i] = 0.0f;
            // layer neagtive or out of range: use regular input
            else if (rNode.m_nLayer < 0 || rNode.m_nLayer >= (int)nLayers)
               m_vvafOut[i] = vvfBuffer[(unsigned int)rNode.m_nChannel];
            // copy recursion buffer
            else
               m_vvafOut[i] = vvvfRecursion[(unsigned int)rNode.m_nLayer][(unsigned int)rNode.m_nChannel];
            }
         else
            m_vvafOut[i] = 0.0f;
         }
      }
   // real processing (no bypass)
   else
      {

      // copy external input data with respect to channel mapping
      for (i = 0; i < nInternalChannels; i++)
         {
         // channel mapped at all ?
         if (i < nMappedChannels)
            {
            TVSTNode& rNode = m_vMappingIn[i];
            // channel negative or out of range: fill with zeroes
            if (rNode.m_nChannel < 0 || rNode.m_nChannel >= (int)nChannels)
               m_vvafIn[i] = 0.0f;
            // layer neagtive or out of range: use regular input
            else if (rNode.m_nLayer < 0 || rNode.m_nLayer >= (int)nLayers)
               m_vvafIn[i] = vvfBuffer[(unsigned int)rNode.m_nChannel];
            // copy recursion buffer
            else
               m_vvafIn[i] = vvvfRecursion[(unsigned int)rNode.m_nLayer][(unsigned int)rNode.m_nChannel];


            #ifdef DEBUG_RECURSE
            if (m_bDebugOutputOnce)

               {
               AnsiString as;
               // channel negative or out of range: fill with zeroes
               if (rNode.m_nChannel < 0 || rNode.m_nChannel >= (int)nChannels)
                  as.printf("not used: %d",i);
               // layer neagtive or out of range: use regular input
               else if (rNode.m_nLayer < 0 || rNode.m_nLayer >= (int)nLayers)
                  as.printf("regular use: %d:%d",i,rNode.m_nChannel);
               // copy recursion buffer
               else
                  as.printf("recursion use: %d:%d:%d",i,rNode.m_nChannel,rNode.m_nLayer);
               OutputDebugString(as.c_str());
               }
            #endif
            }
         else
            m_vvafIn[i] = 0.0f;
         }

      nInternalChannels   = (unsigned int)m_vvafOut.size();
      // clear internal output buffers
      for (i = 0; i < nInternalChannels; i++)
         m_vvafOut[i] = 0.0f;


      if (!m_pProcThread)
         DoProcess();
      else
         {
         ResetEvent(m_hProcEvents[PLUG_EVENT_DONE]);
         SetEvent(m_hProcEvents[PLUG_EVENT_PROC]);
         }

      } // real processing (no bypass)
   m_bDebugOutputOnce = false;
}
//------------------------------------------------------------------------------

//------------------------------------------------------------------------------
/// internal processing routine to be called after data are in place: calls
/// VST-plugins processing routine
//------------------------------------------------------------------------------
void TVSTHostPlugin::DoProcess()
{
   /*
   DWORD dw = 10000;
   while (dw--) ;
   return;
   */
   #ifndef VST_2_4_EXTENSIONS
   if (!m_bCanReplacing)
      {
      m_pEffect->process(m_pEffect, &m_vapfIn[0], &m_vapfOut[0], m_nBlockSize);
      }
   else
   #endif
      {
      m_pEffect->processReplacing(m_pEffect, &m_vapfIn[0], &m_vapfOut[0], m_nBlockSize);
      }
}

//------------------------------------------------------------------------------
/// returns reference to internal 'done' signal handle
//------------------------------------------------------------------------------
HANDLE&    TVSTHostPlugin::GetProcHandle()
{
   return m_hProcEvents[PLUG_EVENT_DONE];
}
//------------------------------------------------------------------------------

//------------------------------------------------------------------------------
/// returns internal output data
//------------------------------------------------------------------------------
const std::vector<std::valarray<float> >& TVSTHostPlugin::GetOutData()
{
   // return complete (!) internal buffer. Host has to pick correct channels according
   // to output channel mapping
   return m_vvafOut;
}
//------------------------------------------------------------------------------

//------------------------------------------------------------------------------
/// returns internal output mapping
//------------------------------------------------------------------------------
const std::vector< int>& TVSTHostPlugin::GetOutputMapping()
{
   return m_viMappingOut;
}
//------------------------------------------------------------------------------

//------------------------------------------------------------------------------
/// returns internal input mapping
//------------------------------------------------------------------------------
const std::vector<TVSTNode >& TVSTHostPlugin::GetInputMapping()
{
   return m_vMappingIn;
}
//------------------------------------------------------------------------------

//------------------------------------------------------------------------------
/// Calls effEditIdle
//------------------------------------------------------------------------------
void TVSTHostPlugin::CallEditIdle()
{
   m_pEffect->dispatcher(m_pEffect, effEditIdle, 0, 0, 0, 0);
}
//------------------------------------------------------------------------------

//******************************************************************************
/// \class TVSTPluginEditor. Simple VST-Parameter-Editor class
//******************************************************************************

//------------------------------------------------------------------------------
/// constructor. Creates parameter frames and menu items for program changes
//------------------------------------------------------------------------------
__fastcall TVSTPluginEditor::TVSTPluginEditor(TVSTHostPlugin* pParentPlugin)
   : TForm((TComponent*)NULL), m_pParentPlugin(pParentPlugin)
{
   if (!pParentPlugin)
      throw Exception("invalid VST instance passed");

   AnsiString str = m_pParentPlugin->m_strProductString + " " + m_pParentPlugin->m_strEffectName;
   if (!Trim(str).IsEmpty() && !m_pParentPlugin->m_strVendorString.IsEmpty())
      str += " (" + m_pParentPlugin->m_strVendorString + ")";
   if (Trim(str).IsEmpty())
      str = ExtractFileName(ChangeFileExt(m_pParentPlugin->m_strLibName, ""));
   Caption = "VSTPluginEditor - " + str;

   for (int i = 0; i < pcPages->PageCount; i++)
      pcPages->Pages[i]->TabVisible = false;
   pcPages->MultiLine  = false;


   TframeVSTParam *pframe = NULL;
   // Iterate parameters...
   int nNumParams = pParentPlugin->m_pEffect->numParams;
   if (!nNumParams)
      return;
   for (VstInt32 nParamIndex = 0; nParamIndex < nNumParams; nParamIndex++)
      pframe = new TframeVSTParam(scbParameter, pParentPlugin->m_pEffect, nParamIndex);

   // adjust height of dialog: maximum of 10 parameters 
   m_nParameterDlgWidth    = pframe->Width + (ClientWidth - scbParameter->Width);
   if (nNumParams > 10)
      nNumParams = 10;
   m_nParameterDlgHeight   = (nNumParams * pframe->Height) + scbParameter->Top + (ClientHeight - tsParameter->Height);

   UpdatePrograms();

   miPluginDlg->Enabled = (pParentPlugin->m_pEffect->flags & effFlagsHasEditor);
   miDlgClick(miPluginDlg->Enabled ? miPluginDlg : miParDlg);
}
//------------------------------------------------------------------------------

//------------------------------------------------------------------------------
/// Updates plugin parameters of all instances of TframeVSTParam
//------------------------------------------------------------------------------
void TVSTPluginEditor::UpdateParameters()
{
   // update parameters
   TframeVSTParam *pframe;
   for (int nParamIndex = 0; nParamIndex < scbParameter->ComponentCount; nParamIndex++)
      {
      pframe = dynamic_cast<TframeVSTParam*>(scbParameter->Components[nParamIndex]);
      if (pframe)
         pframe->UpdateValue(true);
      }
}
//------------------------------------------------------------------------------

//------------------------------------------------------------------------------
/// Updates plugin program names
//------------------------------------------------------------------------------
void TVSTPluginEditor::UpdatePrograms()
{
   TMenuItem *pmi;
   for (int nProgramIndex = 0; nProgramIndex < m_pParentPlugin->m_pslPrograms->Count; nProgramIndex++)
      {
      if (nProgramIndex >= miPrograms->Count)
         {
         pmi = new TMenuItem(miPrograms);
         pmi->OnClick   = miProgramClick;
         pmi->Tag       = nProgramIndex;
         pmi->RadioItem = true;
         miPrograms->Add(pmi);
         }
      else
         {
         pmi = miPrograms->Items[nProgramIndex];
         }

      pmi->Caption   = m_pParentPlugin->m_pslPrograms->Strings[nProgramIndex];

      if (m_pParentPlugin->GetProgramName() == pmi->Caption)
         pmi->Checked = true;
      }
}
//------------------------------------------------------------------------------

//------------------------------------------------------------------------------
/// Sets editor (internal or external.
//------------------------------------------------------------------------------
void TVSTPluginEditor::SetEditor()
{
   if (!m_pParentPlugin)
      return;

   if (miParDlg->Checked)
      {
      ClientWidth          = m_nParameterDlgWidth;
      ClientHeight         = m_nParameterDlgHeight;
      if (scbParameter->VertScrollBar->IsScrollBarVisible())
         ClientWidth += GetSystemMetrics(SM_CYHSCROLL);
      UpdateParameters();
      pcPages->ActivePage  = tsParameter;
      }
   else
      {
      pcPages->ActivePage = tsPlugin;
      m_pParentPlugin->m_pEffect->dispatcher(m_pParentPlugin->m_pEffect, effEditOpen, 0, 0, tsPlugin->Handle, 0);
      ERect* eRect = 0;
      m_pParentPlugin->m_pEffect->dispatcher(m_pParentPlugin->m_pEffect, effEditGetRect, 0, 0, &eRect, 0);
      if (eRect)
         {
         ClientWidth = eRect->right - eRect->left + (ClientWidth - tsPlugin->Width);
         ClientHeight = eRect->bottom - eRect->top + (ClientHeight - tsPlugin->Height);

         if (Width < 100)
            Width = 100;
         if (Height < 100)
            Height = 100;
         }
      }
}
//------------------------------------------------------------------------------

//------------------------------------------------------------------------------
/// Calls FormDestroy
//------------------------------------------------------------------------------
#pragma argsused
void __fastcall TVSTPluginEditor::FormClose(TObject *Sender, TCloseAction &Action)
{
   FormDestroy(Sender);
}
//------------------------------------------------------------------------------

//------------------------------------------------------------------------------
/// calls VST effEditClose
//------------------------------------------------------------------------------
#pragma argsused
void __fastcall TVSTPluginEditor::FormDestroy(TObject *Sender)
{
   if (m_bHasEditor && m_pParentPlugin->m_pEffect)
      m_pParentPlugin->m_pEffect->dispatcher(m_pParentPlugin->m_pEffect, effEditClose, 0, 0, 0, 0);
}
//------------------------------------------------------------------------------

//------------------------------------------------------------------------------
/// Sets editor to internal or external repectively
//------------------------------------------------------------------------------
void __fastcall TVSTPluginEditor::miDlgClick(TObject *Sender)
{
   if (Sender == miParDlg || !miPluginDlg->Enabled)
      miParDlg->Checked = true;
   else if (Sender == miPluginDlg)
      miPluginDlg->Checked = true;
   else
      return;

   SetEditor();
}
//------------------------------------------------------------------------------

//------------------------------------------------------------------------------
/// Selects a program and call UpdateParameters
//------------------------------------------------------------------------------
void __fastcall TVSTPluginEditor::miProgramClick(TObject *Sender)
{
   TMenuItem *pmi = dynamic_cast<TMenuItem*>(Sender);
   if (pmi)
      {
      m_pParentPlugin->m_pEffect->dispatcher(m_pParentPlugin->m_pEffect, effSetProgram, 0, (int)pmi->Tag, 0, 0);
      pmi->Checked = true;
      UpdateParameters();
      }
}
//------------------------------------------------------------------------------

//------------------------------------------------------------------------------
/// Calls UpdateParameters
//------------------------------------------------------------------------------
#pragma argsused
void __fastcall TVSTPluginEditor::miUpdateParsClick(TObject *Sender)
{
   UpdateParameters();
}
//------------------------------------------------------------------------------

//------------------------------------------------------------------------------
/// Calls UpdateParameters
//------------------------------------------------------------------------------
#pragma argsused
void __fastcall TVSTPluginEditor::FormShow(TObject *Sender)
{
   UpdateParameters();
}
//---------------------------------------------------------------------------


