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
#include <vcl.h>
#include <stdio.h>
#include <string>
#include <vector>
#include <limits.h>
#include <inifiles.hpp>
#pragma hdrstop
#include "SoundDllProLoaderMain.h"
#include "formSoundDllProLoaderSettings.h"
//---------------------------------------------------------------------------
#pragma package(smart_init)
#pragma resource "*.dfm"

#define TRYDELETENULL(p) {if (p!=NULL) { try {delete p;} catch (...){;} p = NULL;}}

/// list of 'commands' (used for syncronising status of buttons and menues)
static const char* lpcszCmd[] = {
   "Open", "Save", "Exit",
   "Run", "Break", "Step", "ReloadAndRun",
   "Execute", "Stop", "StopAndExit",
   NULL};


void trim(std::string& str, char cTrim);
void ParseValues(TStringList *psl,const char* lpcsz);
   
TfrmSoundMaster *frmSoundMaster;
//------------------------------------------------------------------------------

//------------------------------------------------------------------------------
/// tool function trimming an std::string in place
//------------------------------------------------------------------------------
void trim(std::string& str, char cTrim)
{
  std::string::size_type pos1 = str.find_first_not_of(cTrim);
  std::string::size_type pos2 = str.find_last_not_of(cTrim);
  str = str.substr(pos1 == std::string::npos ? 0 : pos1,
         pos2 == std::string::npos ? str.length() - 1 : pos2 - pos1 + 1);
}
//-----------------------------------------------------------------------------

//------------------------------------------------------------------------------
/// converts a cDelimiter-delimited string list into passed vector of strings
//------------------------------------------------------------------------------
void ParseValues(TStringList *psl,const char* lpcsz)
{
   psl->Clear();

   std::string str = lpcsz;
   std::string strTmp;
   std::string::size_type pos;
   while (1)
      {
      pos = str.find_first_of(';');
      if (pos == str.npos)
         strTmp = str;
      else
         strTmp = str.substr(0, pos);
      trim(strTmp, '"');
      if (!!strTmp.length())
         psl->Add(strTmp.c_str());
      if (pos == str.npos)
         break;
      str = str.erase(0, pos+1);
      }
}
//------------------------------------------------------------------------------

//------------------------------------------------------------------------------
/// constructor. Intializes members
//------------------------------------------------------------------------------
__fastcall TfrmSoundMaster::TfrmSoundMaster(TComponent* Owner)
   :  TForm(Owner),
      m_hLib(NULL),
      m_lpfnSoundDllProCommand(NULL),
      m_ptl(NULL),
      m_pIni(NULL),
      m_bContinue(false)
{
   m_ptl = new TStringList();
   m_ptl->Delimiter = ';';
   m_pIni = new TIniFile(ChangeFileExt(Application->ExeName, ".ini"));

   Left     = m_pIni->ReadInteger("Settings", "Left", 0);
   Top      = m_pIni->ReadInteger("Settings", "Top", 0);
   Width    = m_pIni->ReadInteger("Settings", "Width", 400);
   Height   = m_pIni->ReadInteger("Settings", "Height", 600);
   btnLoop->Visible = m_pIni->ReadBool("Settings", "Loop", false);
   AnsiString s = m_pIni->ReadString("Settings", "script", "");
   try
      {
      if (!s.IsEmpty())
         LoadScript(s);
      }
   catch (...)
      {
      }
}
//------------------------------------------------------------------------------

//------------------------------------------------------------------------------
/// does cleanup
//------------------------------------------------------------------------------
#pragma argsused
void __fastcall TfrmSoundMaster::FormDestroy(TObject *Sender)
{
   ExitLibrary();
   TRYDELETENULL(m_ptl);
   if (m_pIni)
      {
      try
         {
         m_pIni->WriteInteger("Settings", "Left", Left);
         m_pIni->WriteInteger("Settings", "Top", Top);
         m_pIni->WriteInteger("Settings", "Width", Width);
         m_pIni->WriteInteger("Settings", "Height", Height);
         }
      catch (...)
         {
         }
      }
   TRYDELETENULL(m_pIni);
}
//------------------------------------------------------------------------------

