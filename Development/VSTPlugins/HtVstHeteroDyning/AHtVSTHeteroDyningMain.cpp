//------------------------------------------------------------------------------
/// \file AHtVSTHeteroDyningMain.cpp
/// \author Berg
/// \brief Implementation of main function of VST plugin HtVSTHeteroDyning
///
/// Project SoundMexPro
/// Module  HtVSTHeteroDyning.dll
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
#include <vcl.h>
#include <windows.h>
#include <limits.h>
#include <float.h>


#include "AHtVSTHeteroDyning.h"

//--------------------------------------------------------------------------

//--------------------------------------------------------------------------
/// Prototype of the export function VSTPluginMain
//--------------------------------------------------------------------------
extern "C" {
   AEffect *VSTPluginMain (audioMasterCallback audioMaster);
};
//--------------------------------------------------------------------------


//--------------------------------------------------------------------------
/// main function creating effect
//--------------------------------------------------------------------------
AEffect *VSTPluginMain (audioMasterCallback audioMaster)
{
   // Get VST Version
   if (!audioMaster (0, audioMasterVersion, 0, 0, 0, 0))
      return 0;  // old version

   randomize();


   // Create the AudioEffect
   CHtVSTHeteroDyning* effect = new CHtVSTHeteroDyning (audioMaster);

   if (!effect)
      return 0;

   // Check if no problem in constructor of CHtVSTHeteroDyning
   if (!effect->m_bIsValid)
      {
      delete effect;
      return 0;
      }
   return effect->getAeffect();
}
//--------------------------------------------------------------------------


//--------------------------------------------------------------------------
/// Dll-entry
//--------------------------------------------------------------------------
#pragma argsused
BOOL WINAPI DllMain (HINSTANCE hInst, DWORD dwReason, LPVOID lpvReserved)
{
   #ifndef _WIN64
   if (dwReason == DLL_PROCESS_ATTACH)
      _control87(PC_64|MCW_EM,MCW_PC|MCW_EM);
   #endif
   
   return 1;
}
//--------------------------------------------------------------------------
