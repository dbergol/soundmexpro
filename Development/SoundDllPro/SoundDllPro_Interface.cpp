//------------------------------------------------------------------------------
/// \file SoundDllPro_Interface.cpp
/// \author Berg
/// Main interface file for SoundDllPro implementing exported function SoundDllProCommand
/// and all functions called by SoundDllProCommand. It includes sounddllpro_cmd.h that
/// defines the command buffer containing all commands, help, syntax... 
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
#pragma hdrstop
#include <vcl.h>
#include <string>
#include <inifiles.hpp>
#include <windows.h>
#include <stdio.h>
#include <math.h>
#include <limits.h>
#include <float.h>
#include <objbase.h>
#include "SoundDllPro_Interface.h"
#include "SoundDllPro_Main.h"
#include "SoundDllPro_WaveReader_libsndfile.h"
#include "MPlugin.h"
#include "formTracks.h"
#include "formMixer.h"
#pragma hdrstop
#pragma warn -use
#pragma warn -aus
//------------------------------------------------------------------------------
#pragma package(smart_init)
// patch command buffer into this file
#include "sounddllpro_cmd.h"

using namespace Asio;

//------------------------------------------------------------------------------
TSimpleMidi* g_pMidi = NULL;

/// initialize global variables
AnsiString  g_strLogFile = "";
AnsiString  g_strBinPath = "";

// global flag, if ramps used
bool        g_bUseRamps   = true;
//------------------------------------------------------------------------------
/// Standard DLLEntryPoint. Sets Borland-Fix for floating point exceptions
//------------------------------------------------------------------------------
#pragma argsused
int WINAPI DllEntryPoint(HINSTANCE hinst, unsigned long reason, void* lpReserved)
{
   switch (reason)
      {
      case DLL_PROCESS_ATTACH: 
            #ifndef _WIN64
            _control87(PC_64 | MCW_EM, MCW_PC | MCW_EM);
            #endif
            break;
      }
   return 1;
}
//------------------------------------------------------------------------------

//------------------------------------------------------------------------------
/// Exported main function (string command interface)
//------------------------------------------------------------------------------
int cdecl SoundDllProCommand(const char* lpcszCommand, char* lpszReturnValue, int nLength)
{
   AnsiString sCommand, sError;
   int iReturn = SOUNDDLL_RETURN_ERROR;
   TStringList *psl = new TStringList();
   try // __except
      {
      try // catch
         {
         if (g_strBinPath == "")
            {
            char c[2*MAX_PATH];
            ZeroMemory(c, 2*MAX_PATH);
            if (GetModuleFileName(GetModuleHandle(SOUNDDLLPRO_DLL), c, 2*MAX_PATH-1))
               g_strBinPath = IncludeTrailingBackslash(ExtractFilePath(c));
            }
         if (nLength < 50)
            throw Exception("buffersize for return value too small!");
         if (!lpcszCommand || !lpszReturnValue)
            throw Exception("null pointer passed to SoundDllProCommand");
         // clear passed return buffer
         ZeroMemory((void*)lpszReturnValue, nLength);
         // convert string to TStringList (is ';' delimited!)
         psl->Delimiter = ';';
         ParseValues(psl, lpcszCommand);
         WriteToLogFile(lpcszCommand);

         // check, if command is set at all!
         sCommand = psl->Values[SOUNDDLLPRO_STR_COMMAND];
         if (sCommand.IsEmpty())
            throw Exception("command name missing");
         if (SoundClass())
            SoundClass()->AddDebugString(sCommand);
         //------------------------------------------------------------------------
         // Starting work_buffer
         //------------------------------------------------------------------------
          CMD_ARG     *lpArg = cmd_arg;
         //--------------HELP-----------------------------------------------------
         if (  !strcmpi(sCommand.c_str(), SOUNDDLLPRO_CMD_HELP)
            || !strcmpi(sCommand.c_str(), SOUNDDLLPRO_CMD_HELPA)
            )
            {
            bool bSorted = !strcmpi(sCommand.c_str(), SOUNDDLLPRO_CMD_HELPA);
            // check magic argument passed by SOUNDMEX to decide if
            // commands 'showerror' and 'getlasterror' to be listed
            bool bCallBySoundMex = (psl->Values[SOUNDDLLPRO_PAR_SOUNDMEXCALL] == "1");
            sCommand = psl->Values[SOUNDDLLPRO_CMD_HELP];
            AnsiString sReturn;
            // no further argument: print available commands
            if (sCommand.IsEmpty())
               {
               psl->Clear();
               TStringList *pslHelp = new TStringList();
               try
                  {
                  // find all functions
                  while ( NULL!=lpArg->lpszName )
                     {
                     // command to be listed?
                     if (  !strcmpi(lpArg->lpszName, SOUNDDLLPRO_CMD_BETATEST)
                        || (!bCallBySoundMex && !strcmpi(lpArg->lpszName, SOUNDDLLPRO_CMD_SHOWERROR))
                        || (!bCallBySoundMex && !strcmpi(lpArg->lpszName, SOUNDDLLPRO_CMD_GETLASTERROR))
                        )
                        ; // do nothing by purpose (only else used)!!
                     else
                        pslHelp->Add(AnsiString(lpArg->lpszName) +  ",");
                     lpArg++;
                     }
                  pslHelp->Sorted = bSorted;
                  pslHelp->Delimiter = '\n';
                  psl->Values[SOUNDDLLPRO_CMD_HELP] = pslHelp->DelimitedText;
                  }
               __finally
                  {
                  TRYDELETENULL(pslHelp);
                  }
               }
            else
               {
               psl->Clear();
               // find specific function
               while ( NULL!=lpArg->lpszName && !!strcmpi(lpArg->lpszName, sCommand.c_str()))
                  lpArg++;
               // check command/function
               if ( NULL==lpArg->lpszName) // end of arg list ?
                  throw Exception("no help for command found: command not available in sounddllpro");
               psl->Values[SOUNDDLLPRO_CMD_HELP] = lpArg->lpszHelp;
               }
            iReturn = SOUNDDLL_RETURN_OK;
            }
         //--------------HELP END-------------------------------------------------
         else
            {
            // find function:
            // - name must not be NULL (end of array reached)
            // - name must be the one of interest
            // - function pointer must not be NULL (for Dummy commands like help, helpa..)
            while (  NULL!=lpArg->lpszName && !!strcmpi(lpArg->lpszName, sCommand.c_str()) && NULL!=lpArg->lpfn )
               {
               lpArg++;
               }
            // check command namd and function pointer
            if ( NULL==lpArg->lpszName || NULL==lpArg->lpfn) // end of arg list ?
               throw Exception("Command not found in sounddllpro");
            AnsiString strArgs = lpArg->lpszArgs;
            // check, if all arguments are known
            if (strcmpi(lpArg->lpszName, SOUNDDLLPRO_CMD_BETATEST))
               {
               for (int i = 0; i < psl->Count; i++)
                  {
                  // argument 'command' (i.e. commandname) is always known
                  if (!strcmpi(AnsiString(psl->Names[i]).c_str(), SOUNDDLLPRO_STR_COMMAND))
                     continue;
                  if (strArgs.Pos(psl->Names[i] + ",") == 0)
                     throw Exception("unknown parameter '" + psl->Names[i] + "' passed");
                  }
               }
            // here we check for async errors:
            // command except 'exit' will return false here if such
            // an error occurred
            if (SoundClass())
               {
               if (SoundClass()->AsyncError(sError))
                  {
                  sError = "an asynchroneous error occurred prior to this command: " + sError;
                  // we throw an exception if command is _not_
                  // 'stop', 'exit' or 'reseterror'
                  // on commands we write the error and
                  // perform the command
                  if (  !!strcmpi(sCommand.c_str(), SOUNDDLLPRO_CMD_EXIT)
                     && !!strcmpi(sCommand.c_str(), SOUNDDLLPRO_CMD_STOP)
                     && !!strcmpi(sCommand.c_str(), SOUNDDLLPRO_CMD_RESETERRORA)
                     && !!strcmpi(sCommand.c_str(), SOUNDDLLPRO_CMD_ASYNCERROR)
                     )
                     throw Exception(sError);
                  // Otherwise sError is written to error return value below
                  }
               }
            // a few commands need special handling
            // - 'init' and 'exit' to be called directly, not via created/deleted class
            // - 'isinitialized' only checks class existance,
            // - 'version' needs no init before
            if (!lpArg->nMustInit)
               {
               lpArg->lpfn(psl);
               }
            else
               {
               if (!SoundClass())
                  throw Exception("SoundDllPro is not initialized");
               lpArg->lpfn(psl);
               }
            }
         iReturn = SOUNDDLL_RETURN_OK;
         }
      catch (Exception &e)
         {
         psl->Clear();
         psl->Values[SOUNDDLLPRO_CMD_ERROR] = e.Message;
         }
      catch (Asio::EAsioError &e)
         {
         psl->Clear();
         psl->Values[SOUNDDLLPRO_CMD_ERROR] = e.m_lpszMsg;
         }
      catch (...)
         {
         psl->Clear();
         psl->Values[SOUNDDLLPRO_CMD_ERROR] = "unknown C++ Exception in SoundDllPro";
         }
      }
   __except (true)
      {
      psl->Clear();
      psl->Values[SOUNDDLLPRO_CMD_ERROR] = "unknown C Exception in SoundDllPro";
      }
   if (!sError.IsEmpty())
      psl->Values[SOUNDDLLPRO_CMD_ERROR] = sError;
   if (psl->DelimitedText.Length() >= nLength)
      {
      psl->Clear();
      psl->Values[SOUNDDLLPRO_CMD_ERROR] = "not enough space for return values!";
      iReturn = SOUNDDLL_RETURN_ERROR;
      }
   // finally remove ',' from error string: must NOT occur!!
   psl->Values[SOUNDDLLPRO_CMD_ERROR] = StringReplace(psl->Values[SOUNDDLLPRO_CMD_ERROR], ",", " ", TReplaceFlags() << rfReplaceAll);;
   AnsiString strReturn;
   for (int i = 0; i < psl->Count;i++)
      strReturn += psl->Strings[i] +";";
   RemoveTrailingChar(strReturn, ';');
   snprintf(lpszReturnValue, (unsigned int)nLength, "%hs", strReturn.c_str());
   WriteToLogFile("% RETURN: " + strReturn);
   TRYDELETENULL(psl);
   if (SoundClass())
      SoundClass()->AddDebugString(sCommand + " done");
   return iReturn;
}
//------------------------------------------------------------------------------

//******************************************************************************
// ALL FUNCTIONS BELOW ARE OF TYPE LPFNSNDDLLFUNC AND ARE CALLED THROUGH COMMAND
// BUFFER BY SoundDllProCommand.
// THEY ALL READ THEIR VALUES FROM AND WRITE RETURN VALUES TO PASSED TStringList
// NOTE: if existance of SoundClass() is required, this is stored as flag in command
// buffer and already checked by command SoundDllProCommand
//******************************************************************************

//------------------------------------------------------------------------------
/// Sets driver model. Must be called BEFORE 'init'
//------------------------------------------------------------------------------
void SetDriverModel(TStringList *psl)
{
   AnsiString strType = psl->Values[SOUNDDLLPRO_PAR_VALUE];
   psl->Clear();
   if (strType.IsEmpty())
      strType = "asio";
   if (!strcmpi(strType.c_str(), SOUNDDLLPRO_PAR_ASIO))
      SoundDllProMain::SetDriverModel(DRV_TYPE_ASIO);
   else if (!strcmpi(strType.c_str(), SOUNDDLLPRO_PAR_WDM))
      SoundDllProMain::SetDriverModel(DRV_TYPE_WDM);
   else
      throw Exception("unknown value");
}
//------------------------------------------------------------------------------

//------------------------------------------------------------------------------
/// returns current driver model
//------------------------------------------------------------------------------
void GetDriverModel(TStringList *psl)
{
   psl->Clear();
   TSoundDriverModel sdm = SoundDllProMain::GetDriverModel();
   if (sdm == DRV_TYPE_ASIO)
      SetValue(psl, SOUNDDLLPRO_PAR_VALUE, SOUNDDLLPRO_PAR_ASIO);
   else if (sdm == DRV_TYPE_WDM)
      SetValue(psl, SOUNDDLLPRO_PAR_VALUE, SOUNDDLLPRO_PAR_WDM);
   else
      SetValue(psl, SOUNDDLLPRO_PAR_VALUE, "UNKNOWN");
}
//------------------------------------------------------------------------------

//------------------------------------------------------------------------------
/// Init function. Creates global static instance of SoundClass: SoundClass() and calls
/// it's init function
//------------------------------------------------------------------------------
void Init(TStringList *psl)
{
   #ifdef DEBUG_LOGFILE
   InitDebug();
   #endif
   if (psl->Values[SOUNDDLLPRO_PAR_FORCE] == "1")
      Exit(NULL);
   else if (SoundClass())
      throw Exception("SoundMexPro is already initialized");
   try
      {
      g_strLogFile   = psl->Values[SOUNDDLLPRO_PAR_LOGFILE];
      if (g_strLogFile.Length() > 0)
         DeleteFile(g_strLogFile);
      // here we have to log manually
      // avoid quotes
      psl->QuoteChar = ';';
      WriteToLogFile(psl->DelimitedText);
      #ifdef TEST_INIT_DEBUG
      DeleteFile(g_strBinPath + "init.log");
      WriteToLogFile("StartInit", g_strBinPath + "init.log");
      #endif
      new SoundDllProMain();
      if (!SoundClass())
         throw Exception("error creating SoundClass");
      SoundClass()->Initialize(psl);
      }
   catch (...)
      {
      if (SoundClass())
         delete SoundClass();
      throw;
      }
}
//------------------------------------------------------------------------------

//------------------------------------------------------------------------------
/// Shows AboutBox
//------------------------------------------------------------------------------
void About(TStringList *psl)
{
   psl->Clear();
   if (SoundClass())
      {
      int nRestart = SoundClass()->About();
      if (nRestart)
         {
         Exit(NULL);
         MessageBox(0, "Online license status has changed, module was exited automatically", "Hint", MB_ICONWARNING);
         }
      }
   else
      {      
      if (!IsAudioSpike())
         SetStyle();
      TAboutBox* pfrmAbout  = new TAboutBox(NULL);
      try
         {
         pfrmAbout->About();
         }
      __finally
         {
         TRYDELETENULL(pfrmAbout);
         }
      }
}
//------------------------------------------------------------------------------

//------------------------------------------------------------------------------
/// Returns initialisation status
//------------------------------------------------------------------------------
void Initialized(TStringList *psl)
{
   psl->Clear();
   psl->Values[SOUNDDLLPRO_CMD_INITIALIZED] = !!SoundClass() ? "1" : "0";
}
//------------------------------------------------------------------------------

//------------------------------------------------------------------------------
/// sets and returns current track mapping, i.e. mapping of virtual tracks to real
/// outout channels
//------------------------------------------------------------------------------
void TrackMap(TStringList *psl)
{
   std::vector<int> viTrack = SoundClass()->ConvertSoundChannelArgument(
                                    psl->Values[SOUNDDLLPRO_PAR_TRACK],
                                    CT_OUTPUT,
                                    CCA_ARGS_EMPTYONEMPTY | CCA_ARGS_DUPALLOWED);
   unsigned int nTrackIndex;
   unsigned int nChannelIndex;
   psl->Clear();
   // anything to change?
   if (viTrack.size())
      {
      if (viTrack.size() != SoundClass()->m_vTracks.size())
         throw Exception("number of values must be identical to track count (" + IntToStr((int)SoundClass()->m_vTracks.size()) + ")");
      EnterCriticalSection(&SoundClass()->m_csProcess);
      try
         {
         // establish new mapping
         for (nTrackIndex = 0; nTrackIndex < viTrack.size(); nTrackIndex++)
            {
            nChannelIndex = (unsigned int)viTrack[nTrackIndex];
            if (nChannelIndex >= SoundClass()->SoundActiveChannels(Asio::OUTPUT))
               throw Exception("invalid channel: out of range");
            SoundClass()->m_vTracks[nTrackIndex]->ChannelIndex(nChannelIndex);
            }
         }
      __finally
         {
         LeaveCriticalSection(&SoundClass()->m_csProcess);
         }
      SoundClass()->m_pfrmTracks->UpdateTrackInfos();
      SoundClass()->m_pfrmMixer->UpdateChannelData(CT_TRACK);
      }

   // write current mapping to return
   AnsiString str;
   for (nTrackIndex = 0; nTrackIndex < SoundClass()->m_vTracks.size(); nTrackIndex++)
      str += IntToStr((int)SoundClass()->m_vTracks[nTrackIndex]->ChannelIndex()) + ",";
   // remove last ','
   RemoveTrailingChar(str);
   SetValue(psl, SOUNDDLLPRO_PAR_TRACK, str);
//   psl->Values[SOUNDDLLPRO_PAR_TRACK] = str;
}
//------------------------------------------------------------------------------