//---------------------------------------------------------------------------
/// OnShow callback: initializes DLL
//---------------------------------------------------------------------------
#pragma argsused
void __fastcall TfrmSoundMaster::FormShow(TObject *Sender)
{
   InitLibrary();
}
//---------------------------------------------------------------------------

//------------------------------------------------------------------------------
/// calls stop - exit and shows quit-query
//------------------------------------------------------------------------------
#pragma argsused
void __fastcall TfrmSoundMaster::FormCloseQuery(TObject *Sender,
      bool &CanClose)
{
   m_bScriptBreak = true;
   if (m_hLib)
      {
      StopAndExit();
      CanClose = (ID_YES == MessageBox(Handle, "Do you want to quit?", "Question", MB_ICONQUESTION |MB_YESNO));
      }
}
//------------------------------------------------------------------------------

//------------------------------------------------------------------------------
/// Loads DLL and retrieves string parser function pointer
//------------------------------------------------------------------------------
void __fastcall TfrmSoundMaster::InitLibrary()
{
   ExitLibrary();

   AnsiString s = m_pIni->ReadString("Settings", "library", "sounddllpro.dll");
   if (!FileExists(s))
      {
      miSettingsClick(miSettings);
      s = m_pIni->ReadString("Settings", "library", "sounddllpro.dll");
      }


   // add path of SoundDllPro.dll to search path (for libsndfile-1.dll)
   AnsiString strPath = ";" + ExcludeTrailingBackslash(ExtractFilePath(ExpandFileName(s)));
   // first determine size of current value
   char c[2];

   DWORD dwSize = GetEnvironmentVariable("Path", c, 2);
   dwSize += (DWORD)strPath.Length() + 1;
   // alloc needed size
   char* sz = new char[dwSize];
   ZeroMemory(sz, dwSize);
   // really retrieve it
   GetEnvironmentVariable("Path", sz, dwSize);
//   ShowMessage(sz);
   strPath = AnsiString(sz) + strPath;
//   ShowMessage(strPath);
   delete [] sz;
   // set it
   if (!SetEnvironmentVariable("Path", strPath.c_str()))
      throw Exception("Error setting 'Path' environment variable");

   m_hLib = LoadLibrary(s.c_str());


   if (!m_hLib)
      throw Exception("error loading library '" + s + "'");

   m_lpfnSoundDllProCommand = (LPFNSOUNDDLLPROCOMMAND)GetProcAddress(m_hLib, "_" SOUNDDLL_COMMANDNAME);
   if (!m_lpfnSoundDllProCommand)
      m_lpfnSoundDllProCommand = (LPFNSOUNDDLLPROCOMMAND)GetProcAddress(m_hLib, SOUNDDLL_COMMANDNAME);
   if (!m_lpfnSoundDllProCommand)
      throw Exception("Cannot load function " SOUNDDLL_COMMANDNAME);

}
//------------------------------------------------------------------------------

//------------------------------------------------------------------------------
/// Unloads library
//------------------------------------------------------------------------------
void __fastcall TfrmSoundMaster::ExitLibrary()
{
   if (m_hLib)
      FreeLibrary(m_hLib);
   m_hLib = NULL;
   m_lpfnSoundDllProCommand = NULL;
}
//------------------------------------------------------------------------------

//------------------------------------------------------------------------------
/// Checks, if DLL is loaded
//------------------------------------------------------------------------------
void TfrmSoundMaster::AssertInitialized()
{
   if (!m_hLib)
      throw Exception("Library not initialized");
}
//------------------------------------------------------------------------------

//------------------------------------------------------------------------------
/// Calls DLL
//------------------------------------------------------------------------------
bool TfrmSoundMaster::DoCommand(AnsiString str)
{
   AssertInitialized();

   int nReturn = m_lpfnSoundDllProCommand(str.c_str(), m_lpszReturn, RETSTRMAXLEN);
   ParseValues(m_ptl, m_lpszReturn);


   if (nReturn != SOUNDDLL_RETURN_OK)
      {
      memReturn->Clear();
      memReturn->Lines->Add("error in " + str + ":");
      memReturn->Lines->Add(m_ptl->Values[SOUNDDLLPRO_CMD_ERROR]);
      memReturn->SelStart = memReturn->GetTextLen();
      return false;
      }
   memReturn->Text = m_ptl->Text;
   memReturn->SelStart = memReturn->GetTextLen();
   return true;
}
//------------------------------------------------------------------------------

