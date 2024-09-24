//------------------------------------------------------------------------------
/// \file MPlugin.cpp
/// \author Berg
/// \brief Implementation of class TMPlugin (DSP with MATLAB)
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
#include <windowsx.h>
#include <objbase.h>
#include "SoundDllPro_Tools.h"
#include "SoundDllPro_Main.h"

#pragma hdrstop

#include "MPlugin.h"
//------------------------------------------------------------------------------

//------------------------------------------------------------------------------
/// TMPlugin
///
/// Class for interprocess communication with a MATLAB process that does signal
/// processing. The general strategy of the class is as follows:
///
/// - a named shared memory object is created
/// - multiple named events for interprocess synchronisation are created
/// - a MATLAB process is created. As startup command for that MATLAB instance
///   the 'secondary MEX' mpluginmex.dll is called with the arguments
///   - startup script name
///   - processing script name
///   - name of shared memory object
///   - number of input channels (recording)
///   - number of output channels (playback)
///   - number of samples per channel
/// - the secondary MEX accesses the shared memory and the sync events, using
///   them to signal 'ready' or 'error' respectively on startup. Then it waits
///   TMPlugin to call the processing by setting en event.
/// - TMPlugin::Process gets audio data and copies them to the shared memory.
///   Additionally 'user data' are copied to shared memory. Then an event is set
///   to signal to secondary MEX to start processing. The secondary MEX reads from
///   shared memory, copies data to mxArrays and calls a processing script. Returned
///   data are copied back to shared memory and an event is set to signal 'processing
///   done' or 'error' respectively
//------------------------------------------------------------------------------
TMPlugin::TMPlugin(MPluginStruct &mps)
   :  m_hProcessHandle(NULL),
      m_hMapFile(NULL),
      m_pDataBuf(NULL)
{
   for (int i = 0; i < IPC_EVENT_LAST; i++)
      m_hIPCEvent[i] = NULL;

   // copy value structure
   m_mps = mps;
   // check mandatory values
   if (m_mps.strMatlab.IsEmpty())
      m_mps.strMatlab = "matlab";
   if (m_mps.strPlugin.IsEmpty())
      m_mps.strPlugin = "mpluginmex";
   if (m_mps.strProcCmd.IsEmpty())
      throw Exception("process command empty!");
   if (!m_mps.nSamples || (!m_mps.nInChannels && !m_mps.nOutChannels))
      throw Exception("channels and/or samples empty");
   if (!m_mps.nUserDataSize)
      throw Exception("userdata size empty");

   m_vvfInUserData.resize(m_mps.nInChannels);
   for (unsigned int n = 0; n < m_mps.nInChannels; n++)
      m_vvfInUserData[n].resize(m_mps.nUserDataSize);
   m_vvfOutUserData.resize(m_mps.nOutChannels);
   for (unsigned int n = 0; n < m_mps.nOutChannels; n++)
      m_vvfOutUserData[n].resize(m_mps.nUserDataSize);
   ClearUserData();

   m_nDataBytesPerChannel     = (int)(m_mps.nSamples*sizeof(float));
   m_nUserBytesPerChannel     = (int)(m_mps.nUserDataSize * sizeof(float));

   InitializeCriticalSection(&m_csLock);
}
//------------------------------------------------------------------------------

//------------------------------------------------------------------------------
/// destructor. Does cleanup
//------------------------------------------------------------------------------
TMPlugin::~TMPlugin()
{
   Exit();
   DeleteCriticalSection(&m_csLock);
}
//------------------------------------------------------------------------------

//------------------------------------------------------------------------------
/// sets all user data to 0 
//------------------------------------------------------------------------------
void __fastcall   TMPlugin::ClearUserData()
{
   for (unsigned int n = 0; n < m_vvfInUserData.size(); n++)
      m_vvfInUserData[n] = 0.0f;

   for (unsigned int n = 0; n < m_vvfOutUserData.size(); n++)
      m_vvfOutUserData[n] = 0.0f;
}
//------------------------------------------------------------------------------

