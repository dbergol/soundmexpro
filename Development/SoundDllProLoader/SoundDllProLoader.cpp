//------------------------------------------------------------------------------
/// \file SoundDllProLoader.cpp
/// \author Berg
/// \brief Main for SoundDllProLoader.exe
///
/// Project SoundMexPro
/// Module  SoundDllProLoader.exe
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
#pragma hdrstop
#include <Vcl.Styles.hpp>
#include <Vcl.Themes.hpp>
#include "SoundDllPro_Style.h"
USEFORM("SoundDllProLoaderMain.cpp", frmSoundMaster);
USEFORM("formSoundDllProLoaderSettings.cpp", frmSoundDllProLoaderSettings);
//---------------------------------------------------------------------------
USEFORM("SoundDllProLoaderMain.cpp", frmSoundMaster)
USEFORM("formSoundDllProLoaderSettings.cpp", frmSoundDllProLoaderSettings)
//---------------------------------------------------------------------------


//---------------------------------------------------------------------------
/// WinMain (default from C++-Builder)
//---------------------------------------------------------------------------
#ifdef __clang__
int WINAPI _tWinMain(HINSTANCE, HINSTANCE, LPTSTR, int)
#else
WINAPI WinMain(HINSTANCE, HINSTANCE, LPSTR, int)
#endif
{
   try
   {
     SetStyle();
     Application->Initialize();
     Application->CreateForm(__classid(TfrmSoundMaster), &frmSoundMaster);
     Application->CreateForm(__classid(TfrmSoundDllProLoaderSettings), &frmSoundDllProLoaderSettings);
     Application->Run();
   }
   catch (Exception &exception)
   {
      Application->ShowException(&exception);
   }
   return 0;
}
//------------------------------------------------------------------------------
