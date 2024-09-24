//------------------------------------------------------------------------------
/// \file SoundMexProPyMain.cpp
/// \author Berg
/// \brief main file for SoundMexProPy32.dll and SoundMexProPy64.dll
///
/// Project SoundMexPro
/// Modules  SoundMexProPy32.dll and SoundMexProPy64.dll
///
/// SoundMexProPyMain.cpp is the interface between Python and
/// the 'main' SoundMexPro DLL SoundDllPro.dll. The communication between this
/// Dll and SoundMexPro is done through interprocess communication to allow
/// usage from 32bit AND 64bit processes, and to separate the calling process from
/// SoundMexPro.
/// For this purpose a 32bit interprocess communication process executable named
/// SMPIPC.EXE is started. This EXE loads the main dll SoundDllPro.dll and offers
/// interprocess communication through named events and memory mapped files.
/// This parser passes the incoming command from Python in syntax expected
/// by SoundDllPro to the shared memory and signals through an event, that
/// SMPIPC.EXE may read it. Then it waits for a signal to read return values and/or
/// error messages from shared memory again.
/// Additionally it copies data to the shared memory, if matrices are passed +
/// sizing information (number of rows and columns)
/// Finally it passes returned values back to Python.
///
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
#include <windows.h>
#include <winbase.h>
#include <vector>
#include <valarray>

#include "soundmexpro.h"

#include "soundmexpro_defs.h"
#include "soundmexpro_ipc.h"
#pragma hdrstop
#pragma argsused

// subversion of soundmexpro displayed as fourth number ("build") in version comand
#define SOUNDMEX_SUBVER      0

// define function from soundmexprotools.cpp which is hidden in soundmexpro.h
// through define NOMEX
bool        IsDllCommand(std::string strCommand, std::string strHelp);

// global variables
static int            g_nShowError   = 1;  ///< current error showing behaviour flag
static std::string    g_strLastError = "no error";  ///< string holding last error
static SMPIPCProcess  g_SMPIPC;                     ///< instance of IPC class


// local prototypes
void           ExitFcn(void);

//------------------------------------------------------------------------------
/// WinMain. Calls ExitFcn if DLL is detached
//------------------------------------------------------------------------------
#pragma argsused
int WINAPI DllEntryPoint(HINSTANCE hinst, unsigned long reason, void* lpReserved)
{
   switch (reason)
      {
      case DLL_PROCESS_DETACH:   // printf("unloading SoundMexProPy\n");
                                 ExitFcn();
                                 break;
      }
   return 1;
}
//------------------------------------------------------------------------------

//------------------------------------------------------------------------------
// calls "exit" in SoundDllPro.dll and exits IPC
//------------------------------------------------------------------------------
void ExitFcn(void)
{
   if (!!g_SMPIPC.m_mfCmd.pData && !!g_SMPIPC.m_hIPCEvent[SMP_IPC_EVENT_CMD])
      {
      snprintf(g_SMPIPC.m_mfCmd.pData, CMDBUFSIZE, "command=exit");
      if (!SetEvent(g_SMPIPC.m_hIPCEvent[SMP_IPC_EVENT_CMD]))
         throw ("error setting smc event");

      // wait for mutex signals (only for done and error)
      WaitForMultipleObjects(2, &g_SMPIPC.m_hIPCEvent[SMP_IPC_EVENT_DONE], false, 5000);
      }

   g_SMPIPC.IPCExit();
}
//------------------------------------------------------------------------------




