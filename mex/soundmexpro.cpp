//------------------------------------------------------------------------------
/// \file soundmexpro.cpp
/// \author Berg
/// \brief main file for soundmexpro.dll
///
/// Project SoundMexPro
/// Module  soundmexpro.dll
///
/// soundmexpro.cpp is the interface between MATLABs/Octaves mex interface and
/// the 'main' SoundMexPro DLL SoundDllPro.dll. The communication between this
/// MEX-Dll and SoundMexPro is done through interprocess communication to allow
/// usage from 32bit AND 64bit processes, and to separate the calling process from
/// SoundMexPro.
/// For this purpose a 32bit interprocess communication process executable named
/// SMPIPC.EXE is started. This EXE loads the main dll SoundDllPro.dll and offers
/// interprocess communication through named events and memory mapped files.
/// This parser 'translates' mxArray passed from MATLAB to the MEX-DLL as arguments
/// to the command line syntax of SoundDllPro, writes it to the shared memory and
/// signals through an event, that SMPIPC.EXE may read it. Then it wits for a signal
/// to read return values and/or error messages from shared memory again.
/// Additionally it copies data to the shared memory, if matrices are passed +
/// sizing ionformation (number of rows and columns)
/// Finally it translates returned values back to mxArrays for returning them
/// back to MATLAB/Octave.
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
#include <vector>
#include <stdio.h>
#include <stdlib.h>
#include <string>
#include <sstream>
#include <math.h>
#include <windows.h>
#include <windowsx.h>
#include <float.h>

#include "mex.h"
#include <mmsystem.h>

#include "soundmexpro.h"
#include "soundmexpro_defs.h"
#include "soundmexpro_ipc.h"



// #define BETAVERSION
#define msg(p) MessageBox(0,p,"",0);
//------------------------------------------------------------------------------
// Definitions
//------------------------------------------------------------------------------

#ifdef __BORLANDC__
   #define _strcmpi strcmpi
#endif



// NOTE: resetted to 0 for 2.4.0.0
// NOTE: resetted to 0 for 2.6.0.0 for compiler change
// NOTE: resetted to 0 for 2.8.0.0 for python support
#define SOUNDMEX_SUBVER      1
#define SOUNDMEX_ERR_STRALLOC   throw SOUNDMEX_Error("Fatal string allocation error")



//------------------------------------------------------------------------------
// tYPES AND ENUMS
//------------------------------------------------------------------------------


//------------------------------------------------------------------------------
// Global Variables
//------------------------------------------------------------------------------
int            g_nShowError   = 1;
std::string    g_strLastError = "no error";
SMPIPCProcess  g_SMPIPC;

//------------------------------------------------------------------------------
// Local prototypes
//------------------------------------------------------------------------------
void SecureFpu();
void ExitFcn(void);
std::string GetCurrentMatlabPath();


