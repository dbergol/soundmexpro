//------------------------------------------------------------------------------
/// \file SoundDllPro_Style.cpp
/// \author Berg
/// \brief Implementation of helper functions for VCL styles
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
#pragma hdrstop
#include "SoundDllPro_Style.h"
#include <vcl.h>
#include <Registry.hpp>
#include <Inifiles.hpp>


//------------------------------------------------------------------------------
/// reads windows app-dark-mode
//------------------------------------------------------------------------------
static int SystemDarkTheme() 
{
   int nReturn = 0;   
   TRegistry *pRegistry = NULL;
   try 
      {
      pRegistry = new TRegistry(KEY_READ);
      pRegistry->RootKey = HKEY_CURRENT_USER;
      // False because we do not want to create it if it doesn’t exist
      pRegistry->OpenKey("Software\\Microsoft\\Windows\\CurrentVersion\\Themes\\Personalize", false);
      if (pRegistry->ReadInteger("AppsUseLightTheme") == 0)
         nReturn = 1;
      }
   catch (...) 
      {
      
      }
   if (pRegistry)
      delete pRegistry;
   return nReturn;
}
//------------------------------------------------------------------------------

//------------------------------------------------------------------------------
/// sets style from ini or windows respectively
//------------------------------------------------------------------------------
void SetStyle()
{
   // default: system mode
   int nMode = -1;
   UnicodeString usFixStyle   = "";
   UnicodeString usLightStyle = "Windows";
   UnicodeString usDarkStyle  = "Windows10 Dark";
   
   TIniFile* pIni = NULL;
   try
      {
      pIni           = new TIniFile(IncludeTrailingBackslash(ExtractFilePath(Application->ExeName)) + "soundmexpro.ini");
      nMode          = pIni->ReadInteger("Settings", "DarkMode", -1);      
      usFixStyle     = pIni->ReadString("Settings", "FixStyle", "");      
      usLightStyle   = pIni->ReadString("Settings", "LightStyle", "Windows");      
      usDarkStyle    = pIni->ReadString("Settings", "DarkStyle", "Windows10 Dark");      
      }
   catch (...) 
      {
      
      }
   if (pIni)
      delete pIni;
   if (nMode == -1)
      nMode = SystemDarkTheme();
   UnicodeString usStyle = (nMode == 0) ? usLightStyle : usDarkStyle;  
   if (!usFixStyle.IsEmpty())
      usStyle = usFixStyle;
   
   TStyleManager::TrySetStyle(usStyle);
}
//------------------------------------------------------------------------------

//------------------------------------------------------------------------------
/// returns true, if current style contains 'Dark'
//------------------------------------------------------------------------------
bool IsDarkTheme()
{
   return TStyleManager::ActiveStyle->Name.Pos("Dark") > 0;
}
//------------------------------------------------------------------------------