//------------------------------------------------------------------------------
// Processing routine. Description see constructor above.
//------------------------------------------------------------------------------
void __fastcall   TMPlugin::Process(std::vector<std::valarray<float> >& vvfInBuffers,
                                    std::vector<std::valarray<float> >& vvfOutBuffers)
{
   AnsiString asError;
   // copy must be synced!!
   EnterCriticalSection(&m_csLock);
   try
      {
      unsigned int nInChannels   = (unsigned int)vvfInBuffers.size();
      unsigned int nOutChannels  = (unsigned int)vvfOutBuffers.size();
      if (  nInChannels != m_mps.nInChannels
         || nOutChannels != m_mps.nOutChannels
         )
         throw Exception("engine sizing error 1");

      unsigned int nSamples = 0;
      if (nOutChannels)
         nSamples = (unsigned int)vvfOutBuffers[0].size();
      else if (nInChannels)
         nSamples = (unsigned int)vvfInBuffers[0].size();
      if (nSamples != m_mps.nSamples)
         throw Exception("engine sizing error 2");


      // copy data to shared memory
      int nDataPos = 0;
      unsigned int nChannelIndex;
      for (nChannelIndex = 0; nChannelIndex < nInChannels; nChannelIndex++)
         {
         if (vvfInBuffers[nChannelIndex].size() != nSamples)
            throw Exception("engine sizing error 3");
         CopyMemory((PVOID)&m_pDataBuf[nDataPos], &vvfInBuffers[nChannelIndex][0], m_nDataBytesPerChannel);
         // clear data
         vvfInBuffers[nChannelIndex] = 0;
         nDataPos += m_nDataBytesPerChannel;
         }
      for (nChannelIndex = 0; nChannelIndex < nOutChannels; nChannelIndex++)
         {
         if (vvfOutBuffers[nChannelIndex].size() != nSamples)
            throw Exception("engine sizing error 4");
         CopyMemory((PVOID)&m_pDataBuf[nDataPos], &vvfOutBuffers[nChannelIndex][0], m_nDataBytesPerChannel);
         // clear data
         vvfOutBuffers[nChannelIndex] = 0;
         nDataPos += m_nDataBytesPerChannel;
         }

      for (nChannelIndex = 0; nChannelIndex < nInChannels; nChannelIndex++)
         {
         CopyMemory((PVOID)&m_pDataBuf[nDataPos], &m_vvfInUserData[nChannelIndex][0], m_nUserBytesPerChannel);
         nDataPos += m_nUserBytesPerChannel;
         }
      for (nChannelIndex = 0; nChannelIndex < nOutChannels; nChannelIndex++)
         {
         CopyMemory((PVOID)&m_pDataBuf[nDataPos], &m_vvfOutUserData[nChannelIndex][0], m_nUserBytesPerChannel);
         nDataPos += m_nUserBytesPerChannel;
         }

      if (!SetEvent(m_hIPCEvent[IPC_EVENT_PROCESS]))
         throw Exception("error setting process event");

      // wait for 'done' or 'error'
      DWORD nWaitResult = WaitForMultipleObjects(2, &m_hIPCEvent[IPC_EVENT_ERROR], false, 1000);
      switch (nWaitResult)
         {
         // first is error
         case (WAIT_OBJECT_0):
            // try to read error string from shared memory
            try {
               asError = (const char*)(&m_pDataBuf[0]);
               }
            catch(...)
               {
               asError = "error retrieving erro message form script plugin";
               }
            throw Exception("error returned from MATLAB script plugin: " + asError);
         // second is 'done':
         case (WAIT_OBJECT_0 + 1):
            break;
         case (WAIT_TIMEOUT):
            throw Exception("timeout calling MATLAB script plugin");
         default:
            throw Exception("unexpected error");
         }

      // on success copy sample and user data back from shared memory
      nDataPos = 0;
      for (nChannelIndex = 0; nChannelIndex < nInChannels; nChannelIndex++)
         {
         CopyMemory(&vvfInBuffers[nChannelIndex][0], (PVOID)&m_pDataBuf[nDataPos], m_nDataBytesPerChannel);
         nDataPos += m_nDataBytesPerChannel;
         }
      for (nChannelIndex = 0; nChannelIndex < nOutChannels; nChannelIndex++)
         {
         CopyMemory(&vvfOutBuffers[nChannelIndex][0], (PVOID)&m_pDataBuf[nDataPos], m_nDataBytesPerChannel);
         nDataPos += m_nDataBytesPerChannel;
         }
      for (nChannelIndex = 0; nChannelIndex < nInChannels; nChannelIndex++)
         {
         CopyMemory(&m_vvfInUserData[nChannelIndex][0], (PVOID)&m_pDataBuf[nDataPos], m_nUserBytesPerChannel);
         nDataPos += m_nUserBytesPerChannel;
         }
      for (nChannelIndex = 0; nChannelIndex < nOutChannels; nChannelIndex++)
         {
         CopyMemory(&m_vvfOutUserData[nChannelIndex][0], (PVOID)&m_pDataBuf[nDataPos], m_nUserBytesPerChannel);
         nDataPos += m_nUserBytesPerChannel;
         }
      }
   __finally
      {
      LeaveCriticalSection(&m_csLock);
      }
}
//------------------------------------------------------------------------------