//------------------------------------------------------------------------------
// ***** MAIN MEX-FUNCTION *******
//------------------------------------------------------------------------------
void mexFunction(int nlhs, mxArray *plhs[], int nrhs, const mxArray *prhs[])
{

   // Mapfile object for passing double values from MATLAB to sounddllpro
   MapFile     mfData;

   SecureFpu();
   
   // tell MATLAB to return control after errors in calls to mexCallMATLAB within this MEX! 
   mexSetTrapFlag(1);   

   // Initialize first return value only (ANS)
   plhs[0] = mxCreateDoubleMatrix(1, 1, mxREAL);


   std::string strError;
   std::string strHelpCommand;

   static int nCommandCounter = 0;
   // set exit function
   mexAtExit(ExitFcn);

   int         iDim           = 1;
   int         iReturn        = SOUNDDLL_RETURN_ERROR;

   // 1. No argument: show help an return
   if (nrhs == 0)
      {
      // usage("");
      mexPrintf("\n---------------------------------------------------------------------------------------\n");
      mexPrintf(SOUNDMEX_HELP);
      mexPrintf("\n---------------------------------------------------------------------------------------\n");
      // set  return value to success (1)
      mxGetPr(plhs[0])[0] = (double)1;
      return;
      }

   std::string strCmd;
   std::vector<std::string>   vsRet;
   std::vector<std::string>   vsValues;

   char *lpszCommand    = mxArrayToString(prhs[0]);
   // copy command name to string
   std::string strCommand = lpszCommand;

   // free buffer
   if (lpszCommand)
      {
      mxFree(lpszCommand);
      lpszCommand = NULL;
      }

   try
      {
      
      // check if first return value was created sucessfully!
      if ( NULL==plhs[0] )
         throw SOUNDMEX_Error("Can't create return value");

      // set default return value to 'error' (0)
      mxGetPr(plhs[0])[0] = (double)0;
                        
      if ( strCommand.length() == 0 )
         throw SOUNDMEX_Error("Can't read Command name");

      nCommandCounter++;
      if (nCommandCounter>1)
         throw SOUNDMEX_Error("Error in command " + strCommand + ": sounddllpro busy: asynchroneous command call failed!", SOUNDDLL_RETURN_BUSY);

      // at first we write the command followed by the delimiter
      strCmd = SOUNDDLLPRO_STR_COMMAND "=" + strCommand + ";";

      bool bQuiet       = false;
      bool bQuietInit   = false;

      //-----------------------------------------------------------------
      // special handling for help: one or no argument
      // Additionally append magic argument 'soundmexcall=1'
      //-----------------------------------------------------------------
      if (  0 == _strcmpi(strCommand.c_str(), SOUNDDLLPRO_CMD_HELP)
         || 0 == _strcmpi(strCommand.c_str(), SOUNDDLLPRO_CMD_HELPA)
         )
         {
         if (nrhs == 2)
            {
            if (!mxIsChar(prhs[1]))
               throw SOUNDMEX_Error("command must be string");
            char *lpszStrPar = mxArrayToString(prhs[1]);
            if (!lpszStrPar)
               SOUNDMEX_ERR_STRALLOC;

            strHelpCommand = lpszStrPar;
            strCmd = strCmd + strCommand + "=" + strHelpCommand + ";" + SOUNDDLLPRO_PAR_SOUNDMEXCALL + "=1;";
            mxFree(lpszStrPar);
            }
         else if (nrhs == 1)
            strCmd = strCmd + ";" + SOUNDDLLPRO_PAR_SOUNDMEXCALL + "=1;";
         else
            throw SOUNDMEX_Error("invalid number of parameters (none or command to get help about)");
         }
      //-----------------------------------------------------------------
      // all other commands
      //-----------------------------------------------------------------
      else
         {
         // all commands other than 'help' must have nrhs == 1 + 2n
         // (commandname + pairs of variable and value)
         if (nrhs % 2 != 1)
            throw SOUNDMEX_Error("invalid number of parameters (2n+1)");

         double   dValue, dDummy;
         bool     bDataField        = false;
         bool     bUIHandle         = false;

         std::string strName, strValue;
         // now write all further pairs with checking
         int nNumPairs = (nrhs-1)/2;
         const mxArray *mxa;
         for (int i = 0; i < nNumPairs; i++)
            {
            // start with parameter name
            mxa = prhs[2*i+1];
            bUIHandle = false;
            if (!mxIsChar(mxa))
               throw SOUNDMEX_Error("parameter names must be strings");
            char *lpszStrPar = mxArrayToString(mxa);
            if (!lpszStrPar)
               SOUNDMEX_ERR_STRALLOC;

            // special handling of value 'handle' for setbutton
            bUIHandle = (!_strcmpi(strCommand.c_str(), SOUNDDLLPRO_CMD_SETBUTTON) && !strcmp(lpszStrPar, SOUNDDLLPRO_PAR_HANDLE));
            // special handling of value 'data' vectors
            bDataField = !_strcmpi(lpszStrPar, SOUNDDLLPRO_PAR_DATA);
            // special handling of value 'quiet' 
            bQuiet     = !_strcmpi(lpszStrPar, SOUNDDLLPRO_PAR_QUIET);
            strName = lpszStrPar;
            mxFree(lpszStrPar);

            // then access the value
            mxa = prhs[2*(i+1)];
            // we have a button handle. Convert it boundsrect and caption
            if (bUIHandle)
               {
               // scalar check removed 2015-07-23: newer MATLAB versions return an array, but
               // GetButtonWindowProperties function handles that without changes!
               // must be single value (no matrix)

               // if (!IsScalarDouble(mxa))
               //    throw SOUNDMEX_Error("Button handle must be single value (no matrix)!!");

               // we have to call 'drawnow' in matlab, don't really know why, but
               // otherwise an immediate start of playfile with marking
               // will fail. Seems that matlab does not poll the message loop very well
               mexEvalString("drawnow");

               // retrieve boundsrect and caption of button
               RECT rc;
               std::string strCaption;
               GetButtonWindowProperties((mxArray**)&mxa, rc, strCaption);
               // set empty handle: sounddllpro then determines window handle
               // itself from boundsrect and caption
               strValue = "";
               // write boundsrect and caption
               strCmd = strCmd + SOUNDDLLPRO_PAR_LEFT "=" + IntegerToStr(rc.left) + ";";
               strCmd = strCmd + SOUNDDLLPRO_PAR_WIDTH "=" + IntegerToStr(rc.right) + ";";
               strCmd = strCmd + SOUNDDLLPRO_PAR_TOP "=" + IntegerToStr(rc.top) + ";";
               strCmd = strCmd + SOUNDDLLPRO_PAR_HEIGHT "=" + IntegerToStr(rc.bottom) + ";";
               strCmd = strCmd + SOUNDDLLPRO_PAR_NAME "=" + strCaption + ";";
               }
            else if (bQuiet)
               {
               if (!IsScalarDouble(mxa))
                  throw SOUNDMEX_Error("value must be scalar");                  
               dValue = mxGetScalar(mxa);
               bQuietInit = (dValue == 1.0);
               strValue = DoubleToStr(mxGetScalar(mxa)).c_str();
               }
            // we allow strings....
            else if (mxIsChar(mxa))
               {
               char *lpszStrPar = mxArrayToString(mxa);
               if (!lpszStrPar)
                  SOUNDMEX_ERR_STRALLOC;
               strValue = lpszStrPar;
               mxFree(lpszStrPar);
               }
            // ... and scalar doubles
            else if (IsScalarDouble(mxa))
               {
               // check if its an integer or a float!
               dValue = mxGetScalar(mxa);
               if (modf(dValue, &dDummy) == 0.0)
                  strValue = IntegerToStr((__int64)mxGetScalar(mxa)).c_str();
               else
                  strValue = DoubleToStr(mxGetScalar(mxa)).c_str();
               }
            // we allow double arrays
            else if (mxIsDouble(mxa))
               {
               // must NOT be a sparse matrix
               if (mxIsSparse(mxa))
                  throw SOUNDMEX_Error("sparse matrices not supported");
               
               // special for data vectors: field must be named 'data'!
               if (bDataField)
                  {
                  // create MemMapped file
                  DWORD dwSize = DWORD(mxGetM(mxa) * mxGetN(mxa) * sizeof(double));
                  // write name of MemMapped file
                  strValue = CreateGUID();
                  SMPCreateFileMapping(mfData, strValue.c_str(), dwSize);
                  // copy data to it
                  CopyMemory(mfData.pData, (void*)mxGetPr(mxa), dwSize);
                  // write additional colcount (!): number of samples
                  strCmd = strCmd + SOUNDDLLPRO_PAR_SAMPLES "=" + IntegerToStr(mxGetM(mxa)) + ";";
                  // write additional rowcount: number of channels
                  strCmd = strCmd + SOUNDDLLPRO_PAR_CHANNELS "=" + IntegerToStr(mxGetN(mxa)) + ";";
                  }
               // otherwise only row vectors allowed
               else
                  {
                  if (mxGetM(mxa) > 1)
                     throw SOUNDMEX_Error("value arrays must be row vectors");
                  // take care that no loooong (audio) vector is passed by accident
                  if (mxGetN(mxa) > 100000)
                     throw SOUNDMEX_Error("value arrays must be row vectors with not more than 1000 values");
                  strValue = "";
                  for (int i = 0; i < mxGetN(mxa); i++)
                     {
                     dValue = mxGetPr(mxa)[i];
                     if (modf(dValue, &dDummy) == 0.0)
                        strValue += IntegerToStr((__int64)dValue).c_str();
                     else
                        strValue += DoubleToStr(dValue).c_str();
                     if (i < mxGetN(mxa) - 1)
                        strValue += ",";
                     }
                  }
               }
            // cell arrays with strings
            else if (mxIsCell(mxa))
               {
               size_t nNumCells = mxGetNumberOfElements(mxa);
               strValue.clear();
               for (int i = 0; i < nNumCells; i++)
                  {
                  const mxArray *mxaCell = mxGetCell(mxa, i);
                  if (!mxIsChar(mxaCell))
                     throw SOUNDMEX_Error("cell arrays only supported with string content");
                  char *lpszStrPar = mxArrayToString(mxaCell);

                  if (!lpszStrPar)
                     SOUNDMEX_ERR_STRALLOC;
                  // add values quoted: may contain a comma
                  strValue = strValue + "\"" + lpszStrPar + "\"";
                  mxFree(lpszStrPar);
                  if (i < nNumCells - 1)
                     strValue = strValue + ",";
                  }
               }
            else
               throw SOUNDMEX_Error("values must be strings, doubles, row vectors, cell arrays or data matrices named 'data'!");

            strCmd = strCmd + strName + "=" + strValue + ";";
            }
         }

      if (!g_SMPIPC.IPCProcessActive())
         g_SMPIPC.IPCInit();

      //--------------------------------------------------------------------------------
      // call external command
      //--------------------------------------------------------------------------------
      // all regular DLL-commands
      if (IsDllCommand(strCommand, strHelpCommand))
         {
         // append current MATLAB-Exename and path of calling mfile: needed by MLPlugins!
         if ( 0==_strcmpi(strCommand.c_str(), SOUNDDLLPRO_CMD_INIT))
            {
            char c[2*MAX_PATH];
            ZeroMemory(c, 2*MAX_PATH);
            if (!GetModuleFileName(GetModuleHandle(NULL), c, 2*MAX_PATH-1))
               throw SOUNDMEX_Error("error retrieving MATLAB module name in MEX");
            strCmd += SOUNDDLLPRO_PAR_MATLABEXE "=";
            strCmd += c;
            strCmd += ";";
            strCmd += SOUNDDLLPRO_PAR_PLUGINPATH "=";
            strCmd += GetCurrentMatlabPath();
            strCmd += ";";
            }

         iReturn = SOUNDDLL_RETURN_OK;
         // special: extra calls for recgetdata and plugingetdata to retrieve
         // needed buffer size of memory mapped file to be created
         if (  0==_strcmpi(strCommand.c_str(), SOUNDDLLPRO_CMD_RECGETDATA)
            || 0==_strcmpi(strCommand.c_str(), SOUNDDLLPRO_CMD_PLUGINGETDATA)
            )
            {
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
               __int64 nBufSize, nChannels;
               if (  !TryStrToInteger(GetValue(vsRet, SOUNDDLLPRO_PAR_BUFSIZE), nBufSize)
                  || !TryStrToInteger(GetValue(vsRet, SOUNDDLLPRO_PAR_CHANNELS), nChannels)
                  )
                  throw SOUNDMEX_Error("invalid sizes retrieved from SoundDllPro");
               DWORD dwSize = (int)nBufSize * (int)nChannels * sizeof(float);

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

         // 'regular' command call
         if (iReturn == SOUNDDLL_RETURN_OK)
            {
            // remove trailing colon!!!!
            trim(strCmd, ';');

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

         std::string strTmp = g_SMPIPC.m_mfCmd.pData;
//OutputDebugString(strTmp.c_str());

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
                  mexPrintf("---------------------------------------------------------------------\n");
                  #ifdef BETAVERSION
                  mexPrintf("\t\t\tSoundMexPro %s %s (%d) BETA", GetValue(vsRet, SOUNDDLLPRO_STR_Type).c_str(), GetValue(vsRet, SOUNDDLLPRO_STR_Version).c_str(), SOUNDMEX_SUBVER);
                  // print update error in brackets (if any)
                  if (strlen(strLicValue.c_str()))
                     mexPrintf(" (%s)", strLicValue.c_str());
                  mexPrintf("\n");
                  #else
                  mexPrintf("\t\t\tSoundMexPro %s %s (%d)", GetValue(vsRet, SOUNDDLLPRO_STR_Type).c_str(), GetValue(vsRet, SOUNDDLLPRO_STR_Version).c_str(), SOUNDMEX_SUBVER);
                  // print update error in brackets (if any)
                  if (strlen(strLicValue.c_str()))
                     mexPrintf(" (%s)", strLicValue.c_str());
                  mexPrintf("\n");
                  #endif
                  strLicValue = GetValue(vsRet, SOUNDDLLPRO_STR_Name);
                  if (strlen(strLicValue.c_str()))
                     mexPrintf("\t\t\tLicensed to:\t %s\n", strLicValue.c_str());
                  mexPrintf("---------------------------------------------------------------------\n");
                  }
               // remove all values except Type
               std::string s = GetValue(vsRet, SOUNDDLLPRO_STR_Type);
               vsRet.clear();
               SetValue(vsRet, SOUNDDLLPRO_STR_Type, s.c_str());
               }

            // special for 'getrecdata' and 'plugingetdata': read data from returned
            // memory mapped files
            if (  0==_strcmpi(strCommand.c_str(), SOUNDDLLPRO_CMD_RECGETDATA)
               || 0==_strcmpi(strCommand.c_str(), SOUNDDLLPRO_CMD_PLUGINGETDATA)
               )
               {
               try
                  {
                  // get buffer length
                  __int64 nBufSize64;
                  __int64 nDataPos64;
                  __int64 nChannels64;

                  if (!TryStrToInteger(GetValue(vsRet, SOUNDDLLPRO_PAR_BUFSIZE), nBufSize64) || nBufSize64 < 0)
                     throw SOUNDMEX_Error("invalid buffersize value returned from command: " + GetValue(vsRet, SOUNDDLLPRO_PAR_BUFSIZE));
                  if (nBufSize64 == 0)
                     throw SOUNDMEX_Error("no data available/recorded (buffer size is 0)");

                  if (!TryStrToInteger(GetValue(vsRet, SOUNDDLLPRO_PAR_CHANNELS), nChannels64) || nChannels64 < 0)
                     throw SOUNDMEX_Error("invalid channel number value returned from command: " + GetValue(vsRet, SOUNDDLLPRO_PAR_BUFSIZE));

                  // ... and data position
                  if (0==_strcmpi(strCommand.c_str(), SOUNDDLLPRO_CMD_RECGETDATA))
                     {
                     if (!TryStrToInteger(GetValue(vsRet, SOUNDDLLPRO_PAR_POSITION), nDataPos64))// || nDataPos < 0)
                        throw SOUNDMEX_Error("invalid data position value returned from command: " + GetValue(vsRet, SOUNDDLLPRO_PAR_POSITION));
                     }
                  // access memory mapped file created above
                  float * lpf = (float*)mfData.pData;
                  if (!lpf)
                     throw SOUNDMEX_Error("Internal file mapping error");

                  // all data ok: write return values
                  if (nlhs > 1)
                     {
                     // convert _-int64 to int. No idea why it's necessary, otherwise we get neverending for loops
                     // at least in MATLAB 2017
                     int nBufSize   = (int)nBufSize64;
                     int nChannels  = (int)nChannels64;
                     // create matrix
                     plhs[1] = mxCreateDoubleMatrix(nBufSize, nChannels, mxREAL);
                     if (!plhs[1])
                        throw SOUNDMEX_Error("Error allocating data in Matlab");

                     // write data
                     double* lp = mxGetPr(plhs[1]);
                     for (int j = 0; j < nChannels; j++)
                        {
                        // loop necessary for float to double conversion!
                        for (int i = 0; i < nBufSize; i++)
                           {
                           *lp++ = *lpf++;
                           }
                        }


                     // write data position
                     if ((0==_strcmpi(strCommand.c_str(), SOUNDDLLPRO_CMD_RECGETDATA)) && nlhs > 2)
                        {
                        plhs[2] = mxCreateDoubleMatrix(1, 1, mxREAL);
                        *mxGetPr(plhs[2]) = (double)nDataPos64;
                        }
                     }
                  }
               catch (...)
                  {
                  for (int i = 1; i < nlhs; i++)
                     {
                     plhs[i] = mxCreateDoubleMatrix(0, 0, mxREAL);
                     }
                  throw;
                  }
               }
            else if (  0 == _strcmpi(strCommand.c_str(), SOUNDDLLPRO_CMD_HELP)
               || 0 == _strcmpi(strCommand.c_str(), SOUNDDLLPRO_CMD_HELPA)
               )
               {
               }
            // all other commands (not recgetdata or plugingetdata): standard processing of return values
            else
               {
               // for 'version' we append the MEX subversion to the value returned from DLL in brackets
               if ( 0==_strcmpi(strCommand.c_str(), SOUNDDLLPRO_CMD_VERSION))
                  {
                  #ifdef BETAVERSION
                  SetValue(vsRet, SOUNDDLLPRO_STR_Version, (GetValue(vsRet, SOUNDDLLPRO_STR_Version) + " (" + IntegerToStr(SOUNDMEX_SUBVER) + ") BETA").c_str());
                  #else
                  SetValue(vsRet, SOUNDDLLPRO_STR_Version, (GetValue(vsRet, SOUNDDLLPRO_STR_Version) + " (" + IntegerToStr(SOUNDMEX_SUBVER) + ")").c_str());
                  #endif
                  }


               // limit number of return values to number of requested (assignable)
               // return values
               // NOTE: nlhs is
               //  - 0 if NO return value requested (but success flag set below anyway)
               //  - >0 otherwise.
               // That means that we handle
               //  - NOT A SINGLE return value at all if nlhs<2
               //  - (nlhs-1) return values else
               size_t nRet = (nlhs > 1) ? (nlhs-1) : 0;
               // limit it, if not enough values returned from DLL
               if (nRet > vsRet.size())
                  nRet = vsRet.size();

               // go through values in return string
               std::string sTmp;
               for (size_t i = 0; i < nRet; i++)
                  {
                  // extract value 'y' from 'x=y' string
                  sTmp = ValueFromString(vsRet[i]);
                  ParseValues(sTmp, vsValues, ',');
                  int nValSize = (int)vsValues.size();


                  if (!nValSize)
                     {
                     // changed 04.02.2010: empty return values create empty matrix as return value!
                     // throw SOUNDMEX_Error("empty return value returned from command");
                     plhs[1+i] = mxCreateDoubleMatrix(0, 0, mxREAL);
                     continue;
                     }

                  // check, if all are doubles
                  bool bDoubleValues = true;

                  for (int j = 0; j < nValSize; j++)
                     {
                     if (!IsDouble(vsValues[j]))
                        {
                        bDoubleValues = false;
                        break;
                        }
                     }
                  // for "getchannels" and "getactivechannels" we ALWAYS want strings.
                  // workaound added, for a particular blootooth driver, that had pure numbers
                  // as channel names....
                  if (  0==_strcmpi(strCommand.c_str(), SOUNDDLLPRO_CMD_GETCH_ACTIVE)
                     || 0==_strcmpi(strCommand.c_str(), SOUNDDLLPRO_CMD_GETCH)
                     )
                     bDoubleValues = false;
                  // all are doubles: create matrix
                  if (bDoubleValues)
                     {
                     plhs[1+i] = mxCreateDoubleMatrix(nValSize, 1, mxREAL);
                     double* lp = mxGetPr(plhs[1+i]);
                     for (int j = 0; j < nValSize; j++)
                        lp[j] = StrToDouble(vsValues[j].c_str());
                     }
                  // otherwise ...
                  else
                     {
                     plhs[1+i] = mxCreateCellMatrix(nValSize, 1);
                     for (int j = 0; j < nValSize; j++)
                        {
                        mxSetCell(plhs[1+i], j, mxCreateString(vsValues[j].c_str()));
                        }
                     }
                  }
               // if not enough return values returned, we return more empty matrices
               if (nlhs > 0)
                  {
                  for (size_t i = vsRet.size(); i < nlhs-1; i++)
                       plhs[1+i] = mxCreateDoubleMatrix(0, 0, mxREAL);
                  }
               }
            }
         // we have to build all requested output arguments on errors too, here
         // only returning empty double matrix
         else
            {
            for (int i = 1; i < nlhs; i++)
               {
               plhs[i] = mxCreateDoubleMatrix(0, 0, mxREAL);
               }
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
            // write actual value
            if (nlhs > 1)
               {
               plhs[1] = mxCreateDoubleMatrix(1, 1, mxREAL);
               *mxGetPr(plhs[1]) = g_nShowError;
               }
            iReturn = SOUNDDLL_RETURN_OK;
            }
         // - 'getlasterror'
         else if (0==_strcmpi(strCommand.c_str(), SOUNDDLLPRO_CMD_GETLASTERROR))
            {
            if (vsRet.size() > 1)
               throw SOUNDMEX_Error("invalid parameter passed to 'getlasterror'");
            // write last error
            if (nlhs > 1)
               {
               plhs[1] = mxCreateString(g_strLastError.c_str());
               }
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
         bool bShowCommandList = (nrhs == 1);

         // start printing standard
         mexPrintf("\n---------------------------------------------------------------------------------------\n");
         mexPrintf("\t [errocode, ...] = soundmexpro(Name, par1, value1, par2, value2...)\n");
         mexPrintf("\t errocode is 1 on success of command and 0 on any error.\n");
         mexPrintf("---------------------------------------------------------------------------------------\n");

         if (bShowCommandList)
            {
            if (0==_strcmpi(strCommand.c_str(), SOUNDDLLPRO_CMD_HELPA))
               mexPrintf("\nAvailable commands are (alphabetical order):\n");
            else
               mexPrintf("\nAvailable commands are (logical order):\n");

            mexPrintf("\n%s", GetValue(vsRet, SOUNDDLLPRO_CMD_HELP).c_str());
            }
         else
            {
            mexPrintf("\t parameters (Par.>), values, default values (Def.>)\n");
            mexPrintf("\t and additional return values (Ret.>) are:\n\n");
            mexPrintf("%s\n", GetValue(vsRet, SOUNDDLLPRO_CMD_HELP).c_str());
            }
         mexPrintf("\n---------------------------------------------------------------------------------------\n");
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
         mexWarnMsgTxt(sTmp.c_str());
      else
         mexErrMsgTxt(sTmp.c_str());
      }


   // set 'main' return value
   if (NULL != plhs[0])
      {
      if (iReturn == SOUNDDLL_RETURN_OK)
         mxGetPr(plhs[0])[0] = 1.0;
      else
         {
         if (!bErrorDisplayed && g_nShowError > 1)
            {
            std::string sTmp = "Uknown error in command " + strCommand  + "\n";
            mexErrMsgTxt(sTmp.c_str());
            }
         mxGetPr(plhs[0])[0] = 0.0;
         }
      }

   // copy error to last error
   if (!!strError.length())
      g_strLastError = strError;

}
//------------------------------------------------------------------------------

//------------------------------------------------------------------------------
// Exits IPC
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

//------------------------------------------------------------------------------
// Helper function to set floating point error mode
//------------------------------------------------------------------------------
void SecureFpu()
{
#ifndef MCW_EM
   #define MCW_EM 0x003f
#endif
#ifndef MCW_PC
   #define MCW_PC 0x0300
#endif

_control87(PC_64|MCW_EM,MCW_PC|MCW_EM);

}
//------------------------------------------------------------------------------

//------------------------------------------------------------------------------
/// returns current MatlabPath
//------------------------------------------------------------------------------
std::string GetCurrentMatlabPath()
{
   mxArray *rhs[1], *lhs[1];
   char *path;
   size_t lenpath, lenname, n;
   mexCallMATLAB(1, lhs, 0, NULL, "pwd");
   path = mxArrayToString(lhs[0]);
   mxDestroyArray(lhs[0]);
   std::string str = path;
   mxFree(path);
   return str;        
}

