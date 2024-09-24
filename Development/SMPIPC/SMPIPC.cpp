//------------------------------------------------------------------------------
/// \file SMPIPC.cpp
/// \author Berg
/// \brief Implementation Loader of SoundDllPro for usage with Inter-Process-
/// Communication using shared memory (memory mapped files) and events
///
/// NOTE: this application links vs. static import library SoundDllProe.lib
///
/// Project SoundMexPro
/// Module  SMPIPC.exe
///
/// ****************************************************************************
/// Copyright 2023 Daniel Berg
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
#include <vcl.h>
#include <Vcl.Styles.hpp>
#include <Vcl.Themes.hpp>
#include <stdio.h>
#include <dir.h>
#include <string>
#pragma hdrstop
#include "soundmexpro_defs.h"
#include "soundmexpro.h"
#include "soundmexpro_ipc.h"
#include "SoundDllPro_Interface.h"
#pragma warn -aus
//---------------------------------------------------------------------------

//#define SOUND_MODULE         "SOUNDDLLPRO.DLL"     ///< name of DLL to load
static char  lpszReturn[CMDBUFSIZE];               ///< buffer fpr return values
// local prototypes
int   SMPCommand(const char* lpcszCommand);
bool  DoDebug(void);
void  WriteLog(AnsiString str);
//------------------------------------------------------------------------------

//------------------------------------------------------------------------------
/// Winmain. Creates shared memory, shared events and waits in an endless loop
/// for more commands to follow
//------------------------------------------------------------------------------
#ifdef __clang__
int WINAPI _tWinMain(HINSTANCE, HINSTANCE, LPTSTR, int)
#else
WINAPI WinMain(HINSTANCE, HINSTANCE, LPSTR, int)
#endif
{

   MapFile                 mfCmd;
   HMODULE                 hSoundDllHandle = NULL;
   LPFNSOUNDDLLPROCOMMAND  lpfnSoundDllProCommand = NULL;
   bool bDebug = DoDebug();

   try
   {
   Application->Title = "SoundMexPro";
   HANDLE      hIPCEvent[SMP_IPC_EVENT_LAST] = {0};
   try
      {
      try
         {
         if (bDebug)
            {
            DeleteFile(ChangeFileExt(Application->ExeName, ".log"));
            WriteLog("START");
            }
         if (ParamCount() == 0)
            throw Exception("GUID missing as command line parameter");

         if (bDebug)
            WriteLog("Opening IPC events");
         char c[255];
         // 1. create named events non-signaled and auto-resetting
         for (int i = 0; i < SMP_IPC_EVENT_LAST; i++)
            {
            sprintf(c, "%s%d",  AnsiString(ParamStr(1)).c_str(), i);
            hIPCEvent[i]    = OpenEvent(EVENT_ALL_ACCESS, false, c);
            if (!hIPCEvent[i])
               throw Exception("error creating IPC events");
            }
         if (bDebug)
            WriteLog("Accessing shared memory");
         // 2. get access to memory mapped file (do this ASAP to write error messages to it)!
         SMPAccessFileMapping(mfCmd, AnsiString(ParamStr(1)).c_str());
         // from here we print progress to shared memory to check on timeout, at which step
         // the problem ocurred
         #define PRINT_PROGRESS(p)  \
            if (bDebug) \
               WriteLog(p); \
            sprintf(mfCmd.pData, "startup step '%s'", p);
         PRINT_PROGRESS("setting ready event");
         // now set done event: startup is done
         if (!SetEvent(hIPCEvent[SMP_IPC_EVENT_DONE]))
            throw Exception("error setting done event");
         // do an endless loop were we wait for a command to be passed to SoundDllPro
         // or 'end' flag
         bool bContinue = true;
         int iReturn = SOUNDDLL_RETURN_ERROR;
         if (bDebug)
            WriteLog("STARTUP COMPLETE");
         while (bContinue)
            {
            // wait for mutex signals (only for process or exit: first two handles are CMD and EXIT),
            // AND for messages to keep message loop alive
            DWORD nWaitResult = MsgWaitForMultipleObjects(2, hIPCEvent, false, INFINITE, QS_ALLINPUT);
            switch (nWaitResult)
               {
               case (WAIT_OBJECT_0 + SMP_IPC_EVENT_CMD):
                  ZeroMemory(lpszReturn, CMDBUFSIZE);
                  iReturn = SMPCommand(mfCmd.pData);
                  //OutputDebugString(mfCmd.pData);
                  sprintf(mfCmd.pData, "%s", lpszReturn);
                  // set 'done' event
                  if (!SetEvent(hIPCEvent[iReturn == SOUNDDLL_RETURN_OK ? SMP_IPC_EVENT_DONE : SMP_IPC_EVENT_ERROR]))
                     throw ("error setting done/error event");
                  break;
               // exit: break the loop
               case (WAIT_OBJECT_0 + SMP_IPC_EVENT_EXIT):
                  if (bDebug)
                     WriteLog("EXIT RECEIVED");
                  bContinue = false;
                  break;
               // default (messages): process them!
               default: Application->ProcessMessages();
               }
            }
         }
	  catch(Exception &e)
         {
         if (bDebug)
            WriteLog(e.Message);
         // if shared memory is available write error to it and set error event
         if (mfCmd.pData)
            {
            sprintf(mfCmd.pData, "SOUNDDLLPRO_CMD_ERROR=error in SMPIPC (%s)", AnsiString(e.Message).c_str());
            if (!!hIPCEvent[SMP_IPC_EVENT_ERROR])
               SetEvent(hIPCEvent[SMP_IPC_EVENT_ERROR]);
            }
         // otherwise show a message box and signal error afterwards
         else
            {
            MessageBoxW(0, e.Message.c_str(), L"Error in SMPIPC", MB_ICONERROR);
            if (!!hIPCEvent[SMP_IPC_EVENT_ERROR])
               SetEvent(hIPCEvent[SMP_IPC_EVENT_ERROR]);
            }
         }
      }
   __finally
      {
      if (bDebug)
         WriteLog("cleaning up IPC events");
      // cleanup IPC events
      for (int i = 0; i < SMP_IPC_EVENT_LAST; i++)
         {
      if (hIPCEvent[i] != NULL)
            {
            CloseHandle(hIPCEvent[i]);
            hIPCEvent[i] = NULL;
            }
         }
      try
         {
         if (bDebug)
            WriteLog("exiting SMP");
         SoundDllProCommand("command=exit", lpszReturn, CMDBUFSIZE);
         }
      catch (...)
         {
         }
      }
   }
   catch (Exception &exception)
	   {
	  OutputDebugString("ERR 1");
	  Application->ShowException(&exception);
	   }
   catch (...)
      {
      try
         {
        throw Exception("unknown exception occurred in IPC process");
         }
      catch (Exception &exception)
         {
         OutputDebugString("ERR 2");
         Application->ShowException(&exception);
         }
      }
   if (bDebug)
      WriteLog("releasing shared memory");
   SMPReleaseFileMapping(mfCmd);
   if (bDebug)
      WriteLog("EXIT DONE");
   return 0;
}
//---------------------------------------------------------------------------

