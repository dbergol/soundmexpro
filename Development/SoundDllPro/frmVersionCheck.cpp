//------------------------------------------------------------------------------
/// \file VersionCheck.cpp
/// \author Berg
/// \brief Implementation of form for version checking 
///
/// Project AudioSpike
/// Module  AudioSpike.exe
///
///
/// ****************************************************************************
/// Copyright 2023 Daniel Berg, Oldenburg, Germany
/// ****************************************************************************
///
/// This file is part of AudioSpike.
///
///    AudioSpike is free software: you can redistribute it and/or modify
///    it under the terms of the GNU General Public License as published by
///    the Free Software Foundation, either version 3 of the License, or
///    (at your option) any later version.
///
///    AudioSpike is distributed in the hope that it will be useful,
///    but WITHOUT ANY WARRANTY; without even the implied warranty of
///    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
///    GNU General Public License for more details.
///
///    You should have received a copy of the GNU General Public License
///    along with AudioSpike.  If not, see <http:///www.gnu.org/licenses/>.
///
//------------------------------------------------------------------------------
#include <vcl.h>
#pragma hdrstop

#include "frmVersionCheck.h"
#include "VersionCheck.h"
//------------------------------------------------------------------------------
#pragma package(smart_init)
#pragma resource "*.dfm"
//------------------------------------------------------------------------------

//------------------------------------------------------------------------------
/// constructor. Empty
//------------------------------------------------------------------------------
__fastcall TformVersionCheck::TformVersionCheck(TComponent* Owner)
   : TForm(Owner)
{
}
//------------------------------------------------------------------------------

//------------------------------------------------------------------------------
/// OnClick callback of btnHP: calls ShellExecute with download URL
//------------------------------------------------------------------------------
#pragma argsused
void __fastcall TformVersionCheck::btnHPClick(TObject *Sender)
{
   ShellExecuteW(NULL, NULL, btnHP->Hint.w_str(), NULL, NULL, SW_SHOWNORMAL);   
}
//------------------------------------------------------------------------------

//------------------------------------------------------------------------------
/// Sets strings and shows form modal
//------------------------------------------------------------------------------
void TformVersionCheck::DoShowModal(TVersionChecker &rvch, 
                                    UnicodeString usTitle,
                                    UnicodeString usVersion, 
                                    UnicodeString usDownloadURL)
{
   btnHP->Hint = usDownloadURL;
   lblLatestVersion->Caption = "A new version of " + usTitle + " is available: " + rvch.GetVersionLatest();
   lblCurrentVersion->Caption = "Your are currently running version: " + usVersion; 
   memVersionInfo->Text = rvch.GetVersionHistoryToVersion(usVersion);
   ShowModal();      
}
//------------------------------------------------------------------------------
