//------------------------------------------------------------------------------
/// \file ConvLibMain.cpp
/// \author Berg
/// \brief Interface for a DLL loaded by VST-Plugin HtVSTConv. This DLL does the
/// number crunching itself. This module is designed to be compilable by different
/// compilers to have various possibilities to get a maximum perfromance
///
/// Project SoundMexPro
/// Module  HtVSTConv.dll
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
#include <windows.h>
#include "HtPartitionedConvolution.h"

//------------------------------------------------------------------------------
/// exported functions for VST plugin
//------------------------------------------------------------------------------
extern "C" {
   __declspec(dllexport) void * ConvInit( unsigned nFragsize,
                                          unsigned nChannels,
                                          unsigned nIrsLen,
                                          const float ** ppfIrs);
}


extern "C" {
   __declspec(dllexport) void ConvExit(void * handle);
}

extern "C" {
   __declspec(dllexport) int ConvProcess( void * handle,
                                          unsigned nFragsize,
                                          unsigned nChannelsIn,
                                          unsigned nChannelsOut,
                                          float * const* ppfDataIn,
                                          float * const* ppfDataOut);
}
//------------------------------------------------------------------------------

//------------------------------------------------------------------------------
/// Initialization
//------------------------------------------------------------------------------
#pragma argsused
void* ConvInit(unsigned nFragsize, unsigned nChannels, unsigned nIrsLen, const float ** ppfIrs)
{

   std::vector<std::vector<float > > vvfData;
   vvfData.resize(2);
   for (unsigned int i = 0; i < nIrsLen; i++)
      {
      vvfData[0].push_back(ppfIrs[0][i]);
      vvfData[1].push_back(ppfIrs[1][i]);
      }


   CHtTransferFunctions tfs;

   tfs.push_back(CHtTransferFunction(0, vvfData[0]));
   tfs.push_back(CHtTransferFunction(1, vvfData[vvfData.size()-1]));
   return new CHtPartitionedConvolution(nFragsize, 2, tfs);

}
//------------------------------------------------------------------------------

//------------------------------------------------------------------------------
/// Cleaup function
//------------------------------------------------------------------------------
void ConvExit(void * handle)
{
   if (!!handle)
      {
      try
         {
         delete (CHtPartitionedConvolution*)handle;
         }
      catch (...)
         {
         }
      }
}
//------------------------------------------------------------------------------

//------------------------------------------------------------------------------
/// Processing function
//------------------------------------------------------------------------------
#pragma argsused
int ConvProcess(void * handle, unsigned nFragsize, unsigned nChannelsIn, unsigned nChannelsOut,
                float* const* ppfDataIn, float* const* ppfDataOut)
{
   if (!!handle)
      ((CHtPartitionedConvolution*)handle)->Process(ppfDataIn, ppfDataOut, nChannelsIn, nFragsize);

   return 0;
}
//------------------------------------------------------------------------------

//------------------------------------------------------------------------------
///
//------------------------------------------------------------------------------
#pragma argsused
int WINAPI DllEntryPoint(HINSTANCE hinst, unsigned long reason, void* lpReserved)
{
   return 1;
}
//---------------------------------------------------------------------------


