//------------------------------------------------------------------------------
/// \file VSTHostPlugin.cpp
/// \author Berg
/// \brief Implementation of classes TVSTThread and TVSTHostPlugin.
/// Encapsulates one VST-plugin
///
/// Project SoundMexPro
/// Module  SoundDllPro.dll
///
///
/// ****************************************************************************
/// Copyright 2026 Daniel Berg, Oldenburg, Germany
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
#include "VST2PluginImpl.h"
#include "VST3PluginImpl.h"
#include "SoundDllPro_Tools.h"
//---------------------------------------------------------------------------
#pragma package(smart_init)

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
/// constructor. Detects the plugin format (VST2 DLL vs. VST3 module) and
/// creates the matching format-specific implementation, then retrieves
/// it's properties
//------------------------------------------------------------------------------
TVSTHostPlugin::TVSTHostPlugin(  AnsiString                 strLibName,
                                 float                      fSampleRate,
                                 int                        nBlockSize,
                                 const std::vector<TVSTNode>&  vMappingIn,
                                 const std::vector<int>&       viMappingOut,
                                 bool                       bUseThread,
                                 TThreadPriority            tpThreadPriority)
   :
      m_strLibName(strLibName),
      m_pImpl(NULL),
      m_pProcThread(NULL),
      m_bBypass(false),
      m_kPlugCategory(vpcUnknown),
      m_pslPrograms(NULL),
      m_pslParams(NULL),
      m_pfrmEditor(NULL),
      m_pfrmProperties(NULL),
      m_nBlockSize(nBlockSize)

