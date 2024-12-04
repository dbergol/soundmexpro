//------------------------------------------------------------------------------
/// \file SoundDllPro_IPC.cpp
/// \author Berg
/// \brief Implementation of helper functions for Inter-Process-Communication
///
/// Project SoundMexPro
/// Modules: all
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

#include "soundmexpro_ipc.h"

// NOTE: Applications using VCL SMPIPC, SoundMexProPy32 and SoundMexProPy64) must
// define USE_VCL !!
#ifdef USE_VCL
   #include <vcl.h>
   #define IPCException Exception
#else
   #include "soundmexpro.h"
   #define IPCException SOUNDMEX_Error
#endif
#include <stdio.h>


/// - tool function  to get own module handle
HMODULE GetCurrentModule()
{
  HMODULE hModule = NULL;
  if(GetModuleHandleEx(GET_MODULE_HANDLE_EX_FLAG_FROM_ADDRESS, (LPCTSTR)GetCurrentModule, &hModule)!=0)
    return hModule;
  else return NULL;
}



//------------------------------------------------------------------------------
/// returns a GUID a std::string
//------------------------------------------------------------------------------
std::string CreateGUID()
{
   // create a GUID to use for the identifiers of events and memory mapped file
   GUID guid;
   if (S_OK != CoCreateGuid(&guid))
      throw IPCException("Error creating a GUID");
   wchar_t wc[255];
   ZeroMemory(wc, sizeof(wc));
   if (0 == StringFromGUID2(guid, wc, 255))
      throw IPCException("Error 2 creating a GUID");
   // convert from wchar_t to char
   char lpszGUID[255];
   ZeroMemory(lpszGUID, sizeof(lpszGUID));
   snprintf(lpszGUID, 255, "%ls", wc);
   return std::string(lpszGUID);
}
//------------------------------------------------------------------------------

//------------------------------------------------------------------------------
/// constructor
//------------------------------------------------------------------------------
MapFile::MapFile()
   : hHandle(NULL), pData(NULL)
{
}
//------------------------------------------------------------------------------

//------------------------------------------------------------------------------
/// Creates a global named memory mapped file as shared memory
//------------------------------------------------------------------------------
void SMPCreateFileMapping(MapFile& mf, LPCSTR lpcszName, DWORD dwSize)
{
   SMPReleaseFileMapping(mf);
   std::string strName = "Local\\";
   strName += lpcszName;

   if (dwSize == 0)
      dwSize = CMDBUFSIZE;
   mf.hHandle = CreateFileMapping(
                    INVALID_HANDLE_VALUE,                      // use paging file
                    NULL,                                      // default security
                    PAGE_READWRITE,                            // read/write access
                    0, // max. object size, upper 32 bit (not used)
                    dwSize,// max. object size, lower 32 bit (not used)
                    strName.c_str());    // name of mapping object

   if (::GetLastError() == ERROR_ALREADY_EXISTS)
      {
      if (!!mf.hHandle)
         {
         CloseHandle(mf.hHandle);
         mf.hHandle = NULL;
         }
      throw IPCException("error creating memory mapped file: object unexpectedly already exists");
      }


   if (mf.hHandle == NULL)
      {
      std::string strError = "error creating memory mapped file: ";
      strError += GetLastWindowsErrorString();
      throw IPCException(strError.c_str());
      }

   mf.pData = (LPTSTR) MapViewOfFile(
                           mf.hHandle,          // handle to map object
                           FILE_MAP_ALL_ACCESS, // read/write permission
                           0,
                           0,
                           0);

   if (mf.pData == NULL)
      {
      std::string strError = "error creating map view of file: ";
      strError += GetLastWindowsErrorString();
      CloseHandle(mf.hHandle);
      mf.hHandle = NULL;
      throw IPCException(strError.c_str());
      }

 
   // initialize it with zeroes
   memset((void*)mf.pData, 0, dwSize);
}
//------------------------------------------------------------------------------

//------------------------------------------------------------------------------
/// Accesses existing global named memory mapped file as shared memory
//------------------------------------------------------------------------------
void SMPAccessFileMapping(MapFile &mf, LPCSTR lpcszName)
{
   SMPReleaseFileMapping(mf);
   // if conversion fails try to access it as memory mapped file
   std::string strName = "Local\\";
   strName += lpcszName;
   mf.hHandle = OpenFileMapping(FILE_MAP_ALL_ACCESS, FALSE, strName.c_str());
   if (mf.hHandle == NULL)
      throw IPCException("cannot access memory mapped file");
   mf.pData = (LPTSTR)MapViewOfFile(mf.hHandle,
                                    FILE_MAP_ALL_ACCESS,
                                    0,
                                    0,
                                    0
                                    );
   if (mf.pData == NULL)
      {
      CloseHandle(mf.hHandle);
      mf.hHandle = NULL;
      throw IPCException("cannot get map view of memory mapped file");
      }

}
//------------------------------------------------------------------------------

