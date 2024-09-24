//------------------------------------------------------------------------------
/// \file formSoundDllProLoaderSettings.h
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
#ifndef formSoundDllProLoaderSettingsH
#define formSoundDllProLoaderSettingsH
//------------------------------------------------------------------------------
#include <Classes.hpp>
#include <Controls.hpp>
#include <StdCtrls.hpp>
#include <Forms.hpp>

//------------------------------------------------------------------------------
/// Dialog for selecting a file path
//------------------------------------------------------------------------------
class TfrmSoundDllProLoaderSettings : public TForm
{
   __published:	// Von der IDE verwaltete Komponenten
      TButton *btnOk;
      TButton *btnCancel;
      TGroupBox *gb;
      TEdit *edPath;
      TLabel *Label1;
      TButton *btnSelPath;
      void __fastcall btnSelPathClick(TObject *Sender);
   private:	// Anwender-Deklarationen
   public:		// Anwender-Deklarationen
      __fastcall TfrmSoundDllProLoaderSettings(TComponent* Owner);
};
//------------------------------------------------------------------------------
extern PACKAGE TfrmSoundDllProLoaderSettings *frmSoundDllProLoaderSettings;
//------------------------------------------------------------------------------
#endif