//------------------------------------------------------------------------------
/// sets and returns track mode ('add' or 'multiply') of one or more tracks
//------------------------------------------------------------------------------
void TrackMode(TStringList *psl)
{
   std::vector<int> viTrack = SoundClass()->ConvertSoundChannelArgument(psl->Values[SOUNDDLLPRO_PAR_TRACK], CT_TRACK);
   unsigned int nTrackIndex;
   // check if any mode is to be set
   AnsiString str = psl->Values[SOUNDDLLPRO_PAR_MODE];
   psl->Clear();
   if (!str.IsEmpty())
      {
      TStringList *pslTmp = new TStringList();
      pslTmp->Delimiter = ',';

      try
         {
         pslTmp->DelimitedText = str;
         unsigned int nValues = (unsigned int)pslTmp->Count;
         // dim of new volume vector now must be 1 (all channels) or equal to dim of viChannel
         if (nValues != 1 && nValues != viTrack.size())
            throw Exception("mode value count matching error");
         // first check all modes: we do not want to change anything if any mode is invalid
         for (nTrackIndex = 0; nTrackIndex < (unsigned int)pslTmp->Count; nTrackIndex++)
            {
            if (  pslTmp->Strings[(int)nTrackIndex] != "0"
               && pslTmp->Strings[(int)nTrackIndex] != "1"
               )
               throw Exception("invalid mode: must be 0 or 1");
            }
         EnterCriticalSection(&SoundClass()->m_csProcess);
         try
            {
            for (nTrackIndex = 0; nTrackIndex < viTrack.size(); nTrackIndex++)
               {
               if (nValues == 1)
                  SoundClass()->m_vTracks[(unsigned int)viTrack[nTrackIndex]]->Multiply((pslTmp->Strings[0] == "1"));
               else
                  SoundClass()->m_vTracks[(unsigned int)viTrack[nTrackIndex]]->Multiply((pslTmp->Strings[(int)nTrackIndex] == "1"));
               }
            }
         __finally
            {
            LeaveCriticalSection(&SoundClass()->m_csProcess);
			}
		 }
	  __finally
		 {
		 TRYDELETENULL(pslTmp);
		 }
	  }
   // write modes to return
   str = "";
   for (nTrackIndex = 0; nTrackIndex < SoundClass()->m_vTracks.size(); nTrackIndex++)
	  str += IntToStr((int)SoundClass()->m_vTracks[nTrackIndex]->Multiply()) + ",";
   // remove last ','
   RemoveTrailingChar(str);
   SetValue(psl, SOUNDDLLPRO_PAR_MODE, str);
}
//------------------------------------------------------------------------------

//------------------------------------------------------------------------------
/// returns track length of all tracks
//------------------------------------------------------------------------------
#pragma argsused
void TrackLen(TStringList *psl)
{
   psl->Clear();
   AnsiString str;
   unsigned int nTrackIndex;
   for (nTrackIndex = 0; nTrackIndex < SoundClass()->m_vTracks.size(); nTrackIndex++)
      {
      if (SoundClass()->m_vTracks[nTrackIndex]->EndlessLoop())
         str += "-1,";
      else
         str += IntToStr((int)SoundClass()->m_vTracks[nTrackIndex]->NumTrackSamples()) + ",";
      }
   // remove last ','
   RemoveTrailingChar(str);
   SetValue(psl, SOUNDDLLPRO_PAR_VALUE, str);
}
//------------------------------------------------------------------------------

//------------------------------------------------------------------------------
/// stops audio output and delets global SoundClass()
//------------------------------------------------------------------------------
void Exit(TStringList *psl)
{
   #ifdef DEBUG_LOGFILE
   ExitDebug();
   #endif
   TRYDELETENULL(g_pMidi);
   if (psl)
	  psl->Clear();
   if (SoundClass())
	  {
	  try
		 {
		 Stop(psl);
//         Sleep(1000);
		 }
	  catch (...)
		 {
		 }
	  delete SoundClass(); // set to NULL by destructor
	  }
}
//------------------------------------------------------------------------------

//------------------------------------------------------------------------------
/// returns version information
//------------------------------------------------------------------------------
void Version(TStringList *psl)
{
   psl->Clear();
   AnsiString str = VString();
   psl->Values[SOUNDDLLPRO_STR_Version] = str;
}
//------------------------------------------------------------------------------

//------------------------------------------------------------------------------
/// Returns latest version and flag, if that version is newer than current version
//------------------------------------------------------------------------------
void CheckUpdate(TStringList *psl)
{
   psl->Clear();
   bool bUpdate = false;
   AnsiString str = GetLatestVersion(bUpdate);
   if (!str.Length())                     
      throw Exception("error retrieving latest vesrion from website");
   
   psl->Values[SOUNDDLLPRO_STR_Update]  = bUpdate ? "1" : "0";
   psl->Values[SOUNDDLLPRO_STR_Version] = str;
}
//------------------------------------------------------------------------------

//------------------------------------------------------------------------------
/// returns license information. Edition fix since 2.9.0.0 (freeware)
//------------------------------------------------------------------------------
void License(TStringList *psl)
{
   psl->Clear();
   // Get m
   AnsiString as = VString();
   as = as.SubString(1, as.Pos("."));
   psl->Values["Version"]  = as;
   psl->Values["Ed"]       = "VST+";
}
//------------------------------------------------------------------------------

//------------------------------------------------------------------------------
/// hides mixer form
//------------------------------------------------------------------------------
void Hide(TStringList *psl)
{
   HideMixer(psl);
}
//------------------------------------------------------------------------------

//------------------------------------------------------------------------------
/// shows track visualization
//------------------------------------------------------------------------------
void   ShowTracks(TStringList *psl)
{
   TFormStyle fs = (psl->Values[SOUNDDLLPRO_PAR_TOPMOST] == "1") ? fsStayOnTop : Forms::fsNormal;
   bool bForeGround = !(psl->Values[SOUNDDLLPRO_PAR_FOREGROUND] == "0");
   bool bShowWaveData = psl->Values[SOUNDDLLPRO_PAR_WAVEDATA] != "0";
   psl->Clear();
   if (!SoundClass()->m_vTracks.size())
      throw Exception("no tracks configured to be displayed");

   SoundClass()->m_pfrmTracks->Init();
   if (!SoundClass()->m_pfrmTracks->Visible)
      {
      SoundClass()->m_pfrmTracks->FormStyle = fs;
      SoundClass()->m_pfrmTracks->Show();
      }
   SoundClass()->m_pfrmTracks->UpdateTracks(true, bShowWaveData);
   if (bForeGround)
      SetForegroundWindow(SoundClass()->m_pfrmTracks->Handle);
}
//------------------------------------------------------------------------------

//------------------------------------------------------------------------------
/// hides track visualization
//------------------------------------------------------------------------------
void   HideTracks(TStringList *psl)
{
   psl->Clear();
   SoundClass()->m_pfrmTracks->Hide();
}
//------------------------------------------------------------------------------

//------------------------------------------------------------------------------
/// updates track visualization
//------------------------------------------------------------------------------
void   UpdateTracks(TStringList *psl)
{
   bool bShowWaveData = psl->Values[SOUNDDLLPRO_PAR_WAVEDATA] != "0";
   psl->Clear();
   SoundClass()->m_pfrmTracks->UpdateTracks(true, bShowWaveData);
}
//------------------------------------------------------------------------------

//------------------------------------------------------------------------------
/// shows mixer
//------------------------------------------------------------------------------
void   ShowMixer(TStringList *psl)
{
   TFormStyle fs = (psl->Values[SOUNDDLLPRO_PAR_TOPMOST] == "1") ? fsStayOnTop : Forms::fsNormal;
   bool bForeGround = !(psl->Values[SOUNDDLLPRO_PAR_FOREGROUND] == "0");
   int nTracks = (int)GetInt(psl, SOUNDDLLPRO_PAR_TRACKS, 1, VAL_POS_OR_ZERO);
   int nOutputs = (int)GetInt(psl, SOUNDDLLPRO_PAR_OUTPUTS, 1, VAL_POS_OR_ZERO);
   int nInputs = (int)GetInt(psl, SOUNDDLLPRO_PAR_INPUTS, 1, VAL_POS_OR_ZERO);
   psl->Clear();
   if (!SoundClass()->m_pfrmMixer->Visible)
	  {
	  SoundClass()->m_pfrmMixer->FormStyle = fs;
	  if (!nTracks)
		 SoundClass()->m_pfrmMixer->pnlTracks->Width = 0;
	  if (!nOutputs)
		 SoundClass()->m_pfrmMixer->pnlOutputs->Width = 0;
	  if (!nInputs)
		 SoundClass()->m_pfrmMixer->pnlInputs->Width = 0;

	  SoundClass()->m_pfrmMixer->Show();
	  }
   if (bForeGround)
	  SetForegroundWindow(SoundClass()->m_pfrmMixer->Handle);
}
//------------------------------------------------------------------------------

//------------------------------------------------------------------------------
/// hides mixer
//------------------------------------------------------------------------------
void   HideMixer(TStringList *psl)
{
   psl->Clear();
   SoundClass()->m_pfrmMixer->Hide();
}
//------------------------------------------------------------------------------

//------------------------------------------------------------------------------
/// shows driver control panel
//------------------------------------------------------------------------------
void ShowControlPanel(TStringList *psl)
{
   AnsiString strDriver = psl->Values[SOUNDDLLPRO_PAR_DRIVER];
   psl->Clear();
   if (!strDriver.IsEmpty())
      {
      if (SoundClass())
         throw Exception("SoundMexPro is already initialized: do not specify a value in 'driver' when calling command!");
      }
   else
      strDriver = "0"; // default

   bool bClassCreated = false;
   try
      {
      // if not already initialized create class temporarily
      if (!SoundClass())
         {
         bClassCreated = true;
         new SoundDllProMain();
         if (!SoundClass())
            throw Exception("cannot create temporary sound class");
         SoundClass()->SoundLoadDriver(strDriver);
         }
      SoundClass()->SoundShowControlPanel();
      }
   __finally
      {
      if (bClassCreated && SoundClass())
         {
         SoundClass()->SoundUnloadDriver();
         delete SoundClass();
         }
      }
}
//------------------------------------------------------------------------------

//------------------------------------------------------------------------------
/// returns all installed asio driver names
//------------------------------------------------------------------------------
void GetDrivers(TStringList *psl)
{
   psl->Clear();
   // go through drivers...
   unsigned int nDriverIndex;
   AnsiString str;
   bool bClassCreated = false;
   try
      {
      // if not already initialized create class temporarily
      if (!SoundClass())
         {
         bClassCreated = true;
         new SoundDllProMain();
         if (!SoundClass())
            throw Exception("cannot create temporary sound class");
         }
      for (nDriverIndex = 0; nDriverIndex < SoundClass()->SoundNumDrivers(); nDriverIndex++)
         str += AnsiQuotedStr(SoundClass()->SoundDriverName(nDriverIndex), '"') + ",";
      }
   __finally
      {
      if (bClassCreated && SoundClass())
         delete SoundClass();
      }
   // remove last ','
   RemoveTrailingChar(str);
   psl->Values[SOUNDDLLPRO_PAR_DRIVER] = str;
}
//------------------------------------------------------------------------------

//------------------------------------------------------------------------------
/// returns status of all asio drivers (tries to load each driver!)
//------------------------------------------------------------------------------
void GetDriverStatus(TStringList *psl)
{
   psl->Clear();
   AnsiString str;
   if (SoundClass())
      throw Exception("command only allowed if SoundMexPro is not already initialized");

   try
      {
      // create class temporarily
      new SoundDllProMain();
      if (!SoundClass())
         throw Exception("cannot create temporary sound class");
      unsigned int nDriverIndex;
      // try to load all drivers
      for (nDriverIndex = 0; nDriverIndex < SoundClass()->SoundNumDrivers(); nDriverIndex++)
         {
         try
            {
            // try to load it
            SoundClass()->SoundLoadDriver(IntToStr((int)nDriverIndex));
            // check, if any channels found (error if not)
            if (  (SoundClass()->SoundChannels(Asio::OUTPUT) == 0 || SoundClass()->SoundChannels(Asio::OUTPUT)< 0)
               && (SoundClass()->SoundChannels(Asio::INPUT) == 0  || SoundClass()->SoundChannels(Asio::INPUT) < 0)
               )
               throw Exception("");
            // append a 1 on success...
            str += "1,";
            }
         catch (...)
            {
            // ... or a 0 on failure
            str += "0,";
            }
         // safe unloading
         try
            {
            SoundClass()->SoundUnloadDriver();
            }
         catch (...)
            {
            }
         }
      }
   __finally
      {
      if (SoundClass())
         {
         SoundClass()->SoundUnloadDriver();
         delete SoundClass();
         }
      }
   // remove last ','
   RemoveTrailingChar(str);
   SetValue(psl, SOUNDDLLPRO_PAR_VALUE, str);
}
//------------------------------------------------------------------------------

//------------------------------------------------------------------------------
/// returns all channels of an asio driver
//------------------------------------------------------------------------------
void GetChannels(TStringList *psl)
{
   AnsiString strDriver = psl->Values[SOUNDDLLPRO_PAR_DRIVER];
   if (!strDriver.IsEmpty())
      {
      if (SoundClass())
         throw Exception("SoundMexPro is already initialized: do not specify a value in 'driver' when calling command!");
      }
   else
      strDriver = "0"; // default
   psl->Clear();
   AnsiString strInput, strOutput;
   bool bClassCreated = false;
   try
      {
      // if not already initialized create class temporarily
      if (!SoundClass())
         {
         bClassCreated = true;
         new SoundDllProMain();
         if (!SoundClass())
            throw Exception("cannot create temporary sound class");
         SoundClass()->SoundLoadDriver(strDriver);
         }
      // go through channels
      int nChannelIndex;
      int nChannels = SoundClass()->SoundChannels(Asio::OUTPUT);
      if (nChannels> -1)
         {
         for (nChannelIndex = 0; nChannelIndex < nChannels; nChannelIndex++)
            strOutput += AnsiQuotedStr(SoundClass()->SoundChannelName((unsigned int)nChannelIndex, Asio::OUTPUT), '"') + ",";
         }
      nChannels = SoundClass()->SoundChannels(Asio::INPUT);
      if (nChannels> -1)
         {
         for (nChannelIndex = 0; nChannelIndex < nChannels; nChannelIndex++)
            strInput += AnsiQuotedStr(SoundClass()->SoundChannelName((unsigned int)nChannelIndex, Asio::INPUT), '"') + ",";
         }
      }
   __finally
      {
      if (bClassCreated && SoundClass())
         {
         SoundClass()->SoundUnloadDriver();
         delete SoundClass();
         }
      }

   // remove last ','
   RemoveTrailingChar(strOutput);
   RemoveTrailingChar(strInput);
   SetValue(psl, SOUNDDLLPRO_PAR_OUTPUT, strOutput);
   SetValue(psl, SOUNDDLLPRO_PAR_INPUT, strInput);
}
//------------------------------------------------------------------------------

//------------------------------------------------------------------------------
/// returns name of current asio driver
//------------------------------------------------------------------------------
void GetActiveDriver(TStringList *psl)
{
   psl->Clear();
   psl->Values[SOUNDDLLPRO_PAR_DRIVER] = AnsiQuotedStr(SoundClass()->SoundDriverName(SoundClass()->SoundCurrentDriverIndex()), '"');
}
//------------------------------------------------------------------------------

//------------------------------------------------------------------------------
/// returns initialised channels of current asio driver
//------------------------------------------------------------------------------
void GetActiveChannels(TStringList *psl)
{
   psl->Clear();
   AnsiString strInput, strOutput;
   // go through channels
   unsigned int nChannelIndex;
   for (nChannelIndex = 0; nChannelIndex < SoundClass()->SoundActiveChannels(Asio::OUTPUT); nChannelIndex++)
      strOutput += AnsiQuotedStr(SoundClass()->SoundGetActiveChannelName(nChannelIndex, Asio::OUTPUT), '"') + ",";
   for (nChannelIndex = 0; nChannelIndex < SoundClass()->SoundActiveChannels(Asio::INPUT); nChannelIndex++)
      strInput += AnsiQuotedStr(SoundClass()->SoundGetActiveChannelName(nChannelIndex, Asio::INPUT), '"') + ",";
   // remove last ','
   RemoveTrailingChar(strOutput);
   RemoveTrailingChar(strInput);

   SetValue(psl, SOUNDDLLPRO_PAR_OUTPUT, strOutput);
   SetValue(psl, SOUNDDLLPRO_PAR_INPUT, strInput);
}
//------------------------------------------------------------------------------

