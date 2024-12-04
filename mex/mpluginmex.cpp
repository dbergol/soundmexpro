//------------------------------------------------------------------------------
/// \file mpluginmex.cpp
/// \author Berg
/// \brief main file for mpluginmex.dll
///
/// Project SoundMexPro
/// Module  mpluginmex
///
/// Implementation of a MATLAB plugin as MEX file. The 'primary' SoundDllPro.dll
/// uses the class TMPlugin to create a second MATLAB process for bufferwise DSP
/// processing. This second MATLAB process loads this (!) MEX file with multiple
/// arguments descriped in MPLUGIN.CPP. SoundDllPro.dll creates shared memory and
/// synchronisation events that are accessed by this MEX as well to share data
/// and synchronise read/write access to the audio data.
/// For a more detailed description of the general strategy see MPLUGIN.CPP in
/// SoundDllPro.dll
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

#include <stdio.h>
#include <windows.h>
#include "mex.h"
/// shared definitions
#include "mplugin_defs.h"

/// define snprintf for Microsoft ...
#ifdef _MSC_VER 
   #define snprintf sprintf_s
#endif


//------------------------------------------------------------------------------
// helper define to clear a string buffer allocated with mxArrayToString
//------------------------------------------------------------------------------
#define TRYMXFREENULL(p) \
   {\
   try \
     { \
     if (p) \
        mxFree(p); \
     p = NULL; \
     } \
   catch (...) {p = NULL;} \
   }
//------------------------------------------------------------------------------

//------------------------------------------------------------------------------
// helper define to clear a data buffer allocated with mxCreateDoubleMatrix
//------------------------------------------------------------------------------
#define TRYMXDESTROYARRAYNULL(p) \
   {\
   try \
     { \
     if (p) \
        mxDestroyArray(p); \
     p = NULL; \
     } \
   catch (...) {p = NULL;} \
   }
//------------------------------------------------------------------------------

//------------------------------------------------------------------------------
// enums for arrays
//------------------------------------------------------------------------------
enum {
   MX_INPUT_AUDIO_IN = 0,
   MX_INPUT_AUDIO_OUT,
   MX_INPUT_USER_IN,
   MX_INPUT_USER_OUT,
   MX_INPUT_LAST
   };
enum {
   MX_OUTPUT_AUDIO_IN = 0,
   MX_OUTPUT_AUDIO_OUT,
   MX_OUTPUT_USER_IN,
   MX_OUTPUT_USER_OUT,
   MX_OUTPUT_LAST
   };

enum {
   STRING_NUM_GUID = 0,
   STRING_NUM_INITCMD,
   STRING_NUM_SCRIPTCMD,
   STRING_NUM_LAST
   };
//------------------------------------------------------------------------------
char lpszBuf[1024];

//------------------------------------------------------------------------------
// helper function checking, if an mxArray is a scalar value
//------------------------------------------------------------------------------
bool mxIsScalarDouble(const mxArray *phs)
{
   if (!mxIsDouble(phs))
      return false;
   if (mxGetM(phs) != 1 || mxGetN(phs) != 1)
      return false;
   return true;
}
//------------------------------------------------------------------------------

//------------------------------------------------------------------------------
/// helper function printing a message MATLAB and additionall to a buffer or
/// to OutputDebugString (if buffer is invalid)
//------------------------------------------------------------------------------
void WriteErrorMessage(const char* lpszDest, LPCSTR lpcsz)
{
   try
      {
      mexPrintf("\n%s\n", lpcsz);
      // if (!lpszDest)
      // always write ODS for easier debugging...
      OutputDebugString(lpcsz);
      // we do not really now how large mapped file is here, but we do not
      // expect errors longer tha 1000 and we always have much more space
      // available than 1000 charaters!!
      if (!!lpcsz)
         snprintf((char*)lpszDest, 1000,"%s", lpcsz);
      }
   catch(...)
      {
      }
}
//------------------------------------------------------------------------------