//------------------------------------------------------------------------------
/// Initializes events and shared memory, creates MATLAB process wih startup command
/// and waits for 'ok' from created secondary MEX
//------------------------------------------------------------------------------
void __fastcall TMPlugin::Init(bool bRealtimePriority)
{
   Exit();
   m_hProcessHandle = NULL;
      
   // create a GUID to use for the identifiers of events and memory mapped file
   GUID guid;
   if (S_OK != CoCreateGuid(&guid))
      throw Exception("Error creating a GUID");
   AnsiString strGUID = GUIDToString(guid);

   AnsiString strMemoryMappedFile = CreateMemoryMappedFile(strGUID);


   // retrieve Sounddllpro.dll path to add it to search path of MPlugin's
   // own MATLAB process
   AnsiString strBinPath = g_strBinPath;


   // setting up startup string for MATLAB
   AnsiString strMatlabCommand = m_mps.strMatlab;
   if (strMatlabCommand.Pos("matlab.exe") != 0)
      {
      if (!m_mps.bJVM)
         strMatlabCommand += " -nojvm";
      strMatlabCommand += " -nosplash -r";
      }
   else if (strMatlabCommand.Pos("octave") != 0)
      {
      if (!m_mps.bTerminateProcess)
         strMatlabCommand += " --persist";
      strMatlabCommand += " --eval";

      }
   strMatlabCommand += " \"";
   // change directory in created MATLAB!!
   if (!m_mps.strPluginPath.IsEmpty() && DirectoryExists(m_mps.strPluginPath))
      strMatlabCommand += "cd('" + m_mps.strPluginPath + "');";
   // append path if it was available above
   if (!strBinPath.IsEmpty())
      strMatlabCommand += "path(path, '" + strBinPath + "');";
   strMatlabCommand += m_mps.strPlugin + "('";
   strMatlabCommand += strMemoryMappedFile + "', ";
   strMatlabCommand += "'" + m_mps.strInitCmd + "', ";
   strMatlabCommand += "'" + m_mps.strProcCmd + "', ";
   strMatlabCommand += IntToStr((int)m_mps.nInChannels) + ", ";
   strMatlabCommand += IntToStr((int)m_mps.nOutChannels) + ", ";
   strMatlabCommand += IntToStr((int)m_mps.nSamples) + ", ";
   strMatlabCommand += IntToStr((int)m_mps.nUserDataSize) + ");" + "\"";

   // create named events non-signaled and auto-resetting
   for (int i = 0; i < IPC_EVENT_LAST; i++)
      {
      m_hIPCEvent[i]    = CreateEvent(NULL, FALSE, FALSE, (strGUID + IntToStr(i)).c_str());
      if (!m_hIPCEvent[i])
         throw Exception("error creating IPC events");
      }

   SECURITY_ATTRIBUTES  sec;
   STARTUPINFO          start;
   PROCESS_INFORMATION  pinfo;
   memset(&start, 0, sizeof(start));
   memset(&pinfo, 0, sizeof(pinfo));

   start.cb = sizeof(start);
   start.wShowWindow = (WORD)(m_mps.bShowProcess ?  SW_SHOWDEFAULT : SW_HIDE);
   start.dwFlags = STARTF_USESHOWWINDOW;
   sec.nLength = sizeof(sec);
   sec.lpSecurityDescriptor = NULL;
   sec.bInheritHandle = FALSE;

   DWORD dwFlags = bRealtimePriority ? REALTIME_PRIORITY_CLASS : HIGH_PRIORITY_CLASS;

   // note: CreateProcess uses LPTSTR instead LPCTSTR, which seems to be an error.
   if ( !CreateProcess( NULL,
                        strMatlabCommand.c_str(),
                        &sec,
                        &sec,
                        FALSE,
                        dwFlags,
                        NULL,
                        m_mps.strPluginPath.c_str(),
                        &start,
                        &pinfo)
                        )
      {
      AnsiString strError = "error creating MATLAB plugin process: ";
      strError += GetLastWindowsError();
      throw Exception(strError);
      }


   m_hProcessHandle  = pinfo.hProcess;
   m_hThreadHandle   = pinfo.hThread;

   // then wait for init event to be signaled by called process (init or error)
   HANDLE h[2] = {m_hIPCEvent[IPC_EVENT_INIT], m_hIPCEvent[IPC_EVENT_ERROR]};
   DWORD nWaitResult = WaitForMultipleObjects(2, h, false, m_mps.nStartTimeout);

   switch (nWaitResult)
      {
      // first is 'init'
      case  (WAIT_OBJECT_0):
         break;
      case (WAIT_TIMEOUT):
         throw Exception("MATLAB script plugin startup error (timeout)");
      // all others are an error
      default:
         throw Exception("MATLAB script startup error");
      }
}
//------------------------------------------------------------------------------