//------------------------------------------------------------------------------
/// loads a batch file
//------------------------------------------------------------------------------
#pragma argsused
void __fastcall TfrmSoundMaster::btnOpenClick(TObject *Sender)
{
   if (!btnOpen->Enabled)
      return;
   if (od->Execute())
      LoadScript(od->FileName);
}
//------------------------------------------------------------------------------

//------------------------------------------------------------------------------
/// saves a batch file
//------------------------------------------------------------------------------
#pragma argsused
void __fastcall TfrmSoundMaster::btnSaveClick(TObject *Sender)
{
   if (!btnSave->Enabled)
      return;
   sd->FileName = m_strScript;
   if (sd->Execute())
      {
      memCmd->Lines->SaveToFile(sd->FileName);
      m_strScript = sd->FileName;
      }
}
//------------------------------------------------------------------------------

//------------------------------------------------------------------------------
/// executes command in edit field
//------------------------------------------------------------------------------
#pragma argsused
void __fastcall TfrmSoundMaster::btnExecuteClick(TObject *Sender)
{
   if (!btnExecute->Enabled)
      return;
   DoCommand(edCmd->Text);
}
//------------------------------------------------------------------------------

//------------------------------------------------------------------------------
/// executes batch
//------------------------------------------------------------------------------
#pragma argsused
void __fastcall TfrmSoundMaster::btnRunClick(TObject *Sender)
{
   if (Sender == btnLoop)
      btnLoop->Caption = "Loop";
   TControl *pc = dynamic_cast<TControl*>(Sender);
   if (!!pc && !pc->Enabled)
      return;
   pnlCmd->Enabled = false;
   btnOpen->Enabled  = false;
   btnSave->Enabled  = false;
   btnRun->Enabled  = false;
   btnReloadAndRun->Enabled  = false;
   btnStopAndExit->Enabled = true;
   btnExecute->Enabled = false;
   SyncMenuWithButtons();

   if (!m_hLib)
      InitLibrary();
   if (Sender == btnReloadAndRun || Sender == miReloadAndRun)
      LoadScript(m_strScript);

   m_nLoopCount = 0;
   try
      {
      while (1)
         {
         memCmd->SelStart = 0;
         memCmd->SelLength = 0;
         AssertInitialized();
         m_bScriptBreak = false;
         AnsiString str, strL;
         int n;
         for (int i = 0; i < memCmd->Lines->Count; i++)
            {
            str = memCmd->Lines->Strings[i];
            edCmd->Text = str;

//            memCmd->SelStart = memCmd->Perform(EM_LINEINDEX, i, 0) ;
//            memCmd->SelLength = str.Length() ;

            Application->ProcessMessages();
            // comment in line
            n = str.Pos("%");
            if (n > 0)
               str = str.SubString(1, n-1);
            str = Trim(str);

            if (str.IsEmpty())
               continue;
            strL = str.LowerCase();

            Application->ProcessMessages();

            // pause with key input?
            if (strL == "pause")
               {
               m_bContinue = false;
               btnStep->Enabled = true;
               while( !m_bContinue && !m_bScriptBreak)
                  {
                  Application->ProcessMessages();
                  Sleep(1);
                  }
               if (m_bScriptBreak)
                  return;
               continue;
               }
            // pause(MILLISECONDS) ?
            else if (strL.Pos("pause(") == 1)
               {
               if (strL[strL.Length()] != ')')
                  throw Exception("Closing parenthesis missing in 'pause'");
               strL = strL.SubString((int)strlen("pause(") + 1, strL.Length() - (int)strlen("pause(") - 1);
               if (!TryStrToInt(strL, n) || n <= 0)
                  throw Exception("Argument for 'pause' must be integer > 0");
               DWORD dwNow = GetTickCount();
               while (GetTickCount() < dwNow + (DWORD)n)
                  {
                  Application->ProcessMessages();
                  Sleep(1);
                  }
               continue;
               }


            if (!m_hLib || !DoCommand(str) || m_bScriptBreak)
               return;
            Application->ProcessMessages();
            }
         if (Sender != btnLoop)
            break;
         m_nLoopCount++;
         btnLoop->Caption = "Loop " + IntToStr(m_nLoopCount);
         }
      }
   __finally
      {
      memCmd->SelStart = 0;
      memCmd->SelLength = 0;
      pnlCmd->Enabled = true;
      btnOpen->Enabled  = true;
      btnSave->Enabled  = true;
      btnRun->Enabled  = true;
      btnReloadAndRun->Enabled  = true;
      btnExecute->Enabled = true;
      SyncMenuWithButtons();
      }
}
//------------------------------------------------------------------------------