//------------------------------------------------------------------------------
/// returns properties, here sound buffer size current samplerate, supported
/// sample rates, latencies
//------------------------------------------------------------------------------
void GetProperties(TStringList *psl)
{
   psl->Clear();
   psl->Values[SOUNDDLLPRO_PAR_SAMPLERATE]   = SoundClass()->SoundGetSampleRate();
   psl->Values[SOUNDDLLPRO_PAR_BUFSIZE]      = SoundClass()->SoundBufsizeSamples();
   psl->Values[SOUNDDLLPRO_PAR_SRATES]       = SoundClass()->GetSupportedSamplerateString();
   psl->Values[SOUNDDLLPRO_PAR_SOUNDFORMAT]  = SoundClass()->GetSoundFormatString();
   psl->Values[SOUNDDLLPRO_PAR_LATENCYIN]    = SoundClass()->SoundGetLatency(Asio::INPUT);
   psl->Values[SOUNDDLLPRO_PAR_LATENCYOUT]   = SoundClass()->SoundGetLatency(Asio::OUTPUT);
}
//------------------------------------------------------------------------------

//------------------------------------------------------------------------------
/// loads a memory data vector to one or more tracks
//------------------------------------------------------------------------------
void LoadMem(TStringList *psl)
{
   try
      {
      std::vector<int> viTrack = SoundClass()->ConvertSoundChannelArgument(psl->Values[SOUNDDLLPRO_PAR_TRACK], CT_TRACK);
      unsigned int nTracks    = (unsigned int)viTrack.size();
      unsigned int nSamples   = (unsigned int)GetInt(psl, SOUNDDLLPRO_PAR_SAMPLES, VAL_POS);
      unsigned int nChannels  = (unsigned int)GetInt(psl, SOUNDDLLPRO_PAR_CHANNELS, VAL_POS);
      unsigned int nLoopCount = (unsigned int)GetInt(psl, SOUNDDLLPRO_PAR_LOOPCOUNT, 1, VAL_POS_OR_ZERO);
      uint64_t nOffset        = (uint64_t)GetInt(psl, SOUNDDLLPRO_PAR_OFFSET, 0, VAL_POS_OR_ZERO);
      int64_t nStartOffset    = GetInt(psl, SOUNDDLLPRO_PAR_STARTOFFSET, 0);
      AnsiString strName      = psl->Values[SOUNDDLLPRO_PAR_NAME];
      float fGain             = GetFloat(psl, SOUNDDLLPRO_PAR_GAIN, 1.0f);
      unsigned int nRampLen   = (unsigned int)GetInt(psl, SOUNDDLLPRO_PAR_RAMPLEN, 0, VAL_POS_OR_ZERO);
      unsigned int nLoopRampLen  = (unsigned int)GetInt(psl, SOUNDDLLPRO_PAR_LOOPRAMPLEN, 0, VAL_POS_OR_ZERO);
      bool bLoopCrossfade        = !(  GetInt(psl, SOUNDDLLPRO_PAR_LOOPCROSSFADE, 0, VAL_POS_OR_ZERO) == 0 );
      unsigned int nCrossfadeLen  = (unsigned int)GetInt(psl, SOUNDDLLPRO_PAR_CROSSFADELEN, 0, VAL_POS_OR_ZERO);
      // check data pointer or name of shared memory respectively
      if (psl->Values[SOUNDDLLPRO_PAR_DATA].IsEmpty())
         throw Exception("value for parameter '" + AnsiString(SOUNDDLLPRO_PAR_DATA) + "' must not be empty");
      NativeInt nDataPointer = (NativeInt)GetInt(psl, SOUNDDLLPRO_PAR_DATA, VAL_NON_ZERO);
      // dumb read check (not guaranteed that everything is fine...)
      if (!nDataPointer)
         throw Exception("bad data read pointer");
      // NOTE: track count must devisable by channel count of passed memory!
      if ((nTracks % nChannels) != 0)
         throw Exception("number of tracks must be a multiple of the number of data channels!");
      // set random offset if requested. NOTE: to be done here to have it identical for
      // all channels
      if (nStartOffset < 0)
         nStartOffset = (unsigned int)random((int)(nSamples - nLoopRampLen));
      std::valarray<unsigned int> viOffset(nTracks);
      viOffset = 0;
      // store SDPOutputData pointers  AND total number of samples
      // in track BEFORE loading new data to adjust global position later in blocking loop
      std::vector<SDPOutputData* > vpsdpod;
      unsigned int nTrackIndex;
      for (nTrackIndex = 0; nTrackIndex < nTracks; nTrackIndex++)
         {
         viOffset[nTrackIndex] = (unsigned int)SoundClass()->m_vTracks[(unsigned int)viTrack[nTrackIndex]]->NumTrackSamples();
         // NOTE: create SDPOD_AUDIO within loop to use default values from constructor!!
         SDPOD_AUDIO sdpod;
         sdpod.bIsFile        = false;
         sdpod.pData          = (double*)nDataPointer;
         sdpod.nChannelIndex  = (nTrackIndex % nChannels);
         sdpod.nNumChannels   = nChannels;
         sdpod.nNumSamples    = nSamples;
         sdpod.nLoopCount     = nLoopCount;
         sdpod.nOffset        = nOffset;
         sdpod.nStartOffset   = nStartOffset;
         sdpod.strName        = strName;
         sdpod.fGain          = fGain;
         sdpod.nRampLenght      = nRampLen;
         sdpod.nLoopRampLenght  = nLoopRampLen;
         sdpod.bLoopCrossfade   = bLoopCrossfade;
         sdpod.nCrossfadeLengthLeft = nCrossfadeLen;

         vpsdpod.push_back(SoundClass()->m_vTracks[(unsigned int)viTrack[nTrackIndex]]->LoadAudio(sdpod));
         }
      //*************
      // BUGFIX on February 24th 2009: alignment fails for adding endless looped data here
      // which have a 'length' of 0. So we have to reset nSamples for endless loop!
      if (!nLoopCount)
         nSamples = 0;
      //*************
      // get the longest track
      uint64_t nMaxLen = viOffset.max();
      // NOTE: to keep multiple passed channels synchronous, we have to block the processing
      // thread here and then
      // -  adjust global position for the file, i.e. if it's a multichannel file, we have
      //    to add samples  to global position one or more channels to keep data aligned
      //  - push the new data on all channels before releasing the critical section again!
      // The 'lock' is done as short as possible to avoid xruns, and therefore it is _not_ done in the upper
      // loop that may take a while (memory allocation!)
      EnterCriticalSection(&SoundClass()->m_csProcess);
      try
         {
         uint64_t nLoadPosition = SoundClass()->GetLoadPosition();
         if (nMaxLen < nLoadPosition)
            nMaxLen = nLoadPosition;
         for (nTrackIndex = 0; nTrackIndex < nTracks; nTrackIndex++)
            {
            // adjust offset
            vpsdpod[nTrackIndex]->SetGlobalPosition(nMaxLen + (uint64_t)nOffset);
            // push data
            SoundClass()->m_vTracks[(unsigned int)viTrack[nTrackIndex]]->PushData();
            }
         }
      __finally
         {
         LeaveCriticalSection(&SoundClass()->m_csProcess);
         }
      }
   __finally
      {
      psl->Clear();
      }
}
//------------------------------------------------------------------------------

//------------------------------------------------------------------------------
/// loads one or more channels of an audio file to one or more tracks
//------------------------------------------------------------------------------
void LoadFile(TStringList *psl)
{
   try
      {
      std::vector<int> viTrack   = SoundClass()->ConvertSoundChannelArgument(psl->Values[SOUNDDLLPRO_PAR_TRACK], CT_TRACK, CCA_ARGS_NEGALLOWED);
      unsigned int nTracks       = (unsigned int)viTrack.size();
      AnsiString strFileName     = psl->Values[SOUNDDLLPRO_PAR_FILENAME];
      int nLoopCount             = (int)GetInt(psl, SOUNDDLLPRO_PAR_LOOPCOUNT, 1, VAL_POS_OR_ZERO);
      uint64_t nOffset           = (uint64_t)GetInt(psl, SOUNDDLLPRO_PAR_OFFSET, 0, VAL_POS_OR_ZERO);
      int64_t nStartOffset       = GetInt(psl, SOUNDDLLPRO_PAR_STARTOFFSET, 0, VAL_ALL);
      int64_t nFileOffset        = GetInt(psl, SOUNDDLLPRO_PAR_FILEOFFSET, 0, VAL_ALL);
      uint64_t nLength           = (uint64_t)GetInt(psl, SOUNDDLLPRO_PAR_LENGTH, 0, VAL_POS_OR_ZERO);
      float fGain                = GetFloat(psl, SOUNDDLLPRO_PAR_GAIN, 1.0f);
      unsigned int nRampLen      = (unsigned int)GetInt(psl, SOUNDDLLPRO_PAR_RAMPLEN, 0, VAL_POS_OR_ZERO);
      unsigned int nLoopRampLen  = (unsigned int)GetInt(psl, SOUNDDLLPRO_PAR_LOOPRAMPLEN, 0, VAL_POS_OR_ZERO);
      bool bLoopCrossfade        = !(GetInt(psl, SOUNDDLLPRO_PAR_LOOPCROSSFADE, 0, VAL_POS_OR_ZERO) == 0);
      unsigned int nCrossfadeLen = (unsigned int)GetInt(psl, SOUNDDLLPRO_PAR_CROSSFADELEN, 0, VAL_POS_OR_ZERO);
      // file present at all?
      if (!FileExists(strFileName))
         throw Exception("file '" + ExpandFileName(strFileName) + "' not found");

      unsigned int nChannels, nSamples;
      double dSampleRate;
      SDPWaveReader::WaveFileProperties(strFileName, nChannels, nSamples, dSampleRate);

      // float comparison by purpose
      #pragma clang diagnostic push
      #pragma clang diagnostic ignored "-Wfloat-equal"
      if (dSampleRate != SoundClass()->SoundGetSampleRate())
         throw Exception("file samplerate does not match device samplerate (" + FloatToStr((long double)SoundClass()->SoundGetSampleRate()) + ")");
      #pragma clang diagnostic pop
      
      // NOTE: track count must be devisable by channel count in file!
      if ((nTracks % nChannels) != 0)
         throw Exception("number of tracks must be a multiple of the number of available file channels (" + IntToStr((int)nChannels) + ")! ");
      // set random offsets if requested. NOTE: to be done here to have it identical for
      // all channels
      if (nFileOffset < 0)
         nFileOffset = random((int)nSamples);
      if (nStartOffset < 0)
         {
         unsigned int nUsedSamples = nSamples;
         if (nLength != 0)
            nUsedSamples = (unsigned int)nLength;
         nStartOffset = (unsigned int)random((int)(nUsedSamples - nLoopRampLen));
         }

      std::valarray<unsigned int> viOffset(nTracks);
      viOffset = 0;
      // store SDPOutputData pointers returned by LoadFile AND total number of samples
      // in track BEFORE loading new data to adjust global position later in blocking loop
      std::vector<SDPOutputData* > vpsdpod(nTracks);
      unsigned int nTrackIndex;
      for (nTrackIndex = 0; nTrackIndex < nTracks; nTrackIndex++)
         {
         if (viTrack[nTrackIndex] > -1)
            {
            viOffset[nTrackIndex] = (unsigned int)SoundClass()->m_vTracks[(unsigned int)viTrack[nTrackIndex]]->NumTrackSamples();
            // NOTE: create SDPOD_AUDIO within loop to use default values from constructor!!
            SDPOD_AUDIO sdpod;
            sdpod.bIsFile        = true;
            sdpod.strName        = strFileName;
            sdpod.nChannelIndex  = (unsigned int)(nTrackIndex % nChannels);
            sdpod.nLoopCount     = (unsigned int)nLoopCount;
            sdpod.nOffset        = nOffset;
            sdpod.nStartOffset   = nStartOffset;
            sdpod.nFileOffset    = nFileOffset;
            sdpod.nNumSamples    = nLength;
            sdpod.fGain          = fGain;
            sdpod.nRampLenght      = nRampLen;
            sdpod.nLoopRampLenght  = nLoopRampLen;
            sdpod.bLoopCrossfade   = bLoopCrossfade;
            sdpod.nCrossfadeLengthLeft = nCrossfadeLen;
            vpsdpod[nTrackIndex] = SoundClass()->m_vTracks[(unsigned int)viTrack[nTrackIndex]]->LoadAudio(sdpod);
            }
         }
	  // reset nSamples for endless loop!
      if (!nLoopCount)
         nSamples = 0;
      // get the longest track
      uint64_t nMaxLen = viOffset.max();
      // NOTE: to keep multiple passed channels synchroneous, we have to block the processing
      // thread here and
      // -  adjust global position for the file, i.e. if it's a multichannel file, we have
      //    to add samples  to global position one or more channels to keep the file aligned
      // -  push the new data on all channels before releasing the critical section again!
      // The 'lock' is done as short as possible to avoid xruns, and therefore it is _not_ done in the upper
      // loop that may take a while (memory allocation!)
      EnterCriticalSection(&SoundClass()->m_csProcess);
      try
         {
         uint64_t nLoadPosition = SoundClass()->GetLoadPosition();
         if (nMaxLen < nLoadPosition)
            nMaxLen = nLoadPosition;
         for (nTrackIndex = 0; nTrackIndex < nTracks; nTrackIndex++)
            {
            // check, if file channels to be used at all
            if (viTrack[nTrackIndex] > -1)
               {
               // adjust offset
               vpsdpod[nTrackIndex]->SetGlobalPosition(nMaxLen + nOffset);
               // push data
               SoundClass()->m_vTracks[(unsigned int)viTrack[nTrackIndex]]->PushData();
               }
            }
         }
      __finally
         {
         LeaveCriticalSection(&SoundClass()->m_csProcess);
         }
      }
   __finally
      {
      psl->Clear();
      }
}
//------------------------------------------------------------------------------

//------------------------------------------------------------------------------
/// clears all loaded audio data
//------------------------------------------------------------------------------
void ClearData(TStringList *psl)
{
   psl->Clear();
   SoundClass()->ClearData();
}
//------------------------------------------------------------------------------

//------------------------------------------------------------------------------
/// clears all loaded audio data in one ore more tracks
//------------------------------------------------------------------------------
void   ClearTrack(TStringList *psl)
{
   std::vector<int> viTrack = SoundClass()->ConvertSoundChannelArgument(psl->Values[SOUNDDLLPRO_PAR_TRACK], CT_TRACK);
   psl->Clear();
   SoundClass()->ClearTracks(viTrack);
}
//------------------------------------------------------------------------------

//------------------------------------------------------------------------------
/// starts asio device with 'length' flag
//------------------------------------------------------------------------------
void Start(TStringList *psl)
{
   if (SoundClass()->DeviceIsRunning())
      throw Exception("device is already running");
   // in file2file-mode clear parameters to force default for all values!
   if (SoundClass()->IsFile2File())
      psl->Clear();
   int nLength;
   // set default value to 0 (endless play) if NO output channels used
   if (SoundClass()->SoundActiveChannels(Asio::OUTPUT) == 0)
      nLength  = (int)GetInt(psl, SOUNDDLLPRO_PAR_LENGTH, 0, VAL_ALL);
   // or to -1 (stop after playback is complete) else
   else
      nLength  = (int)GetInt(psl, SOUNDDLLPRO_PAR_LENGTH, -1, VAL_ALL);
   int nPause   = (int)GetInt(psl, SOUNDDLLPRO_PAR_PAUSE, 0, VAL_ALL);
   psl->Clear();
   SoundClass()->StartMode(nLength, nPause == 1);
   Application->ProcessMessages();
   SoundClass()->Start();
}
//------------------------------------------------------------------------------

//------------------------------------------------------------------------------
/// starts playback and recording with threshold feature
//------------------------------------------------------------------------------
void   StartThreshold(TStringList *psl)
{
   if (SoundClass()->DeviceIsRunning())
      throw Exception("device is already running");
   if (SoundClass()->IsFile2File())
      throw Exception("command not available in file2file-mode");

   std::vector<int> viChannel = SoundClass()->ConvertSoundChannelArgument(psl->Values[SOUNDDLLPRO_PAR_CHANNEL], CT_INPUT);
   // check if buffer size is to be set
   AnsiString str = psl->Values[SOUNDDLLPRO_PAR_VALUE];
   int nLength  = (int)GetInt(psl, SOUNDDLLPRO_PAR_LENGTH, -1, VAL_ALL);
   int nPause   = (int)GetInt(psl, SOUNDDLLPRO_PAR_PAUSE, 0, VAL_ALL);

   psl->Clear();
   // set default mode
   int nMode = SDA_THRSHLDMODE_OR;
   if (!str.IsEmpty())
      {
      if (!IsDouble(str))
         throw Exception("invalid threshold value (must be between 0 and 1)");
      float f = (float)StrToDouble(str);
      if (f < 0.0f || f >= 1.0f)
         throw Exception("invalid threshold value (must be between 0 and 1)");
      AnsiString strMode = psl->Values[SOUNDDLLPRO_PAR_MODE];
      if (strMode == "0")
         nMode = SDA_THRSHLDMODE_AND;
      else if (!strMode.IsEmpty() && strMode != "1")
         throw Exception("invalid threshold mode ('0' or '1')");
      SoundClass()->SetStartThreshold(viChannel, f, nMode);
      }
   SoundClass()->StartMode(nLength, nPause == 1);
   Application->ProcessMessages();
   SoundClass()->Start();
   psl->Values[SOUNDDLLPRO_PAR_VALUE]  = DoubleToStr((double)SoundClass()->GetStartThreshold(nMode));
   psl->Values[SOUNDDLLPRO_PAR_MODE]   = IntToStr(nMode);
}
//------------------------------------------------------------------------------