//------------------------------------------------------------------------------
// ***** MAIN MEX-FUNCTION *******
//------------------------------------------------------------------------------
void mexFunction(int nlhs, mxArray *plhs[], int nrhs, const mxArray *prhs[])
{
   mexSetTrapFlag(1);
   bool bError                = true;
   HANDLE hMapFile            = NULL;
   LPCTSTR pBuf               = NULL;

   // create all  needed arrays and initialize them with NULL
   HANDLE hIPCEvent[IPC_EVENT_LAST];
   for (int i = 0; i < IPC_EVENT_LAST; i++)
      hIPCEvent[i] = NULL;

   mxArray *mxaInput[MX_INPUT_LAST];
   mxArray *mxaOutput[MX_OUTPUT_LAST];
   for (int i = 0; i < MX_INPUT_LAST; i++)
      mxaInput[i] = NULL;
   for (int i = 0; i < MX_OUTPUT_LAST; i++)
      mxaOutput[i] = NULL;

   char *lpsz[STRING_NUM_LAST];
   for (int i = 0; i < STRING_NUM_LAST; i++)
      lpsz[i] = NULL;

   try
      {
      // 1. check arguments
      // GUID, initcommand, scriptcommand, inchannels, outchannels, samples, userdatasize
      if (nrhs != 7)
         throw ("wrong number of arguments (GUID, initialcommand, scriptcommand, inchannels, outchannels, samples, userdatasize");
      int i;
      for (i = 0; i < STRING_NUM_LAST; i++)
         {
         if (!mxIsChar(prhs[i]))
            throw ("first three args must be strings");
         lpsz[i] = mxArrayToString(prhs[i]);
         if (!lpsz[i])
            throw ("error allocating memory");
         if (i != STRING_NUM_INITCMD &&  !strlen(lpsz[i]))
            throw ("empty string passed");
         }
         
      if (!mxIsScalarDouble(prhs[i]))
         throw ("inchannels must be a scalar double value");
      int nNumChannelsI = (int)mxGetScalar(prhs[i]);
      if (nNumChannelsI < 0)
         throw ("inchannels must be a scalar double value >= 0");
      i++;
      if (!mxIsScalarDouble(prhs[i]))
         throw ("outchannels must be a scalar double value");
      int nNumChannelsO = (int)mxGetScalar(prhs[i]);
      if (nNumChannelsO < 0)
         throw ("outchannels must be a scalar double value >= 0");
      i++;
      if (!mxIsScalarDouble(prhs[i]))
         throw ("samples must be a scalar double value");
      int nSamples = (int)mxGetScalar(prhs[i]);
      if (nSamples <= 0)
         throw ("samples must be a scalar double value > 0");
      i++;
      if (!mxIsScalarDouble(prhs[i]))
         throw ("userdatasize must be a scalar double value");
      int nUserdataSize = (int)mxGetScalar(prhs[i]);
      if (nUserdataSize <= 0)
         throw ("userdatasize must be a scalar double value > 0");

      int nNumBytes = (nSamples + nUserdataSize) * (nNumChannelsI + nNumChannelsO)* sizeof(float);

      // 2. get access to memory mapped file (do this ASAP to write error messages to it)!
      snprintf(lpszBuf, 1024, "%s",  lpsz[0]);
      hMapFile = OpenFileMapping(FILE_MAP_ALL_ACCESS, FALSE, lpszBuf);
      if (hMapFile == NULL)
         throw ("Plugin-Mex cannot access memory mapped file");

      pBuf = (LPTSTR) MapViewOfFile(hMapFile,
                                    FILE_MAP_ALL_ACCESS,
                                    0,
                                    0,
                                    0
                                    );
      if (pBuf == NULL)
         {
         CloseHandle(hMapFile);
         hMapFile = NULL;
         throw ("cannot get map view of memory mapped file");
         }

      // 2. allocate mxArrays for input value(s) (if no channels allocate only 0x0 matrices)
      mxaInput[MX_INPUT_AUDIO_IN]   = mxCreateDoubleMatrix(nNumChannelsI ? nSamples : 0,        nNumChannelsI, mxREAL);
      mxaInput[MX_INPUT_AUDIO_OUT]  = mxCreateDoubleMatrix(nNumChannelsO ? nSamples : 0,        nNumChannelsO, mxREAL);
      mxaInput[MX_INPUT_USER_IN]    = mxCreateDoubleMatrix(nNumChannelsI ? nUserdataSize : 0, nNumChannelsI, mxREAL);
      mxaInput[MX_INPUT_USER_OUT]   = mxCreateDoubleMatrix(nNumChannelsO ? nUserdataSize : 0, nNumChannelsO, mxREAL);
      for (int i = 0; i < MX_INPUT_LAST; i++)
         {
         if (!mxaInput[i])
            throw ("error allocating memory");
         }

      // 3. create named events non-signaled and auto-resetting
      for (int i = 0; i < IPC_EVENT_LAST; i++)
         {
         snprintf(lpszBuf, 1024, "%s%d",  lpsz[0], i);
         hIPCEvent[i]    = OpenEvent(EVENT_ALL_ACCESS, false, lpszBuf);
         if (!hIPCEvent[i])
            throw ("error creating IPC events");
         }

      // was an init command specified?
      if (strlen(lpsz[STRING_NUM_INITCMD]))
         {
         // alloc data for input (InChannels, OutChannels, NumSamples)
         mxArray *mxaStartInput[3]  = {NULL, NULL, NULL};
         // pointer for output (success flag)
         mxArray *mxaStartOutput    = NULL;
         try
            {
            for (int i = 0; i < 3; i++)
               {
               mxaStartInput[i]  = mxCreateDoubleMatrix(1, 1, mxREAL);
               if (!mxaStartInput[i])
                  throw ("error allocating memory");
               double *lpd = mxGetPr(mxaStartInput[i]);
               if (!lpd)
                  throw ("error accessing memory");
               if (i == 0)
                  *lpd = (double)nNumChannelsI;
               else if (i == 1)
                  *lpd = (double)nNumChannelsO;
               else
                  *lpd = (double)nSamples;
               }

            if (!!mexCallMATLAB(1, &mxaStartOutput, 3, mxaStartInput, lpsz[STRING_NUM_INITCMD]))
               throw ("an error occurred with init command");
            if (!mxaStartOutput  || !mxIsScalarDouble(mxaStartOutput))
               throw ("invalid return value from init command");
            if (mxGetScalar(mxaStartOutput) != 1)
               throw ("init command returned an error value");
            }
         catch (...)
            {
            // cleanup
            for (int i = 0; i < 3; i++)
               TRYMXDESTROYARRAYNULL(mxaStartInput[i]);
            TRYMXDESTROYARRAYNULL(mxaStartOutput);
            throw;
            }
         for (int i = 0; i < 3; i++)
             TRYMXDESTROYARRAYNULL(mxaStartInput[i]);
         TRYMXDESTROYARRAYNULL(mxaStartOutput);
         }

      // now set init event: startup is done
      if (!SetEvent(hIPCEvent[IPC_EVENT_INIT]))
         throw ("error setting init event");

      // now an endless loop waiting for signals of events:
      // 'process':  data to process can be read from shared memory
      // 'exit':     stop request: leave loop
      bool bContinue = true;
      int nChannel, nSample;
      double*  lp8;
      float*   lpf;
      while (bContinue)
         {
         // wait for mutex signals (only for process or exit: first two handles in hMutex)
         DWORD nWaitResult = WaitForMultipleObjects(2, hIPCEvent, false, INFINITE);
         switch (nWaitResult)
            {
            case (WAIT_OBJECT_0 + IPC_EVENT_PROCESS):
               // now we are allowed to touch the shared memory!
               // write data
               lpf = (float*)pBuf;
               // copy audio data
               if (nNumChannelsI)
                  {
                  lp8 = mxGetPr(mxaInput[MX_INPUT_AUDIO_IN]);
                  if (!lp8 || !lpf)
                     throw ("error accessing script input data (1)");
                  for (nChannel = 0; nChannel < nNumChannelsI; nChannel++)
                     for (nSample = 0; nSample < nSamples; nSample++)
                        *lp8++ = *lpf++;
                  }
               if (nNumChannelsO)
                  {
                  lp8 = mxGetPr(mxaInput[MX_INPUT_AUDIO_OUT]);
                  if (!lp8 || !lpf)
                     throw ("error accessing script input data (2)");
                  for (nChannel = 0; nChannel < nNumChannelsO; nChannel++)
                     for (nSample = 0; nSample < nSamples; nSample++)
                        *lp8++ = *lpf++;
                  }
               // copy user data
               if (nNumChannelsI)
                  {
                  lp8 = mxGetPr(mxaInput[MX_INPUT_USER_IN]);
                  if (!lp8 || !lpf)
                     throw ("error accessing script input data (3)");
                  for (nChannel = 0; nChannel < nNumChannelsI; nChannel++)
                     for (nSample = 0; nSample < nUserdataSize; nSample++)
                        *lp8++ = *lpf++;
                  }
               if (nNumChannelsO)
                  {
                  lp8 = mxGetPr(mxaInput[MX_INPUT_USER_OUT]);
                  if (!lp8 || !lpf)
                     throw ("error accessing script input data (4)");
                  for (nChannel = 0; nChannel < nNumChannelsO; nChannel++)
                     for (nSample = 0; nSample < nUserdataSize; nSample++)
                        *lp8++ = *lpf++;
                  }

               // clear input
               ZeroMemory((void*)pBuf, nNumBytes);

               // call MATLAB
               if (!!mexCallMATLAB(MX_OUTPUT_LAST, mxaOutput, MX_INPUT_LAST, mxaInput, lpsz[STRING_NUM_SCRIPTCMD]))
                  throw ("error in script command");

               // check dimensions of result data
               for (int i = 0; i < MX_OUTPUT_LAST; i++)
                  {
                  if (!mxaOutput[i])
                     throw ("error accessing script output data (1)");
                  if (  mxGetM(mxaOutput[i]) != mxGetM(mxaInput[i])
                     || mxGetN(mxaOutput[i]) != mxGetN(mxaInput[i])
                     )
                     throw ("script returned vector(s) with wrong dimension(s)");
                  }

               // copy processed data back
               lpf = (float*)pBuf;
               if (nNumChannelsI)
                  {
                  lp8 = mxGetPr(mxaOutput[MX_OUTPUT_AUDIO_IN]);
                  if (!lp8 || !lpf)
                     throw ("error accessing script output data (2)");
                  for (nChannel = 0; nChannel < nNumChannelsI; nChannel++)
                     {
                     for (nSample = 0; nSample < nSamples; nSample++)
                        {
                        *lpf++ = (float)*lp8++;
                        }
                     }
                  }
               if (nNumChannelsO)
                  {
                  lp8 = mxGetPr(mxaOutput[MX_OUTPUT_AUDIO_OUT]);
                  if (!lp8 || !lpf)
                     throw ("error accessing script output data (2)");
                  for (nChannel = 0; nChannel < nNumChannelsO; nChannel++)
                     {
                     for (nSample = 0; nSample < nSamples; nSample++)
                        {
                        *lpf++ = (float)*lp8++;
                        }
                     }
                  }

               if (nNumChannelsI)
                  {
                  lp8 = mxGetPr(mxaOutput[MX_OUTPUT_USER_IN]);
                  if (!lp8 || !lpf)
                     throw ("error accessing script output data (2)");
                  for (nChannel = 0; nChannel < nNumChannelsI; nChannel++)
                     {
                     for (nSample = 0; nSample < nUserdataSize; nSample++)
                        {
                        *lpf++ = (float)*lp8++;
                        }
                     }
                  }
               if (nNumChannelsO)
                  {
                  lp8 = mxGetPr(mxaOutput[MX_OUTPUT_USER_OUT]);
                  if (!lp8 || !lpf)
                     throw ("error accessing script output data (2)");
                  for (nChannel = 0; nChannel < nNumChannelsO; nChannel++)
                     {
                     for (nSample = 0; nSample < nUserdataSize; nSample++)
                        {
                        *lpf++ = (float)*lp8++;
                        }
                     }
                  }


               // clear allocated output data! Must be done after each processing loop!
               for (int i = 0; i < MX_OUTPUT_LAST; i++)
                  {
                  TRYMXDESTROYARRAYNULL(mxaOutput[i]);
                  }

               // set 'done' event
               if (!SetEvent(hIPCEvent[IPC_EVENT_DONE]))
                  throw ("error setting done event");
               break;
                  
            // all others (abandoned, exit, error): break. We do not handle errors here
            default:
               bContinue = false;
               break;
            }
         }
      bError = false;
      }
   // catch string exception
   catch (LPCSTR lpsz)
      {
      WriteErrorMessage(pBuf, lpsz);
      }
   // catch unknownn exceptions
   catch (...)
      {
      WriteErrorMessage(pBuf, "An unknown C++-exception occurred");
      }

   // free data
   for (int i = 0; i < STRING_NUM_LAST; i++)
      TRYMXFREENULL(lpsz[i]);
   for (int i = 0; i < MX_INPUT_LAST; i++)
      TRYMXDESTROYARRAYNULL(mxaInput[i]);
   for (int i = 0; i < MX_OUTPUT_LAST; i++)
      TRYMXDESTROYARRAYNULL(mxaOutput[i]);

   // signal error by setting error event
   if (bError && hIPCEvent[IPC_EVENT_ERROR])
      // don't care about return value from setting error event: we can't do anything about it...
      SetEvent(hIPCEvent[IPC_EVENT_ERROR]);

   // cleanup shared memory object
   if (pBuf)
      UnmapViewOfFile(pBuf);
   pBuf = NULL;
   if (hMapFile)
      CloseHandle(hMapFile);
   hMapFile = NULL;

   SetEvent(hIPCEvent[IPC_EVENT_TERMINATED]);
   // cleanup IPC events
   for (int i = 0; i < IPC_EVENT_LAST; i++)
      {
      if (hIPCEvent[i] != NULL)
         {
         CloseHandle(hIPCEvent[i]);
         hIPCEvent[i] = NULL;
         }
      }

   mexSetTrapFlag(0);
}
//------------------------------------------------------------------------------

