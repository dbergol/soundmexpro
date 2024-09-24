
//------------------------------------------------------------------------------
/// \file SoundDllProLoaderMain.cpp
/// \author Berg
/// \brief Loader for SoundDllPro.dll with string interface
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
#ifndef SoundDllProLoaderMainH
#define SoundDllProLoaderMainH
//------------------------------------------------------------------------------
#include <Classes.hpp>
#include <Controls.hpp>
#include <StdCtrls.hpp>
#include <Forms.hpp>
#include "soundmexpro_defs.h"
#include <ComCtrls.hpp>
#include <ExtCtrls.hpp>
#include <Buttons.hpp>
#include <Dialogs.hpp>
#include <ToolWin.hpp>
#include <Menus.hpp>
#include <XPMan.hpp>

#define RETSTRMAXLEN 2*SHRT_MAX
//------------------------------------------------------------------------------
/// Main form for SoundDllProLoader
//------------------------------------------------------------------------------
class TfrmSoundMaster : public TForm
{
   __published:
      TPanel *Panel1;
      TEdit *edCmd;
      TPanel *Panel2;
      TSpeedButton *btnOpen;
      TSpeedButton *btnExecute;
      TSpeedButton *btnRun;
      TOpenDialog *od;
      TSpeedButton *btnStop;
      TSpeedButton *btnStopAndExit;
      TSpeedButton *btnLoop;
      TPanel *pnlCmd;
      TMemo *memCmd;
      TSpeedButton *btnSave;
      TSpeedButton *btnStep;
      TSpeedButton *btnReloadAndRun;
      TSaveDialog *sd;
      TMainMenu *MainMenu;
      TMenuItem *miFile;
      TMenuItem *miScript;
      TMenuItem *N1;
      TMenuItem *miOpen;
      TMenuItem *miSave;
      TMenuItem *N2;
      TMenuItem *miExit;
      TMenuItem *miCommand;
      TMenuItem *miStopAndExit;
      TMenuItem *miRun;
      TMenuItem *miStop;
      TMenuItem *miStep;
      TMenuItem *N3;
      TMenuItem *miReloadAndRun;
      TMenuItem *miExecute;
      TMenuItem *miSettings;
      TMenuItem *miHelp;
      TRichEdit *memReturn;
      void __fastcall FormDestroy(TObject *Sender);
      void __fastcall edCmdKeyDown(TObject *Sender, WORD &Key,
             TShiftState Shift);
      void __fastcall memCmdDblClick(TObject *Sender);
      void __fastcall FormCloseQuery(TObject *Sender, bool &CanClose);
      void __fastcall btnOpenClick(TObject *Sender);
      void __fastcall btnExecuteClick(TObject *Sender);
      void __fastcall btnRunClick(TObject *Sender);
      void __fastcall btnStepClick(TObject *Sender);
      void __fastcall FormKeyPress(TObject *Sender, char &Key);
      void __fastcall FormKeyDown(TObject *Sender, WORD &Key,
             TShiftState Shift);
      void __fastcall btnSaveClick(TObject *Sender);
      void __fastcall miExitClick(TObject *Sender);
      void __fastcall miSettingsClick(TObject *Sender);
      void __fastcall miHelpClick(TObject *Sender);
      void __fastcall FormShow(TObject *Sender);
      void __fastcall btnStopClick(TObject *Sender);
   private:
      HINSTANCE               m_hLib;  ///< instance handle for SoundDllPro.dll
      LPFNSOUNDDLLPROCOMMAND  m_lpfnSoundDllProCommand;  ///< pointer to exported command function
      TStringList             *m_ptl;           ///< string list for parsing
      bool                    m_bScriptBreak;   ///< break flag to stop a script
      char                    m_lpszReturn[RETSTRMAXLEN];   ///< buffer for return values
      TIniFile                *m_pIni;          ///< instance for own inifile
      AnsiString              m_strScript;      ///< name of current script
      int                     m_nLoopCount;     ///< current loop count
      bool                    m_bContinue;      ///< continue flag   
      void __fastcall         ExitLibrary();
      void __fastcall         InitLibrary();
      bool                    DoCommand(AnsiString str);
      void                    AssertInitialized();
      void                    LoadScript(AnsiString str);
      AnsiString              SubstituteString(AnsiString str);
      void                    SyncMenuWithButtons();
      void                    StopAndExit();
   public:
      __fastcall              TfrmSoundMaster(TComponent* Owner);
};
//------------------------------------------------------------------------------

//------------------------------------------------------------------------------
extern PACKAGE TfrmSoundMaster *frmSoundMaster;
//------------------------------------------------------------------------------
#endif
