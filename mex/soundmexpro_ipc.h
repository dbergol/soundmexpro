//------------------------------------------------------------------------------
/// \file SoundDllPro_IPC.h
/// \author Berg
/// \brief Interface of helper functions for Inter-Process-Communication
///
/// Project SoundMexPro
/// Modules: all
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
#ifndef soundmexpro_ipcH
#define soundmexpro_ipcH

#include <windows.h>
#include <string>

// filename definitions
#define SOUND_MODULE         "SOUNDDLLPRO.DLL"
#define SOUNDMEX_MODULE      "SOUNDMEXPRO.DLL"
#define SOUNDMEX_MODULE7     "soundmexpro.mexw32"
#define SOUNDMEX_MODULE7_64  "soundmexpro.mexw64"
#define SOUNDMEX_MODULEPY    "soundmexpropy.dll"

#define SOUNDMEX_IPC         "SMPIPC.EXE"
/// definition of buffer size (shared memory) for command line buffer
#define CMDBUFSIZE 2*SHRT_MAX

// tool functio prototypes
std::string CreateGUID();
HMODULE     GetCurrentModule();
std::string GetLastWindowsErrorString();

//------------------------------------------------------------------------------
/// definition of Events for inter-process-synchronization
//------------------------------------------------------------------------------
enum
   {
   SMP_IPC_EVENT_EXIT   = 0,
   SMP_IPC_EVENT_CMD,
   SMP_IPC_EVENT_DONE,
   SMP_IPC_EVENT_ERROR,
   SMP_IPC_EVENT_LAST
   };
//------------------------------------------------------------------------------


//------------------------------------------------------------------------------
/// definition of class for a memory mapped file containing handle and
// pointer to data
//------------------------------------------------------------------------------
class MapFile
{
   public:
      MapFile();
      HANDLE   hHandle;
      LPTSTR   pData;
};
//------------------------------------------------------------------------------

//------------------------------------------------------------------------------
/// Tools functions for handling a memory mappedfile,
//------------------------------------------------------------------------------
void     SMPCreateFileMapping(MapFile& mf, LPCSTR lpcszName, DWORD dwSize);
void     SMPAccessFileMapping(MapFile &mf, LPCSTR lpcszName);
void     SMPReleaseFileMapping(MapFile& mf);
//------------------------------------------------------------------------------

//------------------------------------------------------------------------------
/// Class for handling the SMP Interporocess-Communication
//------------------------------------------------------------------------------
class SMPIPCProcess
{
   private:
      // global process handlee for IPC process
      HANDLE      m_hIPCProcessHandle;
      // corresponding thread handle
      HANDLE      m_hIPCThreadHandle;
   public:
      // array of handles for IPC events
      HANDLE      m_hIPCEvent[SMP_IPC_EVENT_LAST];
      MapFile     m_mfCmd;

      SMPIPCProcess();
      void IPCInit();
      void IPCExit();
      bool IPCProcessActive();
      void IPCCreateProcess(LPCSTR lpszGUID);
};
//------------------------------------------------------------------------------
#endif