//------------------------------------------------------------------------------
/// stops device
//------------------------------------------------------------------------------
void Stop(TStringList *psl)
{
   if (psl)
      psl->Clear();
   if (SoundClass())
      SoundClass()->Stop(true, true);
}
//------------------------------------------------------------------------------

//------------------------------------------------------------------------------
/// returns running status of device
//------------------------------------------------------------------------------
void Started(TStringList *psl)
{
   psl->Clear();
   // if waiting for start-threshold, then return false as well!
   psl->Values[SOUNDDLLPRO_PAR_VALUE] =  (!SoundClass()->IsWaitingForStartThreshold() && SoundClass()->DeviceIsRunning()) ? "1" : "0";
}
//------------------------------------------------------------------------------

//------------------------------------------------------------------------------
/// sets and returns volume of one or more output channels
//------------------------------------------------------------------------------
void Volume(TStringList *psl)
{
   std::vector<int> viChannel = SoundClass()->ConvertSoundChannelArgument(psl->Values[SOUNDDLLPRO_PAR_CHANNEL], CT_OUTPUT);
   std::vector<float> vfAsioVolumes;
   unsigned int nChannelIndex;
   // check if volume is to be set
   AnsiString str = psl->Values[SOUNDDLLPRO_PAR_VALUE];
   psl->Clear();
   if (!str.IsEmpty())
      {
      std::vector<float> vf;
      TStringList *pslTmp = new TStringList();
      try
         {
         pslTmp->Delimiter = ',';
         pslTmp->DelimitedText = str;
         for (nChannelIndex = 0; nChannelIndex < (unsigned int)pslTmp->Count; nChannelIndex++)
            {
            if (!IsDouble(pslTmp->Strings[(int)nChannelIndex]))
               throw Exception("invalid volume(s): must be float");
            vf.push_back((float)StrToDouble(pslTmp->Strings[(int)nChannelIndex]));
            }
         unsigned int nValues = (unsigned int)vf.size();
         // dim of new volume vector now must be 1 (all channels) or equal to dim of viChannel
         if (nValues != 1 && nValues != viChannel.size())
            throw Exception("volume value count matching error");
         // retrieve current volumes
         vfAsioVolumes = SoundClass()->GetGain();
         // set new volumes
         for (nChannelIndex = 0; nChannelIndex < viChannel.size(); nChannelIndex++)
            {
            if (nValues == 1)
               vfAsioVolumes[(unsigned int)viChannel[nChannelIndex]] = vf[0];
            else
               vfAsioVolumes[(unsigned int)viChannel[nChannelIndex]] = vf[nChannelIndex];
            }
         SoundClass()->SetOutputGain(vfAsioVolumes);
         }
      __finally
         {
         TRYDELETENULL(pslTmp);
         }
      }

   // write volumes to return
   str = "";
   vfAsioVolumes = SoundClass()->GetGain();
   for (nChannelIndex = 0; nChannelIndex < vfAsioVolumes.size(); nChannelIndex++)
      {
      str += DoubleToStr((double)vfAsioVolumes[nChannelIndex]) + ",";
      }
   // remove last ','
   RemoveTrailingChar(str);
   SetValue(psl, SOUNDDLLPRO_PAR_VALUE, str);
}
//------------------------------------------------------------------------------

//------------------------------------------------------------------------------
/// sets and returns volume of one or more virtual tracks (with ramping)
//------------------------------------------------------------------------------
void TrackVolume(TStringList *psl)
{
   std::vector<int> viTrack = SoundClass()->ConvertSoundChannelArgument(psl->Values[SOUNDDLLPRO_PAR_TRACK], CT_TRACK);
   unsigned int nRampLength   = (unsigned int)GetInt(psl, SOUNDDLLPRO_PAR_RAMPLEN, 0, VAL_POS_OR_ZERO);
   std::vector<float > vfTrackVolume;
   unsigned int nTrackIndex;
   // check if any volume is to be set
   AnsiString str = psl->Values[SOUNDDLLPRO_PAR_VALUE];
   psl->Clear();
   if (!str.IsEmpty())
      {
      // retrieve current gains
      vfTrackVolume = SoundClass()->GetTrackGain();
      std::vector<float> vf;
      unsigned int nValues;
      TStringList *pslTmp = new TStringList();
      pslTmp->Delimiter = ',';
      try
         {
         pslTmp->DelimitedText = str;
         for (nTrackIndex = 0; nTrackIndex < (unsigned int)pslTmp->Count; nTrackIndex++)
            {
            if (!IsDouble(pslTmp->Strings[(int)nTrackIndex]))
               throw Exception("invalid volume(s): must be float");
            vf.push_back((float)StrToDouble(pslTmp->Strings[(int)nTrackIndex]));
            }
         nValues = (unsigned int)vf.size();
         // dim of new volume vector now must be 1 (all channels) or equal to dim of viChannel
         if (nValues != 1 && nValues != viTrack.size())
            throw Exception("volume value count matching error");
         // set new volumes
         for (nTrackIndex = 0; nTrackIndex < viTrack.size(); nTrackIndex++)
            {
            if (nValues == 1)
               vfTrackVolume[(unsigned int)viTrack[nTrackIndex]] = vf[0];
            else
               vfTrackVolume[(unsigned int)viTrack[nTrackIndex]] = vf[nTrackIndex];
            }
         SoundClass()->SetTrackGain(vfTrackVolume, nRampLength);
         }
      __finally
         {
         TRYDELETENULL(pslTmp);
         }
      }
   // write volumes to return
   str = "";
   vfTrackVolume = SoundClass()->GetTrackGain();
   for (nTrackIndex = 0; nTrackIndex < vfTrackVolume.size(); nTrackIndex++)
      str += DoubleToStr((double)vfTrackVolume[nTrackIndex]) + ",";
   // remove last ','
   RemoveTrailingChar(str);
   SetValue(psl, SOUNDDLLPRO_PAR_VALUE, str);
}
//------------------------------------------------------------------------------

//------------------------------------------------------------------------------
/// sets and returns volume of one or more input channels
//------------------------------------------------------------------------------
void RecVolume(TStringList *psl)
{
   std::vector<int> viChannel = SoundClass()->ConvertSoundChannelArgument(psl->Values[SOUNDDLLPRO_PAR_CHANNEL], CT_INPUT);
   std::vector<float> vfAsioVolumes;
   unsigned int nChannelIndex;
   // check if volume is to be set
   AnsiString str = psl->Values[SOUNDDLLPRO_PAR_VALUE];
   psl->Clear();
   if (!str.IsEmpty())
      {
      std::vector<float> vf;
      TStringList *pslTmp = new TStringList();
      try
         {
         pslTmp->Delimiter = ',';
         pslTmp->DelimitedText = str;
         for (nChannelIndex = 0; nChannelIndex < (unsigned int)pslTmp->Count; nChannelIndex++)
            {
            if (!IsDouble(pslTmp->Strings[(int)nChannelIndex]))
               throw Exception("invalid volume(s): must be float");
            vf.push_back((float)StrToDouble(pslTmp->Strings[(int)nChannelIndex]));
            }
         unsigned int nValues = (unsigned int)vf.size();
         // dim of new volume vector now must be 1 (all channels) or equal to dim of viChannel
         if (nValues != 1 && nValues != viChannel.size())
            throw Exception("volume value count matching error");
         // retrieve current volumes
         vfAsioVolumes = SoundClass()->GetRecGain();
         // set new volumes
         for (nChannelIndex = 0; nChannelIndex < viChannel.size(); nChannelIndex++)
            {
            if (nValues == 1)
               vfAsioVolumes[(unsigned int)viChannel[nChannelIndex]] = vf[0];
            else
               vfAsioVolumes[(unsigned int)viChannel[nChannelIndex]] = vf[nChannelIndex];
            }
         SoundClass()->SetRecGain(vfAsioVolumes);
         }
      __finally
         {
         TRYDELETENULL(pslTmp);
         }
      }

   // write volumes to return
   str = "";
   vfAsioVolumes = SoundClass()->GetRecGain();
   for (nChannelIndex = 0; nChannelIndex < vfAsioVolumes.size(); nChannelIndex++)
      {
      str += DoubleToStr((double)vfAsioVolumes[nChannelIndex]) + ",";
      }
   // remove last ','
   RemoveTrailingChar(str);
   SetValue(psl, SOUNDDLLPRO_PAR_VALUE, str);
}
//------------------------------------------------------------------------------

//------------------------------------------------------------------------------
/// sets and returns mute status of device
//------------------------------------------------------------------------------
void Mute(TStringList *psl)
{
   // check if volume is to be set
   AnsiString str = psl->Values[SOUNDDLLPRO_PAR_VALUE];
   psl->Clear();
   if (!str.IsEmpty())
      {
      int nValue;
      if (!TryStrToInt(str, nValue))
         throw Exception("invalid mute status: must be 1 or 0");
      switch (nValue)
         {
         case 0: SoundClass()->Unmute(); break;
         case 1: SoundClass()->Mute();   break;
         default: throw Exception("invalid mute status: must be 1 or 0");
         }
      }
   // write volumes to return
   str = SoundClass()->Muted() ? "1" : "0";
   psl->Values[SOUNDDLLPRO_PAR_VALUE] = str;
}
//------------------------------------------------------------------------------

//------------------------------------------------------------------------------
/// Sets and returns mute/solo status of tracks, outputs or inputs
//------------------------------------------------------------------------------
void ChannelMuteSolo(TStringList *psl)
{
   // check wich mute type (track, output, input) and set corresponding
   // values
   AnsiString str = psl->Values[SOUNDDLLPRO_STR_COMMAND].LowerCase();
   LPCTSTR lpcsz     = SOUNDDLLPRO_PAR_TRACK;
   TChannelType ct   = CT_TRACK;
   if (str == SOUNDDLLPRO_CMD_CHANNELMUTE || str == SOUNDDLLPRO_CMD_CHANNELSOLO)
      {
      ct             = CT_OUTPUT;
      lpcsz          = SOUNDDLLPRO_PAR_OUTPUT;
      }
   else if (str == SOUNDDLLPRO_CMD_RECMUTE || str == SOUNDDLLPRO_CMD_RECSOLO)
      {
      ct             = CT_INPUT;
      lpcsz          = SOUNDDLLPRO_PAR_INPUT;
      }
   else if (str != SOUNDDLLPRO_CMD_TRACKMUTE && str != SOUNDDLLPRO_CMD_TRACKSOLO)
      throw Exception("invalid command name");
   bool bSolo = str.Pos("solo") > 0;
   std::vector<int> viChannel = SoundClass()->ConvertSoundChannelArgument(psl->Values[lpcsz], ct);
   str = psl->Values[SOUNDDLLPRO_PAR_VALUE];
   psl->Clear();
   unsigned int nIndex;
   // retrieve current mute status values
   std::valarray<bool > vab = SoundClass()->GetChannelMute(ct);
   if (bSolo)
      vab = SoundClass()->GetChannelSolo(ct);
   // any value is to be set?
   if (!str.IsEmpty())
      {
      TStringList *pslTmp = new TStringList();
      try
         {
         pslTmp->Delimiter = ',';
         pslTmp->DelimitedText = str;
         unsigned int nValues = (unsigned int)pslTmp->Count;
         // only one value or same as passed channel count
         if (nValues != 1 && nValues != viChannel.size())
            throw Exception("value count matching error");
         // check single values
         for (nIndex = 0; nIndex < nValues; nIndex++)
            {
            if (pslTmp->Strings[(int)nIndex] != "0" && pslTmp->Strings[(int)nIndex] != "1")
               throw Exception("invalid values: must be 0 or 1");
            }
         // write new values
         for (nIndex = 0; nIndex < viChannel.size(); nIndex++)
            {
            if (nValues == 1)
               vab[(unsigned int)viChannel[nIndex]] = pslTmp->Strings[0] == "1";
            else
               vab[(unsigned int)viChannel[nIndex]] = pslTmp->Strings[(int)nIndex] == "1";
            }

         if (bSolo)
            SoundClass()->SetChannelSolo(vab, ct);
         else
            SoundClass()->SetChannelMute(vab, ct);
         }
      __finally
         {
         TRYDELETENULL(pslTmp);
         }
      }
   // write mute status values to return
   str = "";
   for (nIndex = 0; nIndex < vab.size(); nIndex++)
      str += vab[nIndex] ?  "1," : "0,";
   // remove last ','
   RemoveTrailingChar(str);
   SetValue(psl, SOUNDDLLPRO_PAR_VALUE, str);
}
//------------------------------------------------------------------------------

//------------------------------------------------------------------------------
/// waits for one or moe tracks to finish their output with optional timeout
//------------------------------------------------------------------------------
void ChannelName(TStringList *psl)
{
   // check wich channel name type (track, output, input)
   AnsiString str = psl->Values[SOUNDDLLPRO_STR_COMMAND].LowerCase();
   // convert names into a TStringList
   TStringList *pslNames = new TStringList();
   TStringList *pslAllNames = new TStringList();
   AnsiString strNames = psl->Values[SOUNDDLLPRO_PAR_NAME];
   try
      {
      try
         {
         pslNames->Delimiter = ',';
         strNames = StringReplace(strNames, "'", "\"", TReplaceFlags() << rfReplaceAll);
         pslNames->DelimitedText = strNames;
         pslAllNames->Delimiter = ',';
         // get channels/tracks and check constraint
         std::vector<int> vi;
         unsigned int n;
         if (str == SOUNDDLLPRO_CMD_TRACKNAME)
            {
            vi = SoundClass()->ConvertSoundChannelArgument(psl->Values[SOUNDDLLPRO_PAR_TRACK], CT_TRACK);
            if (pslNames->Count > 0 && pslNames->Count != (int)vi.size())
               throw Exception("number of names must match number of tracks or must be empty");
            SoundClass()->GetChannelNames(CT_TRACK, pslAllNames);
            }
         else if (str == SOUNDDLLPRO_CMD_CHANNELNAME)
            {
            vi = SoundClass()->ConvertSoundChannelArgument(psl->Values[SOUNDDLLPRO_PAR_OUTPUT], CT_OUTPUT);
            if (pslNames->Count > 0 && pslNames->Count != (int)vi.size())
               throw Exception("number of names must match number of output channels or must be empty");
            SoundClass()->GetChannelNames(CT_OUTPUT, pslAllNames);
            }
         else if (str == SOUNDDLLPRO_CMD_RECNAME)
            {
            vi = SoundClass()->ConvertSoundChannelArgument(psl->Values[SOUNDDLLPRO_PAR_OUTPUT], CT_INPUT);
            if (pslNames->Count > 0 && pslNames->Count != (int)vi.size())
               throw Exception("number of names must match number of input channels or must be empty");
            SoundClass()->GetChannelNames(CT_INPUT, pslAllNames);
            }
         else
            throw Exception("invalid command name");
         psl->Clear();

         // set new names in list with ALL names and check for duplicates
         int i;
         for (i = 0; i < pslNames->Count; i++)
            pslAllNames->Strings[vi[(unsigned int)i]] = pslNames->Strings[i];
         for (i = pslAllNames->Count-1; i >= 0; i--)
            {
            if (pslAllNames->IndexOf(pslAllNames->Strings[i]) != i)
               throw Exception("The name '" + pslAllNames->Strings[i] + "' is already in use");
            }
         // no duplicates: set names!!
         if (str == SOUNDDLLPRO_CMD_TRACKNAME)
            {
            for (i = 0; i < pslNames->Count; i++)
               SoundClass()->m_vTracks[(unsigned int)vi[(unsigned int)i]]->Name(pslNames->Strings[i]);
            }
         else if (str == SOUNDDLLPRO_CMD_CHANNELNAME)
            {
            for (i = 0; i < pslNames->Count; i++)
               SoundClass()->m_vOutput[(unsigned int)vi[(unsigned int)i]]->Name(pslNames->Strings[i]);
            }
         else
            {
            for (i = 0; i < pslNames->Count; i++)
               SoundClass()->m_vInput[(unsigned int)vi[(unsigned int)i]]->Name(pslNames->Strings[i]);
            }
         pslNames->Delimiter = ',';

         SoundClass()->m_pfrmTracks->UpdateTrackInfos();
         SoundClass()->m_pfrmMixer->UpdateChannelData(CT_TRACK);
         SoundClass()->m_pfrmMixer->UpdateChannelData(CT_INPUT);
         SoundClass()->m_pfrmMixer->UpdateChannelData(CT_OUTPUT);
         // return names that were set OR all names
         if (pslNames->Count)
            psl->Values[SOUNDDLLPRO_PAR_NAME] = pslNames->DelimitedText;
         else
            psl->Values[SOUNDDLLPRO_PAR_NAME] = pslAllNames->DelimitedText;
         }
      __finally
         {
         delete pslNames;
         delete pslAllNames;
         }
      }
   catch (...)
      {
      psl->Clear();
      }
}
//------------------------------------------------------------------------------