//------------------------------------------------------------------------------
/// cleans up shared memory
//------------------------------------------------------------------------------
void SMPReleaseFileMapping(MapFile& mf)
{
   if (mf.pData)
      {
      try
         {
         UnmapViewOfFile(mf.pData);
         }
      catch (...)
         {
         }
      mf.pData = NULL;
      }
   if (mf.hHandle)
      {
      try
         {
         CloseHandle(mf.hHandle);
         }
      catch (...)
         {
         }
      mf.hHandle = NULL;
      }
}
//------------------------------------------------------------------------------

//------------------------------------------------------------------------------
/// Returns string with last windows error
//------------------------------------------------------------------------------
std::string GetLastWindowsErrorString()
{
   DWORD dw = ::GetLastError();
   LPVOID lpMsgBuf;
   FormatMessage(
       FORMAT_MESSAGE_ALLOCATE_BUFFER |
       FORMAT_MESSAGE_FROM_SYSTEM |
       FORMAT_MESSAGE_IGNORE_INSERTS,
       NULL,
       dw,
       MAKELANGID(LANG_NEUTRAL, SUBLANG_DEFAULT), // Default language
       (LPTSTR) &lpMsgBuf,
       0,
       NULL
   );

   std::string strError = (LPCSTR)lpMsgBuf;
   // Free the buffer.
   LocalFree( lpMsgBuf );

   return strError;
}
//------------------------------------------------------------------------------

//------------------------------------------------------------------------------
/// constructor
//------------------------------------------------------------------------------
SMPIPCProcess::SMPIPCProcess ()
   : m_hIPCProcessHandle(NULL), m_hIPCThreadHandle(NULL)
{
}
//------------------------------------------------------------------------------

//------------------------------------------------------------------------------
/// Initzializes IPC by creating
/// - Shared Memory for command line
/// - events for IPC communication
/// - IPC process
//------------------------------------------------------------------------------
void  SMPIPCProcess::IPCInit()
{
   IPCExit();

   try
      {
      std::string strGUID = CreateGUID();
      SMPCreateFileMapping(m_mfCmd, strGUID.c_str(), CMDBUFSIZE);
      char c[255];
      // create named events non-signaled and auto-resetting
      for (int i = 0; i < SMP_IPC_EVENT_LAST; i++)
         {
         snprintf(c, 255, "%s%d", strGUID.c_str(), i);
         m_hIPCEvent[i]    = CreateEvent(NULL, FALSE, FALSE, c);
         if (!m_hIPCEvent[i])
            throw IPCException("error creating IPC events");
         }
      IPCCreateProcess(strGUID.c_str());
      }
   catch (...)
      {
      IPCExit();
      throw;
      }
}
//------------------------------------------------------------------------------

//------------------------------------------------------------------------------
/// Cleans up IPC
//------------------------------------------------------------------------------
void SMPIPCProcess::IPCExit()
{
   try
      {
      // terminate IPC process
      if (m_hIPCProcessHandle)
         {
         DWORD dw = GetTickCount();
         DWORD dwExit;
         // tell SMPIPC.EXE to quit
         SetEvent(m_hIPCEvent[SMP_IPC_EVENT_EXIT]);
         // and wait for it (1000 second grace period=
         while (1)
            {
            GetExitCodeProcess(m_hIPCProcessHandle, &dwExit);
            Sleep(1);
            if (dwExit != STILL_ACTIVE || (GetTickCount() - dw) > 1000)
               break;
            }
         if (dwExit == STILL_ACTIVE)
            TerminateProcess(m_hIPCProcessHandle, 0);
         }
      }
   catch (...)
      {
      }
   // close handles
   if (m_hIPCProcessHandle)
      CloseHandle(m_hIPCProcessHandle);
   m_hIPCProcessHandle = NULL;
   if (m_hIPCThreadHandle)
      CloseHandle(m_hIPCThreadHandle);
   m_hIPCThreadHandle = NULL;

   SMPReleaseFileMapping(m_mfCmd);
   // cleanup events
   for (int i = 0; i < SMP_IPC_EVENT_LAST; i++)
      {
      if (m_hIPCEvent[i] != NULL)
         {
         CloseHandle(m_hIPCEvent[i]);
         m_hIPCEvent[i] = NULL;
         }
      }
}
//------------------------------------------------------------------------------