//------------------------------------------------------------------------------
/// sets break flag and stops and exits SoundDllPro
//------------------------------------------------------------------------------
#pragma argsused
void __fastcall TfrmSoundMaster::btnStopClick(TObject *Sender)
{
   TControl *pc = dynamic_cast<TControl*>(Sender);
   if (!!pc && !pc->Enabled)
      return;

   m_bScriptBreak = true;
   DWORD dwNow = GetTickCount();
   while (GetTickCount() < dwNow + 100)
      {
      Application->ProcessMessages();
      Sleep(1);
      }
   if (m_hLib)
      StopAndExit();
   if (Sender == btnStopAndExit || Sender == miStopAndExit)
      ExitLibrary();
}
//------------------------------------------------------------------------------

//------------------------------------------------------------------------------
/// stops and exits dll if necessary
//------------------------------------------------------------------------------
void TfrmSoundMaster::StopAndExit()
{
   if (!m_hLib)
      return;

   DoCommand("command=initialized");
   if (m_ptl->Values[SOUNDDLLPRO_CMD_INITIALIZED] == "1")
      {
      DoCommand("command=stop");
      DoCommand("command=exit");
      }
   memReturn->Clear();
}
//------------------------------------------------------------------------------


//------------------------------------------------------------------------------
/// Loads a script
//------------------------------------------------------------------------------
void TfrmSoundMaster::LoadScript(AnsiString str)
{
   memCmd->Lines->LoadFromFile(str);
   m_strScript = str;
   for (int i = 0; i < memCmd->Lines->Count; i++)
      memCmd->Lines->Strings[i] = SubstituteString(Trim(memCmd->Lines->Strings[i]));
   m_pIni->WriteString("Settings", "script", str);
   btnOpen->Enabled  = true;
   btnSave->Enabled  = true;
   btnRun->Enabled  = true;
   btnReloadAndRun->Enabled  = true;
   SyncMenuWithButtons();

}
//------------------------------------------------------------------------------

//------------------------------------------------------------------------------
/// substitutes a string
//------------------------------------------------------------------------------
AnsiString TfrmSoundMaster::SubstituteString(AnsiString str)
{
   // substitution: one per line only!
   int nPos = str.Pos("${");
   if (nPos > 0)
      {
      int nPos2 = str.Pos("}");
      if (nPos2 == 0)
         throw Exception("invalid substitution: closing paranthesis missing");
      if (nPos2 < nPos)
         throw Exception("invalid substitution: closing paranthesis before opening paranthesis");

      AnsiString strTmp = str.SubString(nPos+2, nPos2-nPos-2);
      AnsiString strSubst = m_pIni->ReadString("Substitution", strTmp, "");
      if (strSubst.IsEmpty())
         throw Exception("substitution '" + strTmp + "' is empty!");
      strTmp = str.SubString(1, nPos-1) + strSubst + str.SubString(nPos2+1, str.Length());
      str = strTmp;
      }

   return str;
}
//------------------------------------------------------------------------------

//------------------------------------------------------------------------------
/// Sets continue flag (for magic PAUSE)
//------------------------------------------------------------------------------
#pragma argsused
void __fastcall TfrmSoundMaster::btnStepClick(TObject *Sender)
{
   if (!btnStep->Enabled)
      return;
   m_bContinue = true;
   btnStep->Enabled = false;
}
//------------------------------------------------------------------------------

//------------------------------------------------------------------------------
/// Exits app
//------------------------------------------------------------------------------
#pragma argsused
void __fastcall TfrmSoundMaster::miExitClick(TObject *Sender)
{
   Close();
}
//------------------------------------------------------------------------------