//------------------------------------------------------------------------------
/// waits for one or moe tracks to finish their output with optional timeout
//------------------------------------------------------------------------------
void Wait(TStringList *psl)
{
   if (!SoundClass()->DeviceIsRunning())
      return;
   DWORD dwTimeout   = (DWORD)GetInt(psl, SOUNDDLLPRO_PAR_TIMEOUT, 0, VAL_POS_OR_ZERO);
   bool bWaitForStop = psl->Values[SOUNDDLLPRO_PAR_MODE].LowerCase() == SOUNDDLLPRO_PAR_STOP;
   if (bWaitForStop)
      {
      psl->Clear();
      DWORD dw = GetTickCount();
      while (1)
         {
         Application->ProcessMessages();
         Sleep(20);
         // if device is stopped
         if (!SoundClass()->DeviceIsRunning())
            break;
         if (!!dwTimeout)
            {
            if (ElapsedSince(dw) > dwTimeout)
               throw Exception("a timeout occurred while waiting");
            }
         }
      }
   else
      {
      std::vector<int> viTrack = SoundClass()->ConvertSoundChannelArgument(psl->Values[SOUNDDLLPRO_PAR_TRACK], CT_TRACK);
      psl->Clear();
      unsigned int nTracks = (unsigned int)viTrack.size();
      // check if a channel is specified, where an endless loop is running
      for (unsigned int i = 0; i < nTracks; i++)
         {
         if (SoundClass()->m_vTracks[(unsigned int)viTrack[i]]->EndlessLoop())
            throw Exception("cannot wait for track " + IntToStr(viTrack[i]) + " because it runs an endless data loop");
         }

      DWORD dw = GetTickCount();
      while (1)
         {
         Application->ProcessMessages();
         Sleep(20);
         // if none is playing, break
         if (!SoundClass()->IsPlaying(viTrack))
            break;
         if (!!dwTimeout)
            {
            if (ElapsedSince(dw) > dwTimeout)
               throw Exception("a timeout occurred while waiting");
            }
         }
      }
}
//------------------------------------------------------------------------------

//------------------------------------------------------------------------------
/// sets and returns pause status of device
//------------------------------------------------------------------------------
void Pause(TStringList *psl)
{
   // check if volume is to be set
   AnsiString str = psl->Values[SOUNDDLLPRO_PAR_VALUE];
   psl->Clear();
   if (!str.IsEmpty())
      {
      int nValue;
      if (!TryStrToInt(str, nValue))
         throw Exception("invalid pause status: must be 1 or 0");
      // NOTE: 08-2020: removed call to CheckL in Unpause, pacause it might
      // lead to timing problems: checking the dongle might take a while!!
      switch (nValue)
         {
         case 0:  SoundClass()->Unpause(); break;
         case 1:  SoundClass()->Pause();  break;
         default: throw Exception("invalid pause status: must be 1 or 0");
         }
      }
   // write volumes to return
   str = SoundClass()->Paused() ? "1" : "0";
   psl->Values[SOUNDDLLPRO_PAR_VALUE] = str;
}
//------------------------------------------------------------------------------

//------------------------------------------------------------------------------
/// returns 'track load' of all tracks, i.e. how many data segments are pending
/// for output.
//------------------------------------------------------------------------------
void TrackLoad(TStringList *psl)
{
   AnsiString str;
   psl->Clear();
   // write current rack load to return
   for (unsigned int i = 0; i < SoundClass()->m_vTracks.size(); i++)
      str += IntToStr((int)SoundClass()->m_vTracks[i]->TrackLoad()) + ",";
   // remove last ','
   RemoveTrailingChar(str);
   SetValue(psl, SOUNDDLLPRO_PAR_VALUE, str);
}
//------------------------------------------------------------------------------

//------------------------------------------------------------------------------
/// returns playing status of all tracks
//------------------------------------------------------------------------------
void Playing(TStringList *psl)
{
   AnsiString str;
   psl->Clear();
   unsigned int nTrackIndex;
   for (nTrackIndex = 0; nTrackIndex < SoundClass()->m_vTracks.size(); nTrackIndex++)
      str += IntToStr((int)SoundClass()->m_vTracks[nTrackIndex]->IsPlaying()) + ",";
   // remove last ','
   RemoveTrailingChar(str);
   SetValue(psl, SOUNDDLLPRO_PAR_VALUE, str);
}
//------------------------------------------------------------------------------

//------------------------------------------------------------------------------
/// returns recording to file (!) status of all input channels
//------------------------------------------------------------------------------
void Recording(TStringList *psl)
{
   AnsiString str;
   psl->Clear();
   unsigned int nChannelIndex;
   for (nChannelIndex = 0; nChannelIndex < SoundClass()->SoundActiveChannels(Asio::INPUT); nChannelIndex++)
      str += IntToStr((int)SoundClass()->m_vInput[nChannelIndex]->IsRecording()) + ",";
   // remove last ','
   RemoveTrailingChar(str);
   SetValue(psl, SOUNDDLLPRO_PAR_VALUE, str);
}
//------------------------------------------------------------------------------

//------------------------------------------------------------------------------
/// returns number of xruns
//------------------------------------------------------------------------------
void NumXRuns(TStringList *psl)
{
   psl->Clear();
   // for compatbility reasons we (still) return the sum of all xruns in first return value
   std::valarray<unsigned int>& rvan = SoundClass()->GetXruns();
   psl->Values[SOUNDDLLPRO_PAR_VALUE]     = IntToStr((int)rvan.sum());
   if (SoundClass()->BufferedIO())
      psl->Values[SOUNDDLLPRO_PAR_XR_PROC]   = IntToStr((int)rvan[XR_PROC]);
   else
      psl->Values[SOUNDDLLPRO_PAR_XR_PROC]   = IntToStr((int)rvan[XR_RT]);
   psl->Values[SOUNDDLLPRO_PAR_XR_DONE]   = IntToStr((int)rvan[XR_DONE]);
}
//------------------------------------------------------------------------------

//------------------------------------------------------------------------------
/// sets values between 0 and 1 to be interpreted as clipping for one or more
/// channels on input ot output
//------------------------------------------------------------------------------
void ClipThreshold(TStringList *psl)
{
   Asio::Direction adDirection = Asio::INPUT;
   TChannelType ct = CT_INPUT;
   AnsiString strType = psl->Values[SOUNDDLLPRO_PAR_TYPE];
   if (strType.IsEmpty() || !strcmpi(strType.c_str(), SOUNDDLLPRO_VAL_OUTPUT))
      {
      adDirection = OUTPUT;
      ct = CT_OUTPUT;
      }
   else if (strcmpi(strType.c_str(), SOUNDDLLPRO_VAL_INPUT))
      throw Exception("invalid 'type'");
   std::vector<int> viChannel = SoundClass()->ConvertSoundChannelArgument(psl->Values[SOUNDDLLPRO_PAR_CHANNEL], ct);
   std::vector<float> vfThresholds;
   unsigned int nChannelIndex;
   // check if threshold is to be set
   AnsiString str = psl->Values[SOUNDDLLPRO_PAR_VALUE];
   psl->Clear();
   if (!str.IsEmpty())
      {
      std::vector<float> vf;
      TStringList *pslTmp = new TStringList();
      try
         {
         double d;
         pslTmp->Delimiter = ',';
         pslTmp->DelimitedText = str;
         for (nChannelIndex = 0; nChannelIndex < (unsigned int)pslTmp->Count; nChannelIndex++)
            {
            if (!IsDouble(pslTmp->Strings[(int)nChannelIndex]))
               throw Exception("invalid threshold(s): must be float between 0 and 1");
            d = StrToDouble(pslTmp->Strings[(int)nChannelIndex]);
            if (d < 0 || d > 1)
               throw Exception("invalid threshold(s): must be float between 0 and 1");
            vf.push_back((float)d);
            }
         unsigned int nValues = (unsigned int)vf.size();
         // dim of new threshold vector now must be 1 (all channels) or equal to dim of viChannel
         if (nValues != 1 && nValues != viChannel.size())
            throw Exception("threshold value count matching error");
         // retrieve current threshold
         vfThresholds = SoundClass()->GetClipThreshold(adDirection);
         // set new volumes
         for (nChannelIndex = 0; nChannelIndex < viChannel.size(); nChannelIndex++)
            {
            if (nValues == 1)
               vfThresholds[(unsigned int)viChannel[nChannelIndex]] = vf[0];
            else
               vfThresholds[(unsigned int)viChannel[nChannelIndex]] = vf[nChannelIndex];
            }
         EnterCriticalSection(&SoundClass()->m_csProcess);
         try
            {
            SoundClass()->SetClipThreshold(vfThresholds, adDirection);
            }
         __finally
            {
            LeaveCriticalSection(&SoundClass()->m_csProcess);
            }
         }
      __finally
         {
         TRYDELETENULL(pslTmp);
         }
      }

   // write thresholds to return
   str = "";
   vfThresholds = SoundClass()->GetClipThreshold(adDirection);
   for (nChannelIndex = 0; nChannelIndex < vfThresholds.size(); nChannelIndex++)
      {
      str += DoubleToStr((double)vfThresholds[nChannelIndex]) + ",";
      }
   // remove last ','
   RemoveTrailingChar(str);
   SetValue(psl, SOUNDDLLPRO_PAR_VALUE, str);
}
//------------------------------------------------------------------------------

//------------------------------------------------------------------------------
/// returns number of clipped buffers for all input and output channels
//------------------------------------------------------------------------------
void ClipCount(TStringList *psl)
{
   psl->Clear();
   AnsiString strInput, strOutput, strTrack;
   unsigned int nChannelIndex;
   // go through channels
   std::vector<unsigned int> vn = SoundClass()->GetClipCount(Asio::OUTPUT);
   for (nChannelIndex = 0; nChannelIndex < vn.size(); nChannelIndex++)
      strOutput += IntToStr((int)vn[nChannelIndex]) + ",";
   vn = SoundClass()->GetClipCount(Asio::INPUT);
   for (nChannelIndex = 0; nChannelIndex < vn.size(); nChannelIndex++)
      strInput += IntToStr((int)vn[nChannelIndex]) + ",";
   std::valarray<unsigned int> van = SoundClass()->GetTrackClipCount();
   for (nChannelIndex = 0; nChannelIndex < van.size(); nChannelIndex++)
      strTrack += IntToStr((int)van[nChannelIndex]) + ",";
   // remove last ','
   RemoveTrailingChar(strOutput);
   RemoveTrailingChar(strInput);
   RemoveTrailingChar(strTrack);
   SetValue(psl, SOUNDDLLPRO_PAR_OUTPUT, strOutput);
   SetValue(psl, SOUNDDLLPRO_PAR_INPUT, strInput);
   SetValue(psl, SOUNDDLLPRO_PAR_TRACK, strTrack);
}
//------------------------------------------------------------------------------

//------------------------------------------------------------------------------
/// resets clipcounts
//------------------------------------------------------------------------------
void ResetClipCount(TStringList *psl)
{
   psl->Clear();
   SoundClass()->ResetClipCount();
}
//------------------------------------------------------------------------------

//------------------------------------------------------------------------------
/// returns underrun status for all tracks
//------------------------------------------------------------------------------
void Underrun(TStringList *psl)
{
   AnsiString str;
   psl->Clear();
   unsigned int nTrackIndex;
   for (nTrackIndex = 0; nTrackIndex < SoundClass()->m_vTracks.size(); nTrackIndex++)
      str += IntToStr((int)SoundClass()->m_vTracks[nTrackIndex]->Underrun()) + ",";
   // remove last ','
   RemoveTrailingChar(str);
   SetValue(psl, SOUNDDLLPRO_PAR_VALUE, str);
}
//------------------------------------------------------------------------------

//------------------------------------------------------------------------------
/// returns and sets current global (audible) playback position in samples
//------------------------------------------------------------------------------
void PlayPosition(TStringList *psl)
{
   #ifndef NOALLOW_SETPOS
   int64_t nPlayPosition = (int)GetInt(psl, SOUNDDLLPRO_PAR_POSITION, -1);
   if (nPlayPosition >= 0)
	  SoundClass()->SetPosition((uint64_t)nPlayPosition);
   #endif
   psl->Clear();
   psl->Values[SOUNDDLLPRO_PAR_VALUE] = IntToStr((int64_t)SoundClass()->GetSamplePosition());
}
//------------------------------------------------------------------------------

//------------------------------------------------------------------------------
/// returns current global loading position in samples
//------------------------------------------------------------------------------
void LoadPosition(TStringList *psl)
{
   psl->Clear();
   psl->Values[SOUNDDLLPRO_PAR_VALUE] = IntToStr((int64_t)SoundClass()->GetLoadPosition());
}
//------------------------------------------------------------------------------

//------------------------------------------------------------------------------
/// returns 'recposition' of all input channels, i.e. how many samples were recorded
/// to file (!) up to now
//------------------------------------------------------------------------------
void RecPosition(TStringList *psl)
{
   psl->Clear();
   AnsiString str;
   unsigned int nChannelIndex;
   for (nChannelIndex = 0; nChannelIndex < SoundClass()->SoundActiveChannels(Asio::INPUT); nChannelIndex++)
      str += IntToStr(SoundClass()->m_vInput[nChannelIndex]->RecPosition()) + ",";
   // remove last ','
   RemoveTrailingChar(str);
   SetValue(psl, SOUNDDLLPRO_PAR_VALUE, str);
}
//------------------------------------------------------------------------------

//------------------------------------------------------------------------------
/// retrieves a buffer of recorded data from one or more input channnels
//------------------------------------------------------------------------------
void RecGetData(TStringList *psl)
{
   // checks if license is at least DSP  (function name disguising functionality)
   std::vector<int> viChannel = SoundClass()->ConvertSoundChannelArgument(psl->Values[SOUNDDLLPRO_PAR_CHANNEL], CT_INPUT);
   // data pointer for receiving data passed. If empty only BUFSIZE and CHANNELS are returned
   // for caller to determine needed space
   NativeInt nDataPointer           = (NativeInt)GetInt(psl, SOUNDDLLPRO_PAR_DATA, 0, VAL_POS_OR_ZERO);

   psl->Clear();
   AnsiString str;
   std::valarray<float> *pvaf;
   // NOTE: viChannel cannot be empty here
   unsigned int nChannels = (unsigned int)viChannel.size();
   unsigned int nSize = 0;
   unsigned int nSizeTmp;
   int64_t nPosition = 0;
   EnterCriticalSection(&SoundClass()->m_csBufferDone);
   try
      {
      // first of all check that all requested channels have recorded same size of data
      for (unsigned int i = 0; i < nChannels; i++)
         {
         nSizeTmp = SoundClass()->m_vInput[(unsigned int)viChannel[i]]->BufferSize();
         if (i == 0)
            nSize = nSizeTmp;
         if (!nSizeTmp)
            throw Exception("record data for channel requested, where no data where recorded to memory");
         else if (nSizeTmp != nSize)
            throw Exception("record data for channels with different recbufsizes requested: data must be retrieved in single commands!");
         }
      // get current position
      nPosition = (int64_t)(SoundClass()->GetBufferDonePosition() / SoundClass()->GetRecDownSampleFactor());
      // copy data only if passed data pointer valid!
      if (!!nDataPointer)
         {
         float *lpd = (float*)nDataPointer;
         for (unsigned int i = 0; i < nChannels; i++)
            {
            pvaf = &SoundClass()->m_vInput[(unsigned int)viChannel[i]]->GetBuffer();
            CopyMemory(lpd, &((*pvaf)[0]), nSize*sizeof(float));
            lpd += nSize;
            }
         }
      }
   __finally
      {
      LeaveCriticalSection(&SoundClass()->m_csBufferDone);
      }
   // NOTE: since we have to return 'leading zeroes' in the beginning, the position has to be
   // calculated (may be negative in the beginning!)
   psl->Values[SOUNDDLLPRO_PAR_POSITION]  = IntToStr((int64_t)(nPosition-nSize));
   psl->Values[SOUNDDLLPRO_PAR_BUFSIZE]   = IntToStr((int)nSize);
   psl->Values[SOUNDDLLPRO_PAR_CHANNELS]  = IntToStr((int)nChannels);
   return;
}
//------------------------------------------------------------------------------