//---------------------------------------------------------------------------
// calls DLL command and translates in/out arguments from/to pointer(/memory mapped
// file for IPC
//---------------------------------------------------------------------------
int  SMPCommand(const char* lpcszCommand)
{
   int nReturn = 0;
   MapFile mf;
   try
      {
      std::string str = lpcszCommand;
      std::vector<std::string> vs;
      ParseValues(str, vs, ';');
      std::string strValue = GetValue(vs, SOUNDDLLPRO_PAR_DATA);
      if (strlen(strValue.c_str()))
         {
         SMPAccessFileMapping(mf, strValue.c_str());
         SetValue(vs, SOUNDDLLPRO_PAR_DATA, AnsiString(IntToStr((NativeInt)mf.pData)).c_str());
         unsigned int n;
         str.clear();
         for (n = 0; n < vs.size(); n++)
            {
            str += vs[n] + ";";
            }
         }
      nReturn = SoundDllProCommand(str.c_str(), lpszReturn, CMDBUFSIZE);
      }
   __finally
      {
      SMPReleaseFileMapping(mf);
      }
   return nReturn;
}
//------------------------------------------------------------------------------

//------------------------------------------------------------------------------
/// reads debug-flag from inifile
//------------------------------------------------------------------------------
bool DoDebug(void)
{
   char c[MAX_PATH];
   ZeroMemory(c, MAX_PATH);
   GetPrivateProfileString("Debug",
                           "Log",
                           "0",
                           c,
                           MAX_PATH,
                           AnsiString(ChangeFileExt(Application->ExeName, ".ini")).c_str()
                           );
   return AnsiString(c) == "1";
}
//------------------------------------------------------------------------------

//------------------------------------------------------------------------------
/// writes passed string to logfile (debugging only)
//------------------------------------------------------------------------------
void WriteLog(AnsiString str)
{
   FILE *file = fopen(AnsiString(ChangeFileExt(Application->ExeName, ".log")).c_str(), "a");
   try
      {
      if (file)
         fprintf(file, "%s\n", str.c_str());
      }
   __finally
      {
      if (file)
         fclose(file);
      }
}
//------------------------------------------------------------------------------