extern "C" {
__declspec(dllexport) int cdecl SoundMexPro(const char* lpcszCommand, char* lpcszReturn, int nRetLen);
//------------------------------------------------------------------------------
/// Main exported function
/// \param[in]  lpcszCommand string woth command + arguments
/// \param[out] lpcszReturn buffer to receive return values
/// \param[in]  nRetLen length of lpcszReturn including trailing zero
/// \return sucess/failure flag (1 on success, < 1 else)
//------------------------------------------------------------------------------
__declspec(dllexport) int cdecl SoundMexPro(const char* lpcszCommand, char* lpcszReturn, int nRetLen)
{

   // empty command? print complete help
   if (strlen(lpcszCommand) == 0)
      {
      printf("\n---------------------------------------------------------------------------------------\n");
      printf(SOUNDMEX_HELP);
      printf("\n---------------------------------------------------------------------------------------\n");
      return SOUNDDLL_RETURN_OK;
      }

   // Mapfile object for passing double values from Python to sounddllpro
   MapFile     mfData;
   std::string strError;
   std::string strHelpCommand;

   static int nCommandCounter = 0; ///< count how many commands are currently running
   int        iReturn         = SOUNDDLL_RETURN_ERROR;   ///< return value. Default: error

   std::string strCmd;
   std::string strCommand;
   std::vector<std::string>   vsCmd;   ///< vector of strings containing input arguments
   std::vector<std::string>   vsRet;   ///< vector of strings receiving return values


   int64_t nDataDestPy = 0; ///< pointer to memory, where to write binary values ti be returned
   int64_t nSamplesPy  = 0;  ///< number of audio samples in binary buffer
   int64_t nChannelsPy = 0; ///< number of audio channels in binary buffer


   try
      {
      // parse command line into vector of strings
      ParseValues(std::string(lpcszCommand), vsCmd);

      strCommand = GetValue(vsCmd, SOUNDDLLPRO_STR_COMMAND);
      if ( strCommand.length() == 0 )
         throw SOUNDMEX_Error("Can't read Command name");

      if (  0 == _strcmpi(strCommand.c_str(), SOUNDDLLPRO_CMD_SETBUTTON)
         || 0 == _strcmpi(strCommand.c_str(), SOUNDDLLPRO_CMD_PLUGINSETDATA)
         || 0 == _strcmpi(strCommand.c_str(), SOUNDDLLPRO_CMD_PLUGINGETDATA)
         )
         throw SOUNDMEX_Error("setbutton and plugin commands not supported in Python");

      if (  0== _strcmpi(strCommand.c_str(), SOUNDDLLPRO_CMD_INIT)
         && GetValue(vsCmd, SOUNDDLLPRO_PAR_PLUGIN_PROC) != ""
         )
         throw SOUNDMEX_Error("script plugins not supported in Python");

      nCommandCounter++;
      if (nCommandCounter>1)
         throw SOUNDMEX_Error("Error in command " + strCommand + ": sounddllpro busy: asynchroneous command call failed!", SOUNDDLL_RETURN_BUSY);

      bool bQuietInit   = false;

      strCmd = lpcszCommand;
      //-----------------------------------------------------------------
      // special handling for help: append magic argument 'soundmexcall=1'
      //-----------------------------------------------------------------
      if (  0 == _strcmpi(strCommand.c_str(), SOUNDDLLPRO_CMD_HELP)
         || 0 == _strcmpi(strCommand.c_str(), SOUNDDLLPRO_CMD_HELPA)
         )
         {
         strCmd = strCmd + ";" + SOUNDDLLPRO_PAR_SOUNDMEXCALL + "=1;";
         }
      //-----------------------------------------------------------------
      // all other commands
      //-----------------------------------------------------------------
      else
         {
         double   dValue, dDummy;
         bool     bDataField        = false;
         std::string strName;

         bQuietInit   = GetValue(vsCmd, SOUNDDLLPRO_PAR_QUIET) == "1";
         bDataField   = GetValue(vsCmd, SOUNDDLLPRO_PAR_DATA).length() > 0;
         // the 'data' field has to be handled separately: we have to copy the
         // data to the shared memory!
         if (bDataField)
            {
            // then fields "samples" and "channels" are mandatory
            int64_t nData, nSamples, nChannels;
            if (!TryStrToInteger(GetValue(vsCmd, SOUNDDLLPRO_PAR_DATA), nData))
               throw SOUNDMEX_Error("paramater 'data' must be an int when passing 'data'");
            if (!TryStrToInteger(GetValue(vsCmd, SOUNDDLLPRO_PAR_SAMPLES), nSamples))
               throw SOUNDMEX_Error("paramater 'samples' must be an int when passing 'data'");
            if (!TryStrToInteger(GetValue(vsCmd, SOUNDDLLPRO_PAR_CHANNELS), nChannels))
               throw SOUNDMEX_Error("paramater 'channels' must be an int when passing 'data'");

            std::string strValue = CreateGUID();
            DWORD dwSize = (DWORD)(nSamples*nChannels*(int64_t)sizeof(double));
            // create temporary shared memory
            SMPCreateFileMapping(mfData, strValue.c_str(), dwSize);
            // copy data to it
            CopyMemory(mfData.pData, (void*)nData, dwSize);

            // replace content of 'data' field to contain the name of the memory
            // mapped file
            RemoveValue(vsCmd, SOUNDDLLPRO_PAR_DATA);
            SetValue(vsCmd, SOUNDDLLPRO_PAR_DATA, strValue.c_str());
            }
         }

      // re-create command from vsCmd
      strCmd = "";
      unsigned int i;
      for (i = 0; i < vsCmd.size(); i++)
         strCmd = strCmd + vsCmd[i] + ";";

      if (!g_SMPIPC.IPCProcessActive())
         g_SMPIPC.IPCInit();


      //--------------------------------------------------------------------------------
      // call external command
      //--------------------------------------------------------------------------------
      // all regular DLL-commands
      if (IsDllCommand(strCommand, strHelpCommand))
         {
         iReturn = SOUNDDLL_RETURN_OK;

         // special: extra calls for recgetdata and plugingetdata to retrieve
         // needed buffer size of memory mapped file to be created
         if (  0==_strcmpi(strCommand.c_str(), SOUNDDLLPRO_CMD_RECGETDATA)
            || 0==_strcmpi(strCommand.c_str(), SOUNDDLLPRO_CMD_PLUGINGETDATA)
            )
            {
            // get required input data
            if (!TryStrToInteger(GetValue(vsCmd, SOUNDDLLPRO_PAR_DATADEST), nDataDestPy))
               throw SOUNDMEX_Error("paramater 'datadest' must be an int when retrieving data");
            if (!TryStrToInteger(GetValue(vsCmd, SOUNDDLLPRO_PAR_SAMPLES), nSamplesPy))
               throw SOUNDMEX_Error("paramater 'samples' must be an int when retrieving data");
            if (!TryStrToInteger(GetValue(vsCmd, SOUNDDLLPRO_PAR_CHANNELS), nChannelsPy))
               throw SOUNDMEX_Error("paramater 'channels' must be an int when retrieving data");
            // then remove these fields from command
            RemoveValue(vsCmd, SOUNDDLLPRO_PAR_DATADEST);
            RemoveValue(vsCmd, SOUNDDLLPRO_PAR_SAMPLES);
            RemoveValue(vsCmd, SOUNDDLLPRO_PAR_CHANNELS);

            // currently the 'data' argument is empty. This will cause SoundDllPro
            // to return BUFSIZE and CHANNELS for calculating needed size
            snprintf(g_SMPIPC.m_mfCmd.pData, CMDBUFSIZE, "%s", strCmd.c_str());
            if (!SetEvent(g_SMPIPC.m_hIPCEvent[SMP_IPC_EVENT_CMD]))
               throw SOUNDMEX_Error("error setting CMD event");
            // NOTE: this is a copy of the 'regular' command call below
            DWORD nWaitResult;
            bool bCommandDone = false;
            while (!bCommandDone)
               {
               // wait for mutex signals (only for done and error). With a timeout we
               // check, if the IPC  process is still alive
               nWaitResult = WaitForMultipleObjects(2, &g_SMPIPC.m_hIPCEvent[SMP_IPC_EVENT_DONE], false, 1000);
               switch (nWaitResult)
                  {
                  // first is done
                  case (WAIT_OBJECT_0):
                     iReturn = SOUNDDLL_RETURN_OK;
                     bCommandDone = true;
                     break;
                  // timeout: only check, if IPC process still running
                  case (WAIT_TIMEOUT):
                     if (!g_SMPIPC.IPCProcessActive())
                        {
                        g_SMPIPC.IPCExit();
                        throw SOUNDMEX_Error("IPC process handle invalid");
                        }
                     break;
                  // all others (abandoned, exit, error): break. We do not handle errors here
                  default:
                     iReturn = SOUNDDLL_RETURN_ERROR;
                     bCommandDone = true;
                     break;
                  }
               }
            // if it succeeded then create memory mapped file
            if (iReturn == SOUNDDLL_RETURN_OK)
               {
               ParseValues(std::string(g_SMPIPC.m_mfCmd.pData), vsRet);

               // now we MUST have Samples and Channels available!
               int64_t nBufSize, nChannels = 0;
               if (  !TryStrToInteger(GetValue(vsRet, SOUNDDLLPRO_PAR_BUFSIZE), nBufSize)
                  || !TryStrToInteger(GetValue(vsRet, SOUNDDLLPRO_PAR_CHANNELS), nChannels)
                  )
                  throw SOUNDMEX_Error("invalid sizes retrieved from SoundDllPro");
               DWORD dwSize = (DWORD)(nBufSize * nChannels * (int64_t)sizeof(float));

               // create MemMapped file
               std::string strValue = CreateGUID();
               SMPCreateFileMapping(mfData, strValue.c_str(), dwSize);
               // clear it
               ZeroMemory(mfData.pData, dwSize);
               // append name of MemMapped file to command for 'regular' call below
               strCmd += SOUNDDLLPRO_PAR_DATA "=";
               strCmd += strValue;
               }
            } // end recgetdata or plugingetdata

         if (iReturn == SOUNDDLL_RETURN_OK)
            {
            // remove trailing colon!!!!
            trim(strCmd, ';');

            // copy complete command to shared memory
            snprintf(g_SMPIPC.m_mfCmd.pData, CMDBUFSIZE, "%s", strCmd.c_str());
            if (!SetEvent(g_SMPIPC.m_hIPCEvent[SMP_IPC_EVENT_CMD]))
               throw SOUNDMEX_Error("error setting CMD event");

            DWORD nWaitResult;
            bool bCommandDone = false;
            while (!bCommandDone)
               {
               // wait for mutex signals (only for done and error). With a timeout we
               // check, if the IPC  process is still alive
               nWaitResult = WaitForMultipleObjects(2, &g_SMPIPC.m_hIPCEvent[SMP_IPC_EVENT_DONE], false, 1000);
               switch (nWaitResult)
                  {
                  // first is done
                  case (WAIT_OBJECT_0):
                     iReturn = SOUNDDLL_RETURN_OK;
                     bCommandDone = true;
                     break;
                  // timeout: only check, if IPC process still running
                  case (WAIT_TIMEOUT):
                     if (!g_SMPIPC.IPCProcessActive())
                        {
                        g_SMPIPC.IPCExit();
                        throw SOUNDMEX_Error("IPC process handle invalid");
                        }
                     break;
                  // all others (abandoned, exit, error): break. We do not handle errors here
                  default:
                     iReturn = SOUNDDLL_RETURN_ERROR;
                     bCommandDone = true;
                     break;
                  }
               }
            }
         // command done: get return string
         std::string strTmp = g_SMPIPC.m_mfCmd.pData;

         ParseValues(std::string(g_SMPIPC.m_mfCmd.pData), vsRet);

         std::string strRetError = GetValue(vsRet, SOUNDDLLPRO_CMD_ERROR);
         // extract error (if any)
         if (!!strRetError.length())
            {
            // do _not_ plot this error for 'resetasyncerror' and 'asyncerror'
            if (  0!=_strcmpi(strCommand.c_str(), SOUNDDLLPRO_CMD_RESETERRORA)
               && 0!=_strcmpi(strCommand.c_str(), SOUNDDLLPRO_CMD_ASYNCERROR)
               )
               strError = strRetError;
            // and remove it from return values
            RemoveValue(vsRet, SOUNDDLLPRO_CMD_ERROR);
            }

         //------------------------------------------------------------------------------
         //*********** BELOW ONLY RETURN VALUES ARE TO BE SET****************************
         //       AND SPECIAL STUFF for 'exit' +  other internal and mixed commands
         //------------------------------------------------------------------------------
         if (!!vsRet.size() && iReturn == SOUNDDLL_RETURN_OK)
            {
            //show licence on init!
            if ( 0==_strcmpi(strCommand.c_str(), SOUNDDLLPRO_CMD_INIT))
               {
               // set default error behaviour ...
               g_nShowError   = 1;
               // ... and last error
               g_strLastError = "no error";
               // if licence type is empty, name it Demo
               if (!GetValue(vsRet, SOUNDDLLPRO_STR_Type).length())
                  SetValue(vsRet, SOUNDDLLPRO_STR_Type, "Demo");

               if (!bQuietInit)
                  {
                  // retrieve update error (if any)
                  std::string strLicValue = GetValue(vsRet, SOUNDDLLPRO_STR_Update);
                  printf("---------------------------------------------------------------------\n");
                  #ifdef BETAVERSION
                  printf("\t\t\tSoundMexPro %s %s.%d BETA", GetValue(vsRet, SOUNDDLLPRO_STR_Type).c_str(), GetValue(vsRet, SOUNDDLLPRO_STR_Version).c_str(), SOUNDMEX_SUBVER);
                  // print update error in brackets (if any)
                  if (strlen(strLicValue.c_str()))
                     printf(" (%s)", strLicValue.c_str());
                  printf("\n");
                  #else
                  printf("\t\t\tSoundMexPro %s %s.%d", GetValue(vsRet, SOUNDDLLPRO_STR_Type).c_str(), GetValue(vsRet, SOUNDDLLPRO_STR_Version).c_str(), SOUNDMEX_SUBVER);
                  // print update error in brackets (if any)
                  if (strlen(strLicValue.c_str()))
                     printf(" (%s)", strLicValue.c_str());
                  printf("\n");
                  #endif
                  strLicValue = GetValue(vsRet, SOUNDDLLPRO_STR_Name);
                  if (strlen(strLicValue.c_str()))
                     printf("\t\t\tLicensed to:\t %s\n", strLicValue.c_str());
                  printf("---------------------------------------------------------------------\n");
                  }
               // remove all values except Type
               std::string s = GetValue(vsRet, SOUNDDLLPRO_STR_Type);
               vsRet.clear();
               SetValue(vsRet, SOUNDDLLPRO_STR_Type, s.c_str());
               } // init license printing end

            // special for 'getrecdata' and 'plugingetdata': read data from returned
            // memory mapped files
            if (  0==_strcmpi(strCommand.c_str(), SOUNDDLLPRO_CMD_RECGETDATA)
               || 0==_strcmpi(strCommand.c_str(), SOUNDDLLPRO_CMD_PLUGINGETDATA)
               )
               {
               // get buffer length
               int64_t nBufSize;
               int64_t nDataPos;
               int64_t nChannels;

               if (!TryStrToInteger(GetValue(vsRet, SOUNDDLLPRO_PAR_BUFSIZE), nBufSize) || nBufSize < 0)
                  throw SOUNDMEX_Error("invalid buffersize value returned from command: " + GetValue(vsRet, SOUNDDLLPRO_PAR_BUFSIZE));
               if (!TryStrToInteger(GetValue(vsRet, SOUNDDLLPRO_PAR_CHANNELS), nChannels) || nChannels < 0)
                  throw SOUNDMEX_Error("invalid channel number value returned from command: " + GetValue(vsRet, SOUNDDLLPRO_PAR_BUFSIZE));

               // ... and data position
               if (0==_strcmpi(strCommand.c_str(), SOUNDDLLPRO_CMD_RECGETDATA))
                  {
                  if (!TryStrToInteger(GetValue(vsRet, SOUNDDLLPRO_PAR_POSITION), nDataPos))// || nDataPos < 0)
                     throw SOUNDMEX_Error("invalid data position value returned from command: " + GetValue(vsRet, SOUNDDLLPRO_PAR_POSITION));
                  }

               // then compare it with sizes that MUST have been passed from Python
               if (nSamplesPy != nBufSize)
                  {
                  AnsiString as;
                  as.printf("sample count mismatch Python/SoundDllPro (%d/%d)", (int)nSamplesPy,  (int)nBufSize);
                  throw SOUNDMEX_Error(as.c_str());
                  }
               if (nChannelsPy != nChannels)
                  {
                  AnsiString as;
                  as.printf("channel count mismatch Python/SoundDllPro (%d/%d)", (int)nChannelsPy,  (int)nChannels);
                  throw SOUNDMEX_Error(as.c_str());
                  }

               // copy samples to destination as they are (float)
               CopyMemory((void*)nDataDestPy, mfData.pData, (uint64_t)(nChannels*nBufSize)*sizeof(float));
               }
            else if (  0 == _strcmpi(strCommand.c_str(), SOUNDDLLPRO_CMD_HELP)
               || 0 == _strcmpi(strCommand.c_str(), SOUNDDLLPRO_CMD_HELPA)
               )
               {
               }
            // all other commands (not recgetdata or plugingetdata): standard processing of return values
            else
               {
               // for 'version' we append the subversion to the value returned from DLL
               if ( 0==_strcmpi(strCommand.c_str(), SOUNDDLLPRO_CMD_VERSION))
                  {
                  #ifdef BETAVERSION
                  sprintf(sz, "%s.%d_BETA", GetValue(vsRet, SOUNDDLLPRO_STR_Version).c_str(), SOUNDMEX_SUBVER);
                  SetValue(vsRet, SOUNDDLLPRO_STR_Version, (GetValue(vsRet, SOUNDDLLPRO_STR_Version) + "." + IntegerToStr(SOUNDMEX_SUBVER) + "_BETA").c_str());
                  #else
                  SetValue(vsRet, SOUNDDLLPRO_STR_Version, (GetValue(vsRet, SOUNDDLLPRO_STR_Version) + "." + IntegerToStr(SOUNDMEX_SUBVER)).c_str());
                  #endif
                  }
               }

            // re-create return value from vsRet
            strCmd = "";
            for (i = 0; i < vsRet.size(); i++)
               strCmd = strCmd + vsRet[i] + ";";
            // print it to return buffer
            snprintf(lpcszReturn, (size_t)nRetLen, "%s", strCmd.c_str());
            }

         } // IsDllCommand
      // it's _not_ a DLLCommand
      else
         {
         ParseValues(strCmd, vsRet);
         std::string strTmp;
         // - 'showerror'
         if (0==_strcmpi(strCommand.c_str(), SOUNDDLLPRO_CMD_SHOWERROR))
            {
            if (vsRet.size() > 2)
               throw SOUNDMEX_Error("invalid parameter passed to 'showerror'");
            strTmp = GetValue(vsRet, SOUNDDLLPRO_PAR_VALUE);
            if (strTmp == "1")
               g_nShowError = 1;
            else if (strTmp == "2")
               g_nShowError = 2;
            else if (strTmp == "0")
               g_nShowError = 0;
            else if (!!strTmp.length())
               throw SOUNDMEX_Error("invalid value for parameter 'value' passed to 'showerror'");
            // write current  value to return buffer
            snprintf(lpcszReturn, (size_t)nRetLen, "value=%d", g_nShowError);
            iReturn = SOUNDDLL_RETURN_OK;
            }
         // - 'getlasterror'
         else if (0==_strcmpi(strCommand.c_str(), SOUNDDLLPRO_CMD_GETLASTERROR))
            {
            if (vsRet.size() > 1)
               throw SOUNDMEX_Error("invalid parameter passed to 'getlasterror'");
            // write last error
            // write current  value to return buffer
            snprintf(lpcszReturn, (size_t)nRetLen, "error=%s", g_strLastError.c_str());
            iReturn = SOUNDDLL_RETURN_OK;
            }
         }

      //------------------------------------------------------------------------------
      // help printing
      //------------------------------------------------------------------------------
      if (  0==_strcmpi(strCommand.c_str(), SOUNDDLLPRO_CMD_HELP)
         || 0==_strcmpi(strCommand.c_str(), SOUNDDLLPRO_CMD_HELPA)
         )
         {

         // now argument (i.e. command list requested?)
         bool bShowCommandList = strHelpCommand.length() == 0;

         // start printing standard
         printf("\n---------------------------------------------------------------------------------------\n");
         printf("\t [errocode, ...] = soundmexpro(Name, par1, value1, par2, value2...)\n");
         printf("\t errocode is 1 on success of command and 0 on any error.\n");
         printf("---------------------------------------------------------------------------------------\n");

         if (bShowCommandList)
            {
            if (0==_strcmpi(strCommand.c_str(), SOUNDDLLPRO_CMD_HELPA))
               printf("\nAvailable commands are (alphabetical order):\n");
            else
               printf("\nAvailable commands are (logical order):\n");

            printf("\n%s", GetValue(vsRet, SOUNDDLLPRO_CMD_HELP).c_str());
            }
         else
            {
            printf("\t parameters (Par.>), values, default values (Def.>)\n");
            printf("\t and additional return values (Ret.>) are:\n\n");
            printf("%s\n", GetValue(vsRet, SOUNDDLLPRO_CMD_HELP).c_str());
            }
         printf("\n---------------------------------------------------------------------------------------\n");
         }
      //------------------------------------------------------------------------------
      // finally special handling for exit: unload the library
      //------------------------------------------------------------------------------
      else if ( 0==_strcmpi(strCommand.c_str(), SOUNDDLLPRO_CMD_EXIT))
         {
         g_SMPIPC.IPCExit();
         }
      } // try
   catch (SOUNDMEX_Error &e)
      {
      strError     = e.Message;
      iReturn    = e.iErrorType;
      }

   catch (...)
      {
      strError     = "Unknown C++ exception in soundmexpro";
      iReturn    = SOUNDDLL_RETURN_MEXERROR;
      }

   SMPReleaseFileMapping(mfData);

   bool bErrorDisplayed = false;
   nCommandCounter--;
   if (g_nShowError && !!strError.length())
      {
      bErrorDisplayed = true;
      std::string sTmp = "Error in command " + strCommand + ":\n" + strError + "\n";
      if (g_nShowError == 1)
         printf("%s", sTmp.c_str());
      else
         printf("%s", sTmp.c_str());
      }

   // copy error to last error
   if (!!strError.length())
      g_strLastError = strError;
   return iReturn;
} // SoundMexPro

} // extern "C"