//------------------------------------------------------------------------------
/// sets and returns size if recording ringbuffer for one or more channels
//------------------------------------------------------------------------------
void RecBufferSize(TStringList *psl)
{
   std::vector<int> viChannel = SoundClass()->ConvertSoundChannelArgument(psl->Values[SOUNDDLLPRO_PAR_CHANNEL], CT_INPUT);
   // check if buffer size is to be set
   AnsiString str = psl->Values[SOUNDDLLPRO_PAR_VALUE];
   psl->Clear();
   if (!str.IsEmpty())
      {
      // not allowed if device is running
      if (SoundClass()->DeviceIsRunning())
         throw Exception("command not allowed for setting buffer sizes while device is running");
      std::vector<int> viBufferSize;
      int nValue;
      TStringList *pslTmp = new TStringList();
      try
         {
         pslTmp->Delimiter = ',';
         pslTmp->DelimitedText = str;
         for (int i = 0; i < pslTmp->Count; i++)
            {
            if (!TryStrToInt(pslTmp->Strings[i], nValue) || nValue < 0)
               throw Exception("invalid buffer size(s): must be integer >= 0");
            viBufferSize.push_back(nValue);
            }
         unsigned int nValues = (unsigned int)viBufferSize.size();
         // dim of new volume vector now must be 1 (all channels) or equal to dim of viChannel
         if (nValues != 1 && nValues != viChannel.size())
            throw Exception("buffer size value count matching error");
         // set new buffer sizes
         for (unsigned int i = 0; i < viChannel.size(); i++)
            {
            if (nValues == 1)
               SoundClass()->m_vInput[(unsigned int)viChannel[i]]->BufferSize((unsigned int)viBufferSize[0]);
            else
               SoundClass()->m_vInput[(unsigned int)viChannel[i]]->BufferSize((unsigned int)viBufferSize[i]);
            }
         }
      __finally
         {
         TRYDELETENULL(pslTmp);
         }
      }
   // write buffer sizes to return
   str = "";
   for (unsigned int i = 0; i < SoundClass()->SoundActiveChannels(Asio::INPUT); i++)
      {
      str += IntToStr((int)SoundClass()->m_vInput[(unsigned int)viChannel[i]]->BufferSize()) + ",";
      }
   // remove last ','
   RemoveTrailingChar(str);
   SetValue(psl, SOUNDDLLPRO_PAR_VALUE, str);
}
//------------------------------------------------------------------------------

//------------------------------------------------------------------------------
/// sets and returns recording threshold mode and value for one or more input
/// channel
//------------------------------------------------------------------------------
void RecThreshold(TStringList *psl)
{
   std::vector<int> viChannel = SoundClass()->ConvertSoundChannelArgument(psl->Values[SOUNDDLLPRO_PAR_CHANNEL], CT_INPUT);
   // check if buffer size is to be set
   AnsiString str = psl->Values[SOUNDDLLPRO_PAR_VALUE];
   // set default mode
   int nMode = SDA_THRSHLDMODE_OR;
   if (!str.IsEmpty())
      {
      if (!IsDouble(str))
         throw Exception("invalid threshold value (must be between 0 and 1)");
      float f = (float)StrToDouble(str);
      if (f < 0.0f || f >= 1.0f)
         throw Exception("invalid threshold value (must be between 0 and 1)");
      AnsiString strMode = psl->Values[SOUNDDLLPRO_PAR_MODE];
      if (strMode == "0")
         nMode = SDA_THRSHLDMODE_AND;
      else if (!strMode.IsEmpty() && strMode != "1")
         throw Exception("invalid threshold mode ('0' or '1')");
      EnterCriticalSection(&SoundClass()->m_csProcess);
      try
         {
         SoundClass()->SetRecordThreshold(viChannel, f, nMode);
         }
      __finally
         {
         LeaveCriticalSection(&SoundClass()->m_csProcess);
         }
      }
   psl->Clear();
   psl->Values[SOUNDDLLPRO_PAR_VALUE]  = DoubleToStr((double)SoundClass()->GetRecordThreshold(nMode));
   psl->Values[SOUNDDLLPRO_PAR_MODE]   = IntToStr(nMode);
}
//------------------------------------------------------------------------------

//------------------------------------------------------------------------------
/// Checks, if recording to file (!) was ever started 
//------------------------------------------------------------------------------
void RecStarted(TStringList *psl)
{
   AnsiString str;
   psl->Clear();
   unsigned int nChannelIndex;
   for (nChannelIndex = 0; nChannelIndex < SoundClass()->SoundActiveChannels(Asio::INPUT); nChannelIndex++)
      str += IntToStr((int)SoundClass()->m_vInput[nChannelIndex]->Started()) + ",";
   // remove last ','
   RemoveTrailingChar(str);
   SetValue(psl, SOUNDDLLPRO_PAR_VALUE, str);
}
//------------------------------------------------------------------------------

//------------------------------------------------------------------------------
/// sets recording length, i.e. after how many samples recording to file (!)
/// should be stopped
//------------------------------------------------------------------------------
void RecLength(TStringList *psl)
{
   std::vector<int> viChannel = SoundClass()->ConvertSoundChannelArgument(psl->Values[SOUNDDLLPRO_PAR_CHANNEL], CT_INPUT);
   // check if buffer size is to be set
   AnsiString str = psl->Values[SOUNDDLLPRO_PAR_VALUE];
   psl->Clear();
   if (!str.IsEmpty())
      {
      std::vector<int> viRecLength;
      int nValue;
      TStringList *pslTmp = new TStringList();
      try
         {
         pslTmp->Delimiter = ',';
         pslTmp->DelimitedText = str;
         for (int i = 0; i < pslTmp->Count; i++)
            {
            if (!TryStrToInt(pslTmp->Strings[i], nValue) || nValue < 0)
               throw Exception("invalid record length(s): must be integer >= 0");
            viRecLength.push_back(nValue);
            }
         unsigned int nValues = (unsigned int)viRecLength.size();
         // dim of new volume vector now must be 1 (all channels) or equal to dim of viChannel
         if (nValues != 1 && nValues != viChannel.size())
            throw Exception("buffer size value count matching error");
         // check, if any of the requested channels is currently recording to file in a separate loop
         for (unsigned int i = 0; i < viChannel.size(); i++)
            {
            if (SoundClass()->m_vInput[(unsigned int)viChannel[i]]->IsRecording())
               throw Exception("cannot set record length of a channel that is currently recording");
            }
         // set new lengths
         for (unsigned int i = 0; i < viChannel.size(); i++)
            {
            if (nValues == 1)
               SoundClass()->m_vInput[(unsigned int)viChannel[i]]->RecordLength((unsigned int)viRecLength[0]);
            else
               SoundClass()->m_vInput[(unsigned int)viChannel[i]]->RecordLength((unsigned int)viRecLength[i]);
            }
         }
      __finally
         {
         TRYDELETENULL(pslTmp);
         }
      }
   // write buffer sizes to return
   str = "";
   for (unsigned int i = 0; i < SoundClass()->SoundActiveChannels(Asio::INPUT); i++)
      {
      str += IntToStr((int)SoundClass()->m_vInput[(unsigned int)viChannel[i]]->RecordLength()) + ",";
      }
   // remove last ','
   RemoveTrailingChar(str);
   SetValue(psl, SOUNDDLLPRO_PAR_VALUE, str);
}
//------------------------------------------------------------------------------

//------------------------------------------------------------------------------
/// sets and returns names of recording filenames for one or more channels
//------------------------------------------------------------------------------
void RecFileName(TStringList *psl)
{
   std::vector<int> viChannel = SoundClass()->ConvertSoundChannelArgument(psl->Values[SOUNDDLLPRO_PAR_CHANNEL], CT_INPUT);
   AnsiString str = psl->Values[SOUNDDLLPRO_PAR_FILENAME];
   psl->Clear();
   // check if any filename is to be set
   if (!str.IsEmpty())
      {
      TStringList *pslTmp = new TStringList();
      try
         {
         pslTmp->Delimiter = ',';
         str = StringReplace(str, "'", "\"", TReplaceFlags() << rfReplaceAll);
         pslTmp->DelimitedText = str;

         // number of filenames must be identical to number of channels!
         if (pslTmp->Count != (int)viChannel.size())
            throw Exception("filename count matching error");
         // set new filenames. NOTE: this will fail here, if device is running and
         // this input channel is not paused currently!
         for (unsigned int i = 0; i < viChannel.size(); i++)
            SoundClass()->m_vInput[(unsigned int)viChannel[i]]->FileName(pslTmp->Strings[(int)i]);
         }
      __finally
         {
         TRYDELETENULL(pslTmp);
         }
      }
   // write current filenames to return
   str = "";
   for (unsigned int i = 0; i < SoundClass()->m_vInput.size(); i++)
      str += SoundClass()->m_vInput[i]->FileName() + ",";
   // remove last ','
   RemoveTrailingChar(str);
   SetValue(psl, SOUNDDLLPRO_PAR_VALUE, str);
}
//------------------------------------------------------------------------------

//------------------------------------------------------------------------------
/// sets and returns pause status for one or more recording channel (pauses
/// recording to file)
//------------------------------------------------------------------------------
void RecPause(TStringList *psl)
{
   std::vector<int> viChannel = SoundClass()->ConvertSoundChannelArgument(psl->Values[SOUNDDLLPRO_PAR_CHANNEL], CT_INPUT);
   AnsiString str = psl->Values[SOUNDDLLPRO_PAR_VALUE];
   bool bCloseRecFile = psl->Values[SOUNDDLLPRO_PAR_CLOSERECFILE] == "1";
   psl->Clear();
   // check if mode is to be set
   if (!str.IsEmpty())
      {
      std::vector<bool> vb;
      TStringList *pslTmp = new TStringList();
      try
         {
         int nValue;
         pslTmp->Delimiter = ',';
         pslTmp->DelimitedText = str;
         // check validity of all values before setting any value
         for (int i = 0; i < pslTmp->Count; i++)
            {
            if (!TryStrToInt(pslTmp->Strings[i], nValue))
               throw Exception("invalid value for parameter 'value' (must be 0 or 1)");
            switch (nValue)
               {
               case 0:  vb.push_back(false); break;
               case 1:  vb.push_back(true);  break;
               default: throw Exception("invalid value for parameter 'value' (must be 0 or 1)");
               }
            }
         unsigned int nValues = (unsigned int)vb.size();
         // dim of new volume vector now must be 1 (all channels) or equal to dim of viChannel
         if (nValues != 1 && nValues != viChannel.size())
            throw Exception("mode value count matching error");
         // set new modes
         for (unsigned int i = 0; i < viChannel.size(); i++)
            {
            if (nValues == 1)
               SoundClass()->m_vInput[(unsigned int)viChannel[i]]->Pause(vb[0], bCloseRecFile);
            else
               SoundClass()->m_vInput[(unsigned int)viChannel[i]]->Pause(vb[i], bCloseRecFile);
            }
         }
      __finally
         {
         TRYDELETENULL(pslTmp);
         }
      }
   // write current modes to return
   str = "";
   for (unsigned int i = 0; i < SoundClass()->m_vInput.size(); i++)
      str += IntToStr((int)SoundClass()->m_vInput[i]->Pause()) + ",";
   // remove last ','
   RemoveTrailingChar(str);
   SetValue(psl, SOUNDDLLPRO_PAR_VALUE, str);
}
//------------------------------------------------------------------------------

//------------------------------------------------------------------------------
/// sets and returns debug saving status of one ore more output channels
//------------------------------------------------------------------------------
void DebugSave(TStringList *psl)
{
   // not allowed if device is running
   if (SoundClass()->DeviceIsRunning())
      throw Exception("command not allowed while device is running");
   std::vector<int> viChannel = SoundClass()->ConvertSoundChannelArgument(psl->Values[SOUNDDLLPRO_PAR_CHANNEL], CT_OUTPUT);
   // check if mode is to be set
   AnsiString str = psl->Values[SOUNDDLLPRO_PAR_VALUE];
   psl->Clear();
   if (!str.IsEmpty())
      {
      std::vector<bool> vb;
      TStringList *pslTmp = new TStringList();
      try
         {
         int nValue;
         pslTmp->Delimiter = ',';
         pslTmp->DelimitedText = str;
         // check validity of all values before setting any value
         for (int i = 0; i < pslTmp->Count; i++)
            {
            if (!TryStrToInt(pslTmp->Strings[i], nValue))
               throw Exception("invalid value for parameter 'value' (must be 0 or 1)");
            switch (nValue)
               {
               case 0:  vb.push_back(false); break;
               case 1:  vb.push_back(true);  break;
               default: throw Exception("invalid value for parameter 'value' (must be 0 or 1)");
               }
            }
         unsigned int nValues = (unsigned int)vb.size();
         // dim of new volume vector now must be 1 (all channels) or equal to dim of viChannel
         if (nValues != 1 && nValues != viChannel.size())
            throw Exception("mode value count matching error");
         // set new modes
         for (unsigned int i = 0; i < viChannel.size(); i++)
            {
            if (nValues == 1)
               SoundClass()->m_vOutput[(unsigned int)viChannel[i]]->DebugSave(vb[0]);
            else
               SoundClass()->m_vOutput[(unsigned int)viChannel[i]]->DebugSave(vb[i]);
            }
         }
      __finally
         {
         TRYDELETENULL(pslTmp);
         }
      }
   // write current modes to return
   str = "";
   for (unsigned int i = 0; i < SoundClass()->m_vOutput.size(); i++)
      str += IntToStr((int)SoundClass()->m_vOutput[i]->DebugSave()) + ",";
   // remove last ','
   RemoveTrailingChar(str);
   SetValue(psl, SOUNDDLLPRO_PAR_VALUE, str);
}
//------------------------------------------------------------------------------

//------------------------------------------------------------------------------
/// sets and returns names of debugging or file2file-filenames for one or more
/// channels (called for both functions)
//------------------------------------------------------------------------------
void DebugFileName(TStringList *psl)
{
   std::vector<int> viChannel = SoundClass()->ConvertSoundChannelArgument(psl->Values[SOUNDDLLPRO_PAR_CHANNEL], CT_OUTPUT);
   AnsiString str = psl->Values[SOUNDDLLPRO_PAR_FILENAME];
   psl->Clear();
   // check if any filename is to be set
   if (!str.IsEmpty())
      {
      TStringList *pslTmp = new TStringList();
      try
         {
         pslTmp->Delimiter = ',';
         str = StringReplace(str, "'", "\"", TReplaceFlags() << rfReplaceAll);
         pslTmp->DelimitedText = str;

         // number of filenames must be identical to number of channels!
         if (pslTmp->Count != (int)viChannel.size())
            throw Exception("filename count matching error");
         // set new filenames. NOTE: this will fail here, if device is running and
         // this input channel is not paused currently!
         for (unsigned int i = 0; i < viChannel.size(); i++)
            SoundClass()->m_vOutput[(unsigned int)viChannel[i]]->DebugFileName(pslTmp->Strings[(int)i]);
         }
      __finally
         {
         TRYDELETENULL(pslTmp);
         }
      }
   // write current filenames to return
   str = "";
   for (unsigned int i = 0; i < SoundClass()->m_vInput.size(); i++)
      str += SoundClass()->m_vOutput[(unsigned int)viChannel[i]]->DebugFileName() + ",";
   // remove last ','
   RemoveTrailingChar(str);
   SetValue(psl, SOUNDDLLPRO_PAR_VALUE, str);
}
//------------------------------------------------------------------------------

//------------------------------------------------------------------------------
/// adds a new button for button marking
//------------------------------------------------------------------------------
void   SetButton(TStringList *psl)
{
   AnsiString strReturn;
   try
      {
      int nStartPos     = (int)GetInt(psl, SOUNDDLLPRO_PAR_STARTPOS, VAL_POS_OR_ZERO);
      int nLength       = (int)GetInt(psl, SOUNDDLLPRO_PAR_LENGTH, VAL_POS);
      if (!psl->Values[SOUNDDLLPRO_PAR_HANDLE].IsEmpty())
         {
         HWND nHandle      = (HWND)GetInt(psl, SOUNDDLLPRO_PAR_HANDLE, VAL_POS);
         SoundClass()->AddButton(nHandle, nStartPos, nLength);
         }
      else
         {
         if (psl->Values[SOUNDDLLPRO_PAR_NAME].IsEmpty())
            throw Exception("value for field '" SOUNDDLLPRO_PAR_NAME "' is empty!");
         RECT rc;
         rc.left     = (LONG)GetInt(psl, SOUNDDLLPRO_PAR_LEFT, 0, VAL_ALL, true);
         rc.top      = (LONG)GetInt(psl, SOUNDDLLPRO_PAR_TOP, 0, VAL_ALL, true);
         rc.right    = (LONG)GetInt(psl, SOUNDDLLPRO_PAR_WIDTH, 0, VAL_ALL, true);
         rc.bottom   = (LONG)GetInt(psl, SOUNDDLLPRO_PAR_HEIGHT, 0, VAL_ALL, true);
         SoundClass()->AddButton(rc, psl->Values[SOUNDDLLPRO_PAR_NAME], nStartPos, nLength);
         }
      }
   __finally
      {
      psl->Clear();
      }
}
//------------------------------------------------------------------------------

