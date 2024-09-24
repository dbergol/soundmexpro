//------------------------------------------------------------------------------
/// \file VersionCheck.h
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
#ifndef frmVersionCheckH
#define frmVersionCheckH
//------------------------------------------------------------------------------
#include <System.Classes.hpp>
#include <Vcl.Controls.hpp>
#include <Vcl.StdCtrls.hpp>
#include <Vcl.Forms.hpp>
class TVersionChecker;
//------------------------------------------------------------------------------
/// Dialog for showing version information
//------------------------------------------------------------------------------
class TformVersionCheck : public TForm
{
   __published:	// Von der IDE verwaltete Komponenten
      TMemo *memVersionInfo;
      TLabel *lblLatestVersion;
      TButton *OKButton;
      TButton *btnHP;
      TGroupBox *gbVersionHistory;
      TLabel *lblCurrentVersion;
      void __fastcall btnHPClick(TObject *Sender);
   private:	// Benutzer-Deklarationen
   public:		// Benutzer-Deklarationen
      __fastcall TformVersionCheck(TComponent* Owner);
      void           DoShowModal(TVersionChecker &rvch, 
                                 UnicodeString usTitle, 
                                 UnicodeString usVersion, 
                                 UnicodeString usDownloadURL);
};
//------------------------------------------------------------------------------
#endif
