//------------------------------------------------------------------------------
/// \file formSoundDllProLoaderSettings.cpp
/// \author Berg
/// \brief Settings for loader for SoundDllPro.dll with string interface
///
/// Project SoundMexPro
/// Module  SoundDllProLoader.exe
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
#include <shlobj.h>
#include <windowsx.h>

#pragma hdrstop

#include "formSoundDllProLoaderSettings.h"
//------------------------------------------------------------------------------
#pragma package(smart_init)
#pragma resource "*.dfm"
TfrmSoundDllProLoaderSettings *frmSoundDllProLoaderSettings;


//------------------------------------------------------------------------------
/// constructor
//------------------------------------------------------------------------------
__fastcall TfrmSoundDllProLoaderSettings::TfrmSoundDllProLoaderSettings(TComponent* Owner)
   : TForm(Owner)
{
}
//------------------------------------------------------------------------------

//------------------------------------------------------------------------------
/// OnClick callback attached btnSelPath. Shows a dialog to select a path using
/// WIndows API SHBrowseForFolder
//------------------------------------------------------------------------------
#pragma argsused
void __fastcall TfrmSoundDllProLoaderSettings::btnSelPathClick(
      TObject *Sender)
{
   ::BROWSEINFO bi;
   char GDir[4*MAX_PATH];
   char FolderName[4*MAX_PATH];
   LPITEMIDLIST ItemID;
   memset(&bi, 0, sizeof(::BROWSEINFO));
   memset(GDir, 0, MAX_PATH);
   bi.hwndOwner      = NULL;
   bi.pszDisplayName = FolderName;
   bi.lpszTitle      = "";
   ItemID = SHBrowseForFolder(&bi);
   if (!!ItemID)
      {
      SHGetPathFromIDList(ItemID, GDir);
      GlobalFreePtr(ItemID);
      }
   edPath->Text = GDir;
}
//------------------------------------------------------------------------------