//------------------------------------------------------------------------------
/// Checks, whether process in g_hIPCProcessHandle is still activbe
//------------------------------------------------------------------------------
bool SMPIPCProcess::IPCProcessActive()
{
   if (!m_hIPCProcessHandle)
      return false;
   if (WAIT_OBJECT_0 == WaitForMultipleObjects(1, &m_hIPCProcessHandle, FALSE, 0))
      return false;
   return true;
}
//------------------------------------------------------------------------------

//------------------------------------------------------------------------------
/// creates IPC process
//------------------------------------------------------------------------------
void SMPIPCProcess::IPCCreateProcess(LPCSTR lpszGUID)
{
   if (!!m_hIPCProcessHandle)
      throw IPCException("global IPC process handle not NULL on process creation");

   // find path to SMPIPC.EXE
   char cModule[10*MAX_PATH];
   ZeroMemory(cModule, 10*MAX_PATH);

   // check, if 'soundmexpro.dll' or 'soundmexpro.mexw32' or 'soundmexpro.mexw64'
   // is the loader here
   HMODULE hThis = GetCurrentModule();
   if (!hThis)
      hThis = GetModuleHandle(SOUNDMEX_MODULE);
   if (!hThis)
      hThis = GetModuleHandle(SOUNDMEX_MODULE7);
   if (!hThis)
      hThis = GetModuleHandle(SOUNDMEX_MODULE7_64);
   if (!hThis)
      hThis = GetModuleHandle(SOUNDMEX_MODULEPY);

   if (!GetModuleFileName(hThis, cModule, MAX_PATH))
      throw IPCException("Unknown GetModuleFileName error");

   // add path of SoundDllPro.dll to search path (for libsndfile-1.dll)
   char drive[_MAX_DRIVE];
   char dir[_MAX_DIR];
   char file[_MAX_FNAME];
   char ext[_MAX_EXT];
   _splitpath(cModule,drive,dir,file,ext);
   std::string strCmd = std::string(drive) + std::string(dir);
   strCmd += "\\" SOUNDMEX_IPC " ";
   strCmd += lpszGUID;


   // create process
   SECURITY_ATTRIBUTES  sec;
   STARTUPINFO          start;
   PROCESS_INFORMATION  pinfo;
   memset(&start, 0, sizeof(start));
   memset(&pinfo, 0, sizeof(pinfo));
   start.cb = sizeof(start);
   start.wShowWindow = SW_SHOWDEFAULT; // : SW_HIDE;
   start.dwFlags = STARTF_USESHOWWINDOW;

   sec.nLength = sizeof(sec);
   sec.lpSecurityDescriptor = NULL;
   sec.bInheritHandle = FALSE;

   // note: CreateProcess uses LPTSTR instead LPCTSTR, which seems to be an error.
   if ( !CreateProcess( NULL,
                        const_cast<LPTSTR>(strCmd.c_str()),
                        &sec,
                        &sec,
                        FALSE,
                        0,
                        NULL,
                        NULL,
                        &start,
                        &pinfo)
                        )
      {
      std::string strError = "error creating SoundMexPro IPC process: ";
      strError += GetLastWindowsErrorString();
      throw IPCException(strError.c_str());
      }

   // store process handle in global handle
   m_hIPCProcessHandle  = pinfo.hProcess;
   m_hIPCThreadHandle   = pinfo.hThread;


   // wait for IPC process to signal 'I'm ready', 'error occurred' - or to be terminated unexpectedly
   HANDLE h[3] = {m_hIPCEvent[SMP_IPC_EVENT_DONE], m_hIPCEvent[SMP_IPC_EVENT_ERROR], m_hIPCProcessHandle};
   DWORD nWaitResult = WaitForMultipleObjects(3, &h[0], false, 6000);
   std::string strError;
   switch (nWaitResult)
      {
      // ok: seems to be fine!
      case  (WAIT_OBJECT_0):
         break;
      case  (WAIT_OBJECT_0 + 1):
         {
         strError = "SMPIPC startup error: ";
         strError += m_mfCmd.pData;
         if (!IPCProcessActive())
            strError += " (IPC process not active)";
         throw IPCException(strError.c_str());
         }
      case  (WAIT_OBJECT_0 + 2):
         {
         strError = "SMPIPC startup error: IPC process unexpectedly terminated: check existance of runtime libraries listed in the manual's Getting Started section";
         throw IPCException(strError.c_str());
         }
      case (WAIT_TIMEOUT):
         {
         strError = "SMPIPC startup error (timeout): ";
         strError += m_mfCmd.pData;
         if (!IPCProcessActive())
            strError += " (IPC process not active)";
         throw IPCException(strError.c_str());
         }
      }
}
//------------------------------------------------------------------------------