//------------------------------------------------------------------------------
/// sets current MATLAB plugin user data
//------------------------------------------------------------------------------
void   PluginSetData(TStringList *psl)
{
   if (!SoundClass()->m_pMPlugin)
      throw Exception("no plugin active");
   AnsiString str = psl->Values[SOUNDDLLPRO_PAR_MODE];
   bool bOutput;
   if (str.IsEmpty() || !strcmpi(str.c_str(), SOUNDDLLPRO_VAL_OUTPUT))
      bOutput = true;
   else if (!strcmpi(str.c_str(), SOUNDDLLPRO_VAL_INPUT))
      bOutput = false;
   else
      throw Exception("invalid 'mode'");
   unsigned int nValues       = (unsigned int)GetInt(psl, SOUNDDLLPRO_PAR_SAMPLES, VAL_POS);
   unsigned int nChannels     = (unsigned int)GetInt(psl, SOUNDDLLPRO_PAR_CHANNELS, VAL_POS);

   // check data pointer or name of shared memory respectively
   if (psl->Values[SOUNDDLLPRO_PAR_DATA].IsEmpty())
      throw Exception("value for parameter '" + AnsiString(SOUNDDLLPRO_PAR_DATA) + "' must not be empty");
   NativeInt nDataPointer = (NativeInt)GetInt(psl, SOUNDDLLPRO_PAR_DATA, VAL_NON_ZERO);
   psl->Clear();
   SoundClass()->m_pMPlugin->SetUserData((double*)nDataPointer, nChannels, nValues, bOutput);
}
//------------------------------------------------------------------------------

//------------------------------------------------------------------------------
/// returns current MATLAB plugin user data
//------------------------------------------------------------------------------
void   PluginGetData(TStringList *psl)
{
   if (!SoundClass()->m_pMPlugin)
      throw Exception("no plugin active");
   AnsiString str = psl->Values[SOUNDDLLPRO_PAR_MODE];
   bool bOutput;
   if (str.IsEmpty() || !strcmpi(str.c_str(), SOUNDDLLPRO_VAL_OUTPUT))
      bOutput = true;
   else if (!strcmpi(str.c_str(), SOUNDDLLPRO_VAL_INPUT))
      bOutput = false;
   else
      throw Exception("invalid 'mode'");
   // data pointer for receiving data passed. If empty only BUFSIZE and CHANNELS are returned
   // for caller to determine needed space
   NativeInt nDataPointer = (NativeInt)GetInt(psl, SOUNDDLLPRO_PAR_DATA, 0, VAL_POS_OR_ZERO);
   str = "";
   psl->Clear();
   vvf& rvvf = SoundClass()->m_pMPlugin->GetUserData(bOutput);
   unsigned int nChannels = (unsigned int)rvvf.size();
   // check for sizing errors first
   unsigned int nSize = 0;
   // first of all check that all reuqested channels have same size of data
   for (unsigned int i = 0; i < nChannels; i++)
      {
      if (i == 0)
         nSize = (unsigned int)rvvf[i].size();
      else if (rvvf[i].size() != nSize)
         throw Exception("internal user data sizing error");
      }
   if (!!nDataPointer)
      {
      float *lpd = (float*)nDataPointer;
      for (unsigned int i = 0; i < nChannels; i++)
         {
         CopyMemory(lpd, &rvvf[i][0], nSize*sizeof(float));
         lpd += nSize;
         }
      }
   psl->Values[SOUNDDLLPRO_PAR_BUFSIZE]   = IntToStr((int)nSize);
   psl->Values[SOUNDDLLPRO_PAR_CHANNELS]  = IntToStr((int)nChannels);
}
//------------------------------------------------------------------------------

//------------------------------------------------------------------------------
/// sets IO-Status of one input channel, i.e. a list of virtual tracks, where
/// the current recording data should be added to on the fly
//------------------------------------------------------------------------------
void IOStatus(TStringList *psl)
{
   unsigned int nIndex;
   // allow duplicates here: one input channel may be mapped to multiple tracks!
   std::vector<int> viInputChannel = SoundClass()->ConvertSoundChannelArgument(psl->Values[SOUNDDLLPRO_PAR_INPUT], CT_INPUT, CCA_ARGS_DUPALLOWED);
   unsigned int nInputChannels = (unsigned int)viInputChannel.size();
   unsigned int nInputChannel  = (unsigned int)viInputChannel[0];

   AnsiString str = psl->Values[SOUNDDLLPRO_PAR_TRACK];
   // check if status is to be set
   psl->Clear();
   if (!str.IsEmpty())
      {
      EnterCriticalSection(&SoundClass()->m_csProcess);
      try
         {
         // -1 specified for track?
         if (str == "-1")
            {
            // here we allow empty inputs: set it for ALL channels
            if (!nInputChannels)
               nInputChannels = (unsigned int)SoundClass()->m_vInput.size();
            // clear mapping
            for (nIndex = 0; nIndex < nInputChannels; nIndex++)
               SoundClass()->m_vviIOMapping[(unsigned int)viInputChannel[nIndex]].resize(0);
            }
         // set new mapping
         else
            {
            std::vector<int> viTrack = SoundClass()->ConvertSoundChannelArgument(str, CT_TRACK);
            unsigned int nTracks = (unsigned int)viTrack.size();
            if (nInputChannels > 1)
               {
               // if  more than one input specified, then number of inputs and tracks must be identical
               if (nInputChannels != nTracks)
                  throw Exception("number of tracks must be identical to number inputs if more than one input channel specified");
               // go through passed inputs. In first loop we have to resize all mappings
               // for inputs that are passed at least once to 0 to use push_back afterwards
               for (nIndex = 0; nIndex < nInputChannels; nIndex++)
                  SoundClass()->m_vviIOMapping[(unsigned int)viInputChannel[nIndex]].resize(0);
               // In second loop we push the new mapping values
               for (nIndex = 0; nIndex < nInputChannels; nIndex++)
                  SoundClass()->m_vviIOMapping[(unsigned int)viInputChannel[nIndex]].push_back(viTrack[nIndex]);
               }
            // one input: map input to all specified tracks
            else
               {
               /*
               SoundClass()->m_vviIOMapping[nInputChannel].resize(nTracks);
               for (nIndex = 0; nIndex < nTracks; nIndex++)
                  SoundClass()->m_vviIOMapping[nInputChannel][nIndex] = viTrack[nIndex];
               */
               SoundClass()->m_vviIOMapping[nInputChannel].resize(0);
               for (nIndex = 0; nIndex < nTracks; nIndex++)
                  SoundClass()->m_vviIOMapping[nInputChannel].push_back(viTrack[nIndex]);
               }
            }
         SoundClass()->m_pfrmTracks->UpdateTrackInfos();
         SoundClass()->m_pfrmMixer->UpdateChannelData(CT_TRACK);
         }
      __finally
         {
         LeaveCriticalSection(&SoundClass()->m_csProcess);
         }
      }
   // write current IO status to return. This is only possible if only one input
   // is specified
   if (nInputChannels == 1)
      {
      str = "";
      for (nIndex = 0; nIndex < SoundClass()->m_vviIOMapping[nInputChannel].size(); nIndex++)
         str += IntToStr(SoundClass()->m_vviIOMapping[nInputChannel][nIndex]) + ",";
      // remove last ','
      RemoveTrailingChar(str);
      SetValue(psl, SOUNDDLLPRO_PAR_TRACK, str);
      }
}
//------------------------------------------------------------------------------

//------------------------------------------------------------------------------
/// resets global asynchroneous error
//------------------------------------------------------------------------------
void ResetError(TStringList *psl)
{
   psl->Clear();
   SoundClass()->ResetError();
}
//------------------------------------------------------------------------------

//------------------------------------------------------------------------------
/// returns global asynchroneous error
//------------------------------------------------------------------------------
void AsyncError(TStringList *psl)
{
   psl->Clear();
   if (!!SoundClass())
      {
      AnsiString sError;
      if (SoundClass()->AsyncError(sError))
         {
         sError = StringReplace(sError, ",", " ", TReplaceFlags() << rfReplaceAll);;
         psl->Values[SOUNDDLLPRO_PAR_VALUE] = "1";
         psl->Values[SOUNDDLLPRO_PAR_ERRORTEXT] = AnsiQuotedStr(sError, '"');
         }
      else
         psl->Values[SOUNDDLLPRO_PAR_VALUE] = "0";
      }
   else
      psl->Values[SOUNDDLLPRO_PAR_VALUE] = "0";
}
//------------------------------------------------------------------------------

//------------------------------------------------------------------------------
/// queries properties of a VST plugin
//------------------------------------------------------------------------------
void   VSTQuery(TStringList *psl)
{
   // this lengthy class is implemented in SoundClass itself...
   SoundClass()->VSTQueryPlugin(psl);
}
//------------------------------------------------------------------------------

//------------------------------------------------------------------------------
/// loads a VST plugin
//------------------------------------------------------------------------------
void   VSTLoad(TStringList *psl)
{
   if (SoundClass()->DeviceIsRunning())
      throw Exception("command not allowed if device is running");
   TIniFile    *pIni    = NULL;
   TStringList *pslTmp  = NULL;
   try
      {
      // first check, if an optional config file is passed
      AnsiString strConfigFile = psl->Values[SOUNDDLLPRO_PAR_CONFIGFILE];
      if (!strConfigFile.IsEmpty())
         {
         if (!FileExists(strConfigFile))
            throw Exception("VST config file '" + strConfigFile + "' not found");
         pIni = new TIniFile(ExpandFileName(strConfigFile));
         pslTmp = new TStringList();
         // copy entries from section "Settings"
         pIni->ReadSectionValues(SOUNDDLLPRO_STR_Settings, pslTmp);
         // patch non-exiting entries to psl (i.e. psl wins)
         CopyNonExistingEntries(pslTmp, psl);
         }
      AnsiString strFileName = psl->Values[SOUNDDLLPRO_PAR_FILENAME];
      if (!FileExists(strFileName))
         throw Exception("VST plugin file '" + strFileName + "' not found");
      AnsiString strType = psl->Values[SOUNDDLLPRO_PAR_TYPE];
      if (strType.IsEmpty())
         strType = SOUNDDLLPRO_VAL_MASTER;
      TVSTHost *pHost = SoundClass()->VSTGetHost(strType);
      if (!pHost)
         throw Exception("VSTHost of requested type not active (no corresponding channels in use?)");

      unsigned int nPosition = (unsigned int)GetInt(psl, SOUNDDLLPRO_PAR_POSITION, 0, VAL_POS_OR_ZERO);
      std::vector<int> viInput   = ConvertChannelArgument(psl->Values[SOUNDDLLPRO_PAR_INPUT], (int)pHost->m_nChannels, CCA_ARGS_NEGALLOWED | CCA_ARGS_NEGDUPALLOWED);
      AnsiString strOutput = psl->Values[SOUNDDLLPRO_PAR_OUTPUT];
      if (strOutput.IsEmpty())
         strOutput = psl->Values[SOUNDDLLPRO_PAR_INPUT];
      std::vector<int> viOutput  = ConvertChannelArgument(strOutput, (int)pHost->m_nChannels);
      // check recursion parameters
      std::vector<int> viRecurseCh = ConvertChannelArgument(psl->Values[SOUNDDLLPRO_PAR_RECURSECHANNEL], (int)pHost->m_nChannels, CCA_ARGS_EMPTYONEMPTY);
      std::vector<int> viRecursePos = ConvertChannelArgument(psl->Values[SOUNDDLLPRO_PAR_RECURSEPOS], VSTHOST_NUM_PLUGINS, CCA_ARGS_NEGALLOWED | CCA_ARGS_EMPTYONEMPTY | CCA_ARGS_DUPALLOWED);
      unsigned int nRecursion = (unsigned int)viRecurseCh.size();
      if (nRecursion != viRecursePos.size())
         throw Exception("dimensions of 'recursechannel' and 'recursepos' must agree");
      std::vector<TVSTNode> vRecurse;
      for (unsigned int i = 0; i < nRecursion; i++)
         vRecurse.push_back(TVSTNode(viRecursePos[i], viRecurseCh[i]));

      TVSTHostPlugin* pPlugin = pHost->LoadPlugin(strFileName, viInput, viOutput, vRecurse, nPosition);
      // if a config file was passed, we have to call VSTSet as well!
      if (!strConfigFile.IsEmpty())
         VSTSet(psl);
      else
         {
         // set program if requested
         if (!psl->Values[SOUNDDLLPRO_PAR_PROGRAM].IsEmpty())
            pPlugin->SetProgram(psl->Values[SOUNDDLLPRO_PAR_PROGRAM]);
         // set programname if requested
         if (!psl->Values[SOUNDDLLPRO_PAR_PROGRAMNAME].IsEmpty())
            pPlugin->SetProgramName(psl->Values[SOUNDDLLPRO_PAR_PROGRAMNAME]);
         }
      psl->Clear();
      // finally write the really applied values
      psl->Values[SOUNDDLLPRO_PAR_TYPE]      = strType;
      psl->Values[SOUNDDLLPRO_PAR_INPUT]     = ConvertChannelVector(viInput);
      psl->Values[SOUNDDLLPRO_PAR_OUTPUT]    = ConvertChannelVector(viOutput);
      psl->Values[SOUNDDLLPRO_PAR_POSITION]  = IntToStr((int)nPosition);
      }
   __finally
      {
      TRYDELETENULL(pIni);
      TRYDELETENULL(pslTmp);
      }
}
//------------------------------------------------------------------------------

//------------------------------------------------------------------------------
/// unloads a VST plugin
//------------------------------------------------------------------------------
void   VSTUnload(TStringList *psl)
{
   if (SoundClass()->DeviceIsRunning())
      throw Exception("command not allowed if device is running");
   try
      {
      TVSTHost *pHost      = SoundClass()->VSTGetHost(psl->Values[SOUNDDLLPRO_PAR_TYPE]);
      if (!pHost)
         throw Exception("VSTHost of requested type not active (no corresponding channels in use?)");
      unsigned int nPosition        = (unsigned int)GetInt(psl, SOUNDDLLPRO_PAR_POSITION, 0, VAL_POS_OR_ZERO);
      // use first input to identify plugin
      unsigned int nChannel         = (unsigned int)ConvertChannelArgument(psl->Values[SOUNDDLLPRO_PAR_INPUT], (int)pHost->m_nChannels)[0];
      pHost->UnloadPlugin(nPosition, nChannel);
      }
   __finally
      {
      psl->Clear();
      }
}
//------------------------------------------------------------------------------

//------------------------------------------------------------------------------
/// sets a VST program and returns current program
//------------------------------------------------------------------------------
void   VSTProgram(TStringList *psl)
{
   TVSTHostPlugin* pPlugin = SoundClass()->VSTGetPlugin(  psl->Values[SOUNDDLLPRO_PAR_TYPE],
                                                         psl->Values[SOUNDDLLPRO_PAR_POSITION],
                                                         psl->Values[SOUNDDLLPRO_PAR_INPUT]);
   AnsiString strProgram = psl->Values[SOUNDDLLPRO_PAR_PROGRAM];
   bool bUserConfig = psl->Values[SOUNDDLLPRO_PAR_USERCONFIG] == "1";
   psl->Clear();
   if (bUserConfig)
      {
      if (pPlugin->SetUserConfig(strProgram))
         psl->Values[SOUNDDLLPRO_PAR_PROGRAM] = AnsiQuotedStr(strProgram, '"');
      }
   else
      {
      if (!strProgram.IsEmpty())
         pPlugin->SetProgram(strProgram);
      psl->Values[SOUNDDLLPRO_PAR_PROGRAM] = AnsiQuotedStr(pPlugin->GetProgramName(), '"');
      }
}
//------------------------------------------------------------------------------

//------------------------------------------------------------------------------
/// sets a VST program name (!)
//------------------------------------------------------------------------------
void   VSTProgramName(TStringList *psl)
{
   TVSTHostPlugin* pPlugin = SoundClass()->VSTGetPlugin(  psl->Values[SOUNDDLLPRO_PAR_TYPE],
                                                         psl->Values[SOUNDDLLPRO_PAR_POSITION],
                                                         psl->Values[SOUNDDLLPRO_PAR_INPUT]);
   AnsiString strProgramName = psl->Values[SOUNDDLLPRO_PAR_PROGRAMNAME];
   psl->Clear();
   if (!strProgramName.IsEmpty())
      pPlugin->SetProgramName(strProgramName);
   psl->Values[SOUNDDLLPRO_PAR_PROGRAM] = AnsiQuotedStr(pPlugin->GetProgramName(), '"');;
}
//------------------------------------------------------------------------------