//------------------------------------------------------------------------------
/// synchronizesmenu items with buttons
//------------------------------------------------------------------------------
void TfrmSoundMaster::SyncMenuWithButtons()
{
   int i = 0;
   TSpeedButton   *pb;
   TMenuItem      *pm;
   while (!!lpcszCmd[i])
      {
      pb = (TSpeedButton*)FindComponent("btn" + AnsiString(lpcszCmd[i]));
      pm = (TMenuItem*)FindComponent("mi" + AnsiString(lpcszCmd[i]));
      if (!!pb && !!pm)
         pm->Enabled = pb->Enabled;
      i++;
      }
}
//------------------------------------------------------------------------------

//------------------------------------------------------------------------------
/// Shows settings dialog
//------------------------------------------------------------------------------
#pragma argsused
void __fastcall TfrmSoundMaster::miSettingsClick(TObject *Sender)
{
   frmSoundDllProLoaderSettings->edPath->Text = ExtractFilePath(m_pIni->ReadString("Settings", "library", "sounddllpro.dll"));
   if (mrOk == frmSoundDllProLoaderSettings->ShowModal())
      m_pIni->WriteString("Settings", "library", IncludeTrailingBackslash(frmSoundDllProLoaderSettings->edPath->Text) + "sounddllpro.dll");
}
//------------------------------------------------------------------------------

//------------------------------------------------------------------------------
/// Tries to show help-PDF
//------------------------------------------------------------------------------
#pragma argsused
void __fastcall TfrmSoundMaster::miHelpClick(TObject *Sender)
{
   AnsiString str = IncludeTrailingBackslash(ExtractFilePath(m_pIni->ReadString("Settings", "library", "sounddllpro.dll"))) + "..\\manual\\sounddllproloader.pdf";
   if (!FileExists(str))
      {
      str = "Cannot find help file '" + str + "'";
      MessageBox(Handle, str.c_str(), "Error", MB_ICONERROR);
      }
   else
      ShellExecute(NULL, "open", str.c_str(), 0, 0, SW_SHOW);
}
//---------------------------------------------------------------------------

//------------------------------------------------------------------------------
/// Calls btnStepClick
//------------------------------------------------------------------------------
#pragma argsused
void __fastcall TfrmSoundMaster::FormKeyPress(TObject *Sender, char &Key)
{
   if (btnStep->Enabled && (Key == VK_RETURN || Key == VK_SPACE))
      btnStepClick(NULL);
}
//------------------------------------------------------------------------------

//------------------------------------------------------------------------------
/// Shortcuts to buttons
//------------------------------------------------------------------------------
#pragma argsused
void __fastcall TfrmSoundMaster::FormKeyDown(TObject *Sender, WORD &Key,
      TShiftState Shift)
{
   if (Shift.Contains(ssAlt) && Shift.Contains(ssShift))
      {
      switch (Key)
         {
         case 'O': btnOpenClick(btnRun); break;
         case 'S': btnSaveClick(btnRun); break;
         case 'X': btnRunClick(btnRun); break;
         case 'R': btnRunClick(btnReloadAndRun); break;
         case 'C': btnStopClick(btnStop); break;
         case 'P': btnStepClick(btnStep); break;
         case 'U': btnStopClick(btnStopAndExit);break;
         }
      }
}
//------------------------------------------------------------------------------

//------------------------------------------------------------------------------
/// If ENTER pressed in edit field, calls btnExecuteClick
//------------------------------------------------------------------------------
#pragma argsused
void __fastcall TfrmSoundMaster::edCmdKeyDown(TObject *Sender, WORD &Key,
      TShiftState Shift)
{
   if (Key == VK_RETURN)
      btnExecuteClick(btnExecute);
}
//------------------------------------------------------------------------------

//------------------------------------------------------------------------------
/// Copies selected command do edit field and executes it
//------------------------------------------------------------------------------
#pragma argsused
void __fastcall TfrmSoundMaster::memCmdDblClick(TObject *Sender)
{
/*
   int nLine = memCmd->Perform(EM_LINEFROMCHAR, memCmd->SelStart, 0) ;
   if (nLine != -1 && btnExecute->Enabled)
      {
      edCmd->Text = memCmd->Lines->Strings[nLine];
      btnExecuteClick(btnExecute);
      }
 */
}
//------------------------------------------------------------------------------



