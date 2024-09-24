//------------------------------------------------------------------------------
/// \file MPlugin.h
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
#ifndef MPluginH
#define MPluginH
//------------------------------------------------------------------------------

#include <vcl.h>
#include <vector>
#include <valarray>
#include "mplugin_defs.h"
//------------------------------------------------------------------------------


//------------------------------------------------------------------------------
/// structure containing all properties for MATLAB plugin passed to constructor
/// of TMPlugin
//------------------------------------------------------------------------------
struct MPluginStruct{
   AnsiString     strMatlab;
   AnsiString     strPluginPath;
   AnsiString     strPlugin;
   AnsiString     strInitCmd;
   AnsiString     strProcCmd;
   unsigned int   nSamples;
   unsigned int   nUserDataSize;
   unsigned int   nInChannels;
   unsigned int   nOutChannels;
   bool           bTerminateProcess;
   bool           bShowProcess;
   unsigned int   nStartTimeout;
   bool           bJVM;
};
//------------------------------------------------------------------------------

//------------------------------------------------------------------------------
/// \class TMPlugin. Class for creating an independant MATLAB process for DSP.
/// \brief Establishes shared memory and events for inter-process communication.
/// Description in cpp.
//------------------------------------------------------------------------------
class TMPlugin
{
   private:
      CRITICAL_SECTION                    m_csLock;
      HANDLE                              m_hProcessHandle;
      HANDLE                              m_hThreadHandle;
      HANDLE                              m_hIPCEvent[IPC_EVENT_LAST];
      HANDLE                              m_hMapFile;
      LPCTSTR                             m_pDataBuf;
      MPluginStruct                       m_mps;
      AnsiString                          m_strError;
      std::vector<std::valarray<float> >  m_vvfInUserData;
      std::vector<std::valarray<float> >  m_vvfOutUserData;
      std::vector<std::valarray<float> >  m_vvfTmpData;
      int                                 m_nDataBytesPerChannel;
      int                                 m_nUserBytesPerChannel;
      AnsiString __fastcall               CreateMemoryMappedFile(AnsiString str);
      void __fastcall                     DeleteMemoryMappedFile();
   protected:
      void __fastcall                     Exit();
   public:
      TMPlugin(MPluginStruct &mps);
      ~TMPlugin();
      void __fastcall                     Process( std::vector<std::valarray<float> >& vvfInBuffers,
                                                   std::vector<std::valarray<float> >& vvfOutBuffers);
      void __fastcall                     Init(bool bRealtimePriority);
      void __fastcall                     ClearUserData();
      void __fastcall                     SetUserData(double* lpData,
                                                      unsigned int nChannels,
                                                      unsigned int nValues,
                                                      bool bOutput);
      std::vector<std::valarray<float> >& __fastcall GetUserData(bool bOutput);
};
//---------------------------------------------------------------------------
#endif