//------------------------------------------------------------------------------
/// sets a VST parameter
//------------------------------------------------------------------------------
void   VSTParameter(TStringList *psl)
{
   TVSTHostPlugin* pPlugin = SoundClass()->VSTGetPlugin(  psl->Values[SOUNDDLLPRO_PAR_TYPE],
														 psl->Values[SOUNDDLLPRO_PAR_POSITION],
														 psl->Values[SOUNDDLLPRO_PAR_INPUT]);
   AnsiString strParams = psl->Values[SOUNDDLLPRO_PAR_PARAMETER];
   if (strParams.IsEmpty())
	  strParams = pPlugin->GetParameterNames();
   else
	  strParams = StringReplace(strParams, "'", "\"", TReplaceFlags() << rfReplaceAll);
   AnsiString strValues     = psl->Values[SOUNDDLLPRO_PAR_VALUE];
   psl->Clear();
   if (!strValues.IsEmpty())
	  pPlugin->SetParameters(strParams, strValues);
   strParams = Trim(StringReplace(strParams, "\"", "", TReplaceFlags() << rfReplaceAll));
   strParams = Trim(StringReplace(strParams, " ", "", TReplaceFlags() << rfReplaceAll));
   psl->Values[SOUNDDLLPRO_PAR_PARAMETER] = strParams;
   psl->Values[SOUNDDLLPRO_PAR_VALUE]     = pPlugin->GetParameterValues(strParams);
}
//------------------------------------------------------------------------------

//------------------------------------------------------------------------------
/// sets program and/or parameters using a config file
//------------------------------------------------------------------------------
void   VSTSet(TStringList *psl)
{
   TIniFile    *pIni    = NULL;
   TStringList *pslTmp  = NULL;
   try
      {
      // config file is mandatory!
      AnsiString strConfigFile = psl->Values[SOUNDDLLPRO_PAR_CONFIGFILE];
      if (strConfigFile.IsEmpty())
         throw Exception("parameter 'configfile' is empty!");
      if (!FileExists(strConfigFile))
         throw Exception("VST config file '" + strConfigFile + "' not found");
      pIni = new TIniFile(ExpandFileName(strConfigFile));
      pslTmp = new TStringList();
      // copy entries from section "Settings"
      pIni->ReadSectionValues(SOUNDDLLPRO_STR_Settings, pslTmp);
      // patch non-exiting entries to psl (i.e. psl wins)
      CopyNonExistingEntries(pslTmp, psl);
      // copy entries from section "Program"
      pIni->ReadSectionValues(SOUNDDLLPRO_STR_Program, pslTmp);
      // patch non-exiting entries to psl (i.e. psl wins)
      CopyNonExistingEntries(pslTmp, psl);
      TVSTHostPlugin* pPlugin = SoundClass()->VSTGetPlugin(  psl->Values[SOUNDDLLPRO_PAR_TYPE],
                                                            psl->Values[SOUNDDLLPRO_PAR_POSITION],
                                                            psl->Values[SOUNDDLLPRO_PAR_INPUT]);
      // set program if requested
      if (!psl->Values[SOUNDDLLPRO_PAR_PROGRAM].IsEmpty())
         pPlugin->SetProgram(psl->Values[SOUNDDLLPRO_PAR_PROGRAM]);
      // set programname if requested
      if (!psl->Values[SOUNDDLLPRO_PAR_PROGRAMNAME].IsEmpty())
         pPlugin->SetProgramName(psl->Values[SOUNDDLLPRO_PAR_PROGRAMNAME]);
      // set multiple values
      // copy entries from section "Parameter"
      pIni->ReadSectionValues(SOUNDDLLPRO_STR_Parameter, pslTmp);
      if (!Trim(pslTmp->Text).IsEmpty())
         {
         pslTmp->Delimiter = ',';
         pPlugin->SetParameters(pslTmp->DelimitedText);
         }
      psl->Clear();
      psl->Values[SOUNDDLLPRO_PAR_PROGRAM]   = pPlugin->GetPrograms();
      psl->Values[SOUNDDLLPRO_PAR_PARAMETER] = pPlugin->GetParameterNames();
      psl->Values[SOUNDDLLPRO_PAR_VALUE]     = pPlugin->GetParameterValues(AnsiString());
      }
   __finally
      {
      TRYDELETENULL(pIni);
      TRYDELETENULL(pslTmp);
      }
}
//------------------------------------------------------------------------------

//------------------------------------------------------------------------------
/// stores current configuration of a plugin in an INIfile
//------------------------------------------------------------------------------
void   VSTStore(TStringList *psl)
{
   TIniFile    *pIni    = NULL;
   TStringList *pslTmp  = NULL;
   try
      {
      // config file name is mandatory!
      AnsiString strConfigFile = psl->Values[SOUNDDLLPRO_PAR_CONFIGFILE];
      if (strConfigFile.IsEmpty())
         throw Exception("parameter 'configfile' is empty!");
      // delete it if exists
      DeleteFile(strConfigFile);
      // create new inifile
      pIni = new TIniFile(ExpandFileName(strConfigFile));
      pslTmp = new TStringList();
      pslTmp->Delimiter = ',';
      // NOTE: here we set defaults for type and position to have ot stored
      if (psl->Values[SOUNDDLLPRO_PAR_TYPE].IsEmpty())
         psl->Values[SOUNDDLLPRO_PAR_TYPE] = SOUNDDLLPRO_VAL_MASTER;
      if (psl->Values[SOUNDDLLPRO_PAR_POSITION].IsEmpty())
         psl->Values[SOUNDDLLPRO_PAR_POSITION] = "0";
      // get type (default 'master')
      TVSTHostPlugin* pPlugin = SoundClass()->VSTGetPlugin(  psl->Values[SOUNDDLLPRO_PAR_TYPE],
                                                            psl->Values[SOUNDDLLPRO_PAR_POSITION],
                                                            psl->Values[SOUNDDLLPRO_PAR_INPUT]);
      // ccreate input channel vector
      std::vector<int > vi;
      for (unsigned int i = 0; i < pPlugin->GetInputMapping().size(); i++)
         vi.push_back(pPlugin->GetInputMapping()[i].m_nChannel);
      // write values to section "Settings"
      pIni->WriteString(SOUNDDLLPRO_STR_Settings, SOUNDDLLPRO_PAR_FILENAME,   ExpandFileName(pPlugin->m_strLibName));
      pIni->WriteString(SOUNDDLLPRO_STR_Settings, SOUNDDLLPRO_PAR_TYPE,       psl->Values[SOUNDDLLPRO_PAR_TYPE]);
      pIni->WriteString(SOUNDDLLPRO_STR_Settings, SOUNDDLLPRO_PAR_INPUT,      ConvertChannelVector(vi));
      pIni->WriteString(SOUNDDLLPRO_STR_Settings, SOUNDDLLPRO_PAR_OUTPUT,     ConvertChannelVector(pPlugin->GetOutputMapping()));
      pIni->WriteString(SOUNDDLLPRO_STR_Settings, SOUNDDLLPRO_PAR_POSITION,   psl->Values[SOUNDDLLPRO_PAR_POSITION]);
      // write values to section "Program"
      pIni->WriteString(SOUNDDLLPRO_STR_Program, SOUNDDLLPRO_PAR_PROGRAM,     pPlugin->GetProgramName());
      // write values to section "Parameter"
      pslTmp->DelimitedText   = pPlugin->GetParameters(AnsiString());
      for (int i = 0; i < pslTmp->Count; i++)
         pIni->WriteString(SOUNDDLLPRO_STR_Parameter, pslTmp->Names[i], pslTmp->Values[pslTmp->Names[i]]);
      }
   __finally
      {
      psl->Clear();
      TRYDELETENULL(pIni);
      TRYDELETENULL(pslTmp);
      }
}
//------------------------------------------------------------------------------

//------------------------------------------------------------------------------
/// calls a VST plugins GUI editor
//------------------------------------------------------------------------------
void   VSTEdit(TStringList *psl)
{
   TVSTHostPlugin* pPlugin = SoundClass()->VSTGetPlugin(  psl->Values[SOUNDDLLPRO_PAR_TYPE],
                                                         psl->Values[SOUNDDLLPRO_PAR_POSITION],
                                                         psl->Values[SOUNDDLLPRO_PAR_INPUT]);
   psl->Clear();
   pPlugin->ShowEditor();
}
//------------------------------------------------------------------------------

//------------------------------------------------------------------------------
/// returns current and maximum DSP load
//------------------------------------------------------------------------------
void   DspLoad(TStringList *psl)
{
   psl->Clear();
   psl->Values[SOUNDDLLPRO_PAR_VALUE]     = IntToStr((int)floor(100.0*SoundClass()->m_pcProcess[PERF_COUNTER_DSP].DecayValue()/SoundClass()->SecondsPerBuffer()));
   psl->Values[SOUNDDLLPRO_PAR_MAXVALUE]  = IntToStr((int)floor(100.0*SoundClass()->m_pcProcess[PERF_COUNTER_DSP].MaxValue()/SoundClass()->SecondsPerBuffer()));
}
//------------------------------------------------------------------------------

//------------------------------------------------------------------------------
/// resets the DSPload maximum value
//------------------------------------------------------------------------------
#pragma argsused
void   DspLoadReset(TStringList *psl)
{
   SoundClass()->m_pcProcess[PERF_COUNTER_DSP].Reset();
}
//------------------------------------------------------------------------------

//------------------------------------------------------------------------------
/// Interface to ADM
//------------------------------------------------------------------------------
void   AsioDirectMonitoring(TStringList *psl)
{
   ASIOInputMonitor aim;
   try
      {
      aim.input  = (long)GetInt(psl, SOUNDDLLPRO_PAR_INPUT, 0, VAL_ALL, true);
      aim.output = (long)GetInt(psl, SOUNDDLLPRO_PAR_OUTPUT, 0, VAL_ALL, true);
      aim.gain   = (long)GetInt(psl, SOUNDDLLPRO_PAR_GAIN, 0x20000000, VAL_ALL, true);
      aim.pan    = (long)GetInt(psl, SOUNDDLLPRO_PAR_PAN, 0x7fffffffL/2, VAL_ALL, false);
      aim.state  = (ASIOBool)GetInt(psl, SOUNDDLLPRO_PAR_MODE, 1, VAL_ALL, true);
      }
   __finally
      {
      psl->Clear();
      }
   if (aim.input < -1 || aim.input >= (int)SoundClass()->SoundActiveChannels(Asio::INPUT))
      throw Exception("'input' out of range");
   if (aim.output < 0 || aim.output >= (int)SoundClass()->SoundActiveChannels(Asio::OUTPUT))
      throw Exception("'output' out of range");
   if (aim.gain < 0 || aim.gain > 0x7fffffffL)
      throw Exception("'gain' out of range");
   if (aim.pan < 0 || aim.pan > 0x7fffffffL)
      throw Exception("'pan' out of range");
//      if (aim.state < 0 || aim.state > 2)
//         throw Exception("invalid value for 'mode'");

   ASIOError ae = SoundClass()->SetInputMonitor(aim);
   switch (ae)
      {
      case ASE_SUCCESS:          break;
      case ASE_InvalidParameter: throw Exception("error setting ASIO input monitor (invalid parameter)");
      case ASE_NotPresent:       throw Exception("error setting ASIO input monitor (not present)");
      default:                   throw Exception("error setting ASIO input monitor (unknown error)");
      }
}
//------------------------------------------------------------------------------

//------------------------------------------------------------------------------
/// Initializes MIDI device
//------------------------------------------------------------------------------
void   MIDIInit(TStringList *psl)
{
   AnsiString strDriver = psl->Values[SOUNDDLLPRO_PAR_DRIVER];
   psl->Clear();
   if (strDriver.IsEmpty())
      strDriver = "0";
   unsigned int nIndex;  
   int i;
   if (!TryStrToInt(strDriver, i))
      {
      bool bFound = false;
      // search for it
      unsigned int nNum = TSimpleMidi::GetNumDevs();
      for (nIndex = 0; nIndex < nNum; nIndex++)
         {
         if (UpperCase(TSimpleMidi::GetDevName(nIndex)) == UpperCase(strDriver))
            {
            bFound = true;
            break;
            }
         }
      if (!bFound)
         throw Exception("MIDI driver '" + strDriver + "' not found in the system");
      }
   else  
      nIndex = (unsigned int)i;
   TRYDELETENULL(g_pMidi);
   g_pMidi = new TSimpleMidi(nIndex);
   psl->Values[SOUNDDLLPRO_PAR_DRIVER] = g_pMidi->GetDevName(nIndex);
}
//------------------------------------------------------------------------------

//------------------------------------------------------------------------------
/// exits MIDI device
//------------------------------------------------------------------------------
void   MIDIExit(TStringList *psl)
{
   TRYDELETENULL(g_pMidi);
   psl->Clear();
}
//------------------------------------------------------------------------------

//------------------------------------------------------------------------------
/// returns all available MIDI devices
//------------------------------------------------------------------------------
void   MIDIGetDrivers(TStringList *psl)
{
   psl->Clear();
   // go through drivers...
   UINT nNum = TSimpleMidi::GetNumDevs();
   UINT n;
   AnsiString str;
   for (n = 0; n < nNum; n++)
      str += AnsiQuotedStr(TSimpleMidi::GetDevName(n), '"') + ",";
   // remove last ','
   RemoveTrailingChar(str);
   psl->Values[SOUNDDLLPRO_PAR_DRIVER] = str;
}
//------------------------------------------------------------------------------

//------------------------------------------------------------------------------
/// plays a note on MIDI device
//------------------------------------------------------------------------------
void   MIDIPlayNote(TStringList *psl)
{
   try
      {
      if (!g_pMidi)
         throw Exception("MIDI not initialized");
      int64_t nNote     = GetInt(psl, SOUNDDLLPRO_PAR_NOTE, 0, VAL_ALL, true);
      int64_t nVolume   = GetInt(psl, SOUNDDLLPRO_PAR_VOLUME, 127, VAL_ALL);
      int64_t nChannel  = GetInt(psl, SOUNDDLLPRO_PAR_CHANNEL, 0, VAL_ALL);

      if (nNote < 0 || nNote > 127)
         throw Exception("note must be between 0 and 127");
      if (nVolume < 0 || nVolume > 127)
         throw Exception("volume must be between 0 and 127");
      if (nChannel < 0 || nChannel > 15)
         throw Exception("channel must be between 0 and 15");
      g_pMidi->PlayNote((BYTE)nNote, (BYTE)nVolume, (BYTE)nChannel);
      }
   __finally
      {
      psl->Clear();
      }
}
//------------------------------------------------------------------------------

//------------------------------------------------------------------------------
/// sends a OutShrtMsg to MIDI device
//------------------------------------------------------------------------------
void   MIDIShortMsg(TStringList *psl)
{
   try
      {
      if (!g_pMidi)
         throw Exception("MIDI not initialized");
      int64_t nStatus   = GetInt(psl, SOUNDDLLPRO_PAR_STATUS, 0, VAL_ALL, true);
      int64_t nMidi1    = GetInt(psl, SOUNDDLLPRO_PAR_MIDI1, 0, VAL_ALL, true);
      int64_t nMidi2    = GetInt(psl, SOUNDDLLPRO_PAR_MIDI2, 0, VAL_ALL, true);
      if (nStatus < 0 || nStatus > 255)
         throw Exception("status must be between 0 and 255");
      if (nMidi1 < 0 || nMidi1 > 255)
         throw Exception("midi1 must be between 0 and 255");
      if (nMidi2 < 0 || nMidi2 > 255)
         throw Exception("midi2 must be between 0 and 255");
      g_pMidi->OutShortMsg((BYTE)nStatus, (BYTE)nMidi1, (BYTE)nMidi2);
      }
   __finally
      {
      psl->Clear();
      }
}
//------------------------------------------------------------------------------

//------------------------------------------------------------------------------
/// Command for beta tests. Accepts all parameters magically
//------------------------------------------------------------------------------
void   BetaTest(TStringList *psl)
{
   try
      {
      AnsiString strTestCommand = psl->Values["testcommand"];
      if (!strcmpi(strTestCommand.c_str(), "gethann"))
         {
         unsigned int len = (unsigned int)GetInt(psl, "len", 0, VAL_POS_OR_ZERO, true);
         unsigned int pos = (unsigned int)GetInt(psl, "pos", 0, VAL_POS_OR_ZERO, true);
         float f = GetHanningRamp(pos, len, true);
         psl->Clear();
         psl->Values["value"] = DoubleToStr((double)f);
         }
      else if (!strcmpi(strTestCommand.c_str(), "ramps"))
         {
         g_bUseRamps = GetInt(psl, "value", 0, VAL_ALL, true) != 0;
         psl->Clear();
         }
      else
         throw Exception("unknown 'testcommand' value");
      }
   __finally
      {
      }
}
//------------------------------------------------------------------------------