{
   try
      {
      m_bDebugOutputOnce = true;

      for (int i = 0; i < PLUG_EVENT_LAST; i++)
         m_hProcEvents[i] = 0;

      // create the format-specific implementation: VST3 modules export
      // 'GetPluginFactory' - everything else is assumed to be a VST2 DLL.
      // Neither implementation needs an externally supplied host callback
      // (CVST2PluginImpl uses its own VSTHostCallback() internally).
      if (CVST3PluginImpl::IsVST3Module(strLibName))
         m_pImpl = new CVST3PluginImpl(strLibName, fSampleRate, nBlockSize);
      else
         m_pImpl = new CVST2PluginImpl(strLibName, fSampleRate, nBlockSize);

      if ((int)vMappingIn.size() > m_pImpl->GetNumInputs())
         throw Exception(  IntToStr((int)vMappingIn.size())
                        +  " input channels requested from VST plugin that has only "
                        +  IntToStr(m_pImpl->GetNumInputs())
                        +  " inputs");

      if ((int)viMappingOut.size() > m_pImpl->GetNumOutputs())
         throw Exception(  IntToStr((int)viMappingOut.size())
                        +  " output channels requested from VST plugin that has only "
                        +  IntToStr(m_pImpl->GetNumOutputs())
                        +  " outputs");


      // set vector/valarray sizes
      m_vvafIn.resize((unsigned int)m_pImpl->GetNumInputs());
      m_vapfIn.resize((unsigned int)m_pImpl->GetNumInputs());
      m_vvafOut.resize((unsigned int)m_pImpl->GetNumOutputs());
      m_vapfOut.resize((unsigned int)m_pImpl->GetNumOutputs());

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
   return m_pImpl->SetUserConfig(as);
}
//------------------------------------------------------------------------------

//------------------------------------------------------------------------------
/// deletes member classes, buffers. Deactivates plugin and unloads implementation
//------------------------------------------------------------------------------
void TVSTHostPlugin::Cleanup()
{
   TRYDELETENULL(m_pfrmEditor);
   TRYDELETENULL(m_pfrmProperties);
   TRYDELETENULL(m_pslPrograms);
   TRYDELETENULL(m_pslParams);
   try
      {
      // deleting the implementation stops processing, closes and unloads
      // the plugin (see CVST2PluginImpl::Cleanup())
      TRYDELETENULL(m_pImpl);

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
}
//------------------------------------------------------------------------------

//------------------------------------------------------------------------------
/// returns number of inputs
//------------------------------------------------------------------------------
int TVSTHostPlugin::GetNumInputs()
{
   return m_pImpl->GetNumInputs();
}
//------------------------------------------------------------------------------

//------------------------------------------------------------------------------
/// returns number of outputs
//------------------------------------------------------------------------------
int TVSTHostPlugin::GetNumOutputs()
{
   return m_pImpl->GetNumOutputs();
}
//------------------------------------------------------------------------------

//------------------------------------------------------------------------------
/// returns number of inputs and outputs for a plugin module/DLL, without
/// instantiating it permanently
//------------------------------------------------------------------------------
void TVSTHostPlugin::GetNumChannels(AnsiString strLibName, int &nNumIn, int &nNumOut)
{
   if (CVST3PluginImpl::IsVST3Module(strLibName))
      CVST3PluginImpl::GetNumChannels(strLibName, nNumIn, nNumOut);
   else
      CVST2PluginImpl::GetNumChannels(strLibName, nNumIn, nNumOut);
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
   m_strEffectName      = m_pImpl->GetEffectName();
   m_strVendorString    = m_pImpl->GetVendorString();
   m_strProductString   = m_pImpl->GetProductString();

   // NOTE: internally program names and property names are stored in comma-separated lists.
   // This would lead to problems, if these names contain commas. This never happened up to now
   // but to be sure we refuse to use a plugin in that case

   // Iterate programs and store names in string list
   AnsiString as;
   int nNumPrograms = m_pImpl->GetNumPrograms();
   for (int nProgIndex = 0; nProgIndex < nNumPrograms; nProgIndex++)
      {
      as = Trim(m_pImpl->GetProgramNameAt(nProgIndex));
      if (as.Pos(",") > 0)
         throw Exception("VST Plugin '" + m_strLibName + "' cannot be used (program names contain commas)");

      m_pslPrograms->Add(as);
      }
   // restore program 0 (if any - avoids an unnecessary/invalid SetProgram
   // call for plugins without programs)
   if (nNumPrograms > 0)
      m_pImpl->SetProgram(0);


   // Iterate parameters and store names in string list
   int nNumParams = m_pImpl->GetNumParams();
   for (int nParamIndex = 0; nParamIndex < nNumParams; nParamIndex++)
      {
      as = Trim(m_pImpl->GetParamNameAt(nParamIndex));
      if (as.Pos(",") > 0)
         throw Exception("VST Plugin '" + m_strLibName + "' cannot be used (parameter names contain commas)");
      m_pslParams->Add(as);
      }

   m_kPlugCategory = m_pImpl->GetCategory();
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

   m_pImpl->Start(fSampleRate, nBlockSize);
}
//------------------------------------------------------------------------------

//------------------------------------------------------------------------------
/// Deactivates plugin
//------------------------------------------------------------------------------
void TVSTHostPlugin::Stop()
{
   if (!!m_hProcEvents[PLUG_EVENT_STOP])
      SetEvent(m_hProcEvents[PLUG_EVENT_STOP]);
   m_pImpl->Stop();
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
      m_pfrmProperties = new TVSTPluginProperties(m_pImpl);
   m_pfrmProperties->Show();
}
//------------------------------------------------------------------------------

//------------------------------------------------------------------------------
/// returns capability of bypass
//------------------------------------------------------------------------------
bool TVSTHostPlugin::CanBypass()
{
   return (m_pImpl->GetNumInputs() == m_pImpl->GetNumOutputs());
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
   return m_pImpl->GetProcError();
}
//------------------------------------------------------------------------------

//------------------------------------------------------------------------------
/// returns program name
//------------------------------------------------------------------------------
AnsiString TVSTHostPlugin::GetProgramName()
{
   return m_pImpl->GetProgramName();
}
//------------------------------------------------------------------------------

//------------------------------------------------------------------------------
/// returns program index
//------------------------------------------------------------------------------
int TVSTHostPlugin::GetProgramIndex()
{
   return m_pImpl->GetProgramIndex();
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
   if (nIndex < 0 || nIndex >= m_pImpl->GetNumPrograms())
      throw Exception("program index out of range");
   m_pImpl->SetProgram(nIndex);
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
   m_pImpl->SetProgram(nIndex);
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
   m_pImpl->SetProgramName(strName);
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

   return m_pImpl->GetParameter(nIndex);
}
//------------------------------------------------------------------------------

//------------------------------------------------------------------------------
/// returns parameter value by index
//------------------------------------------------------------------------------
float TVSTHostPlugin::GetParameter(int nIndex)
{
   if (nIndex < 0 || nIndex >= m_pImpl->GetNumParams())
      throw Exception("parameter index out of range");

   return m_pImpl->GetParameter(nIndex);
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
   if (nIndex < 0 || nIndex >= m_pImpl->GetNumParams())
      throw Exception("parameter index out of range");
   if (fValue < 0.0f || fValue > 1.0f)
      throw Exception("parameter value out of range (0 <= value <= 1)");

   m_pImpl->SetParameter(nIndex, fValue);
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

   m_pImpl->SetParameter(nIndex, fValue);
 /* TODO : check success ??? */

   if (bUpdateGUI && m_pfrmEditor)
      m_pfrmEditor->UpdateParameters();
}
//------------------------------------------------------------------------------

//------------------------------------------------------------------------------
/// returns plugin category
//------------------------------------------------------------------------------
EVSTPluginCategory TVSTHostPlugin::GetCategory()
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
/// the implementation's processing routine
//------------------------------------------------------------------------------
void TVSTHostPlugin::DoProcess()
{
   m_pImpl->Process(&m_vapfIn[0], &m_vapfOut[0], m_nBlockSize);
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
/// Calls effEditIdle equivalent
//------------------------------------------------------------------------------
void TVSTHostPlugin::CallEditIdle()
{
   m_pImpl->EditIdle();
}
//------------------------------------------------------------------------------