//------------------------------------------------------------------------------
/// Does cleanup.
//------------------------------------------------------------------------------
void __fastcall TMPlugin::Exit()
{
   // copy must be synced!!
   EnterCriticalSection(&m_csLock);

   try
      {
      if (m_hIPCEvent[IPC_EVENT_EXIT])
         {
         // tell called process/MEX to exit
         SetEvent(m_hIPCEvent[IPC_EVENT_EXIT]);
         // wait for termination event set by called process/MEX
         WaitForSingleObject(m_hIPCEvent[IPC_EVENT_TERMINATED], 5000);
         }
      // kill process
      if (m_hProcessHandle && m_mps.bTerminateProcess)
         {
         DWORD dwExit;
         GetExitCodeProcess(m_hProcessHandle, &dwExit);
         if (dwExit == STILL_ACTIVE)
            TerminateProcess(m_hProcessHandle, 0);
         }
      if (m_hProcessHandle)
         CloseHandle(m_hProcessHandle);
      m_hProcessHandle = NULL;
      if (m_hThreadHandle)
         CloseHandle(m_hThreadHandle);
      m_hThreadHandle = NULL;


      // cleanup memory mapped file
      DeleteMemoryMappedFile();

      // cleanup events
      for (int i = 0; i < IPC_EVENT_LAST; i++)
         {
         if (m_hIPCEvent[i] != NULL)
            {
            CloseHandle(m_hIPCEvent[i]);
            m_hIPCEvent[i] = NULL;
            }
         }
      }
   __finally
      {
      LeaveCriticalSection(&m_csLock);
      }
}
//------------------------------------------------------------------------------

//------------------------------------------------------------------------------
/// Creates a global named memory mapped file as shared memory
//------------------------------------------------------------------------------
AnsiString __fastcall TMPlugin::CreateMemoryMappedFile(AnsiString str)
{
   DeleteMemoryMappedFile();

   unsigned int nTotalBytes = (m_mps.nSamples + m_mps.nUserDataSize) * (m_mps.nInChannels + m_mps.nOutChannels) * sizeof(float);

   // 20.11.2009: changed from "Global" to "Local": otherwise "access denied" error on Vista and higher
   str = "Local\\" + str;
   m_hMapFile = CreateFileMapping(
                    INVALID_HANDLE_VALUE,                      // use paging file
                    NULL,                                      // default security
                    PAGE_READWRITE,                            // read/write access
                    0, // max. object size, upper 32 bit (not used)
                    nTotalBytes,// max. object size, lower 32 bit (not used)
                    str.c_str());    // name of mapping object

   if (::GetLastError() == ERROR_ALREADY_EXISTS)
      {
      if (!!m_hMapFile)
         {
         CloseHandle(m_hMapFile);
         m_hMapFile = NULL;
         }
      throw Exception("error creating memory mapped file: object unexpectedly already exists");
      }


   if (m_hMapFile == NULL)
      {
      AnsiString strError = "error creating memory mapped file: ";
      strError += GetLastWindowsError();
      throw Exception(strError);
      }

   m_pDataBuf = (LPTSTR) MapViewOfFile(
                           m_hMapFile,          // handle to map object
                           FILE_MAP_ALL_ACCESS, // read/write permission
                           0,
                           0,
                           0);

   if (m_pDataBuf == NULL)
      {
      CloseHandle(m_hMapFile);
      m_hMapFile = NULL;
      throw Exception("error creating map view of memory mapped file");
      }

   // initialize it with zeroes
   memset((void*)m_pDataBuf, 0, nTotalBytes);

   return str;
}
//------------------------------------------------------------------------------

//------------------------------------------------------------------------------
/// cleans up shared memory
//------------------------------------------------------------------------------
void __fastcall TMPlugin::DeleteMemoryMappedFile()
{
   if (m_pDataBuf)
      {
      try
         {
         UnmapViewOfFile(m_pDataBuf);
         }
      catch (...)
         {
         }
      m_pDataBuf = NULL;
      }
   if (m_hMapFile)
      {
      try
         {
         CloseHandle(m_hMapFile);
         }
      catch (...)
         {
         }
      m_hMapFile = NULL;
      }
}
//------------------------------------------------------------------------------

//------------------------------------------------------------------------------
/// copies user data from data pointers to internal vectors
//------------------------------------------------------------------------------
void __fastcall TMPlugin::SetUserData(double* lpData, unsigned int nChannels, unsigned int nValues, bool bOutput)
{
   EnterCriticalSection(&m_csLock);

   try
      {
      // check dimensions
      if (nValues != m_mps.nUserDataSize)
         throw Exception("a matrix with " + IntToStr((int)m_mps.nUserDataSize) + " rows must be specified");

      if (!lpData)
         throw Exception("bad data read pointer");

      if (bOutput)
         {
         if (nChannels != m_mps.nOutChannels)
            throw Exception("a matrix with " + IntToStr((int)m_mps.nOutChannels) + " columns must be specified");

         // NOTE: data are non-interleaved in pData!!!
         for (unsigned int nChannel = 0; nChannel < nChannels; nChannel++)
            for (unsigned int nValue = 0; nValue < m_mps.nUserDataSize; nValue++)
               m_vvfOutUserData[nChannel][nValue] = (float)*lpData++;
         }
      else
         {
         if (nChannels != m_mps.nInChannels)
            throw Exception("a matrix with " + IntToStr((int)m_mps.nInChannels) + " columns must be specified");
         // NOTE: data are non-interleaved in pData!!!
         for (unsigned int nChannel = 0; nChannel < nChannels; nChannel++)
            for (unsigned int nValue = 0; nValue < m_mps.nUserDataSize; nValue++)
               m_vvfInUserData[nChannel][nValue] = (float)*lpData++;
         }
      }
   __finally
      {
      LeaveCriticalSection(&m_csLock);
      }
}
//------------------------------------------------------------------------------

//------------------------------------------------------------------------------
/// creates a copy of internal user data vector and retuns a reference to the copy
//------------------------------------------------------------------------------
std::vector<std::valarray<float> >& __fastcall TMPlugin::GetUserData(bool bOutput)
{
   EnterCriticalSection(&m_csLock);
   // create a copy of requested user data
   try
      {
      if (bOutput)
         m_vvfTmpData = m_vvfOutUserData;
      else
         m_vvfTmpData = m_vvfInUserData;
      }
   __finally
      {
      LeaveCriticalSection(&m_csLock);
      }
   return m_vvfTmpData;
}
//------------------------------------------------------------------------------

