//------------------------------------------------------------------------------
/// \file formVisual.cpp
/// \author Berg
/// \brief Implementation of class TAboutBox
///
/// Project SoundMexPro
/// Module  SoundDllPro.dll
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
#pragma hdrstop
#include "formAbout.h"
#include "formTracks.h"
#include "formMixer.h"
#include "SoundDllPro_Main.h"
//------------------------------------------------------------------------------
#pragma package(smart_init)
#pragma resource "*.dfm"
#pragma warn -use

using namespace Asio;

UnicodeString TAboutBox::ms_usVerInfo;
//------------------------------------------------------------------------------
/// constructor
//------------------------------------------------------------------------------
__fastcall TAboutBox::TAboutBox(TComponent* Owner)
   : TForm(Owner)
{
}
//------------------------------------------------------------------------------
//------------------------------------------------------------------------------
/// Shows form modal with respect to 'stayontop' of other forms
//------------------------------------------------------------------------------
int __fastcall TAboutBox::About()
{
   
   if (IsDarkTheme())
      {
      
      pnl->Color     = TColor(0x404040);
      pnlSMP->Color  = pnl->Color;
      }
   else
      {
      pnl->ParentColor     = true;
      pnlSMP->ParentColor  = true;
      }
   Tag = 0;
   if (SoundClass() && !SoundClass()->m_bNoGUI)
      {
      bool bMix      = (!!SoundClass()->m_pfrmMixer
                        && SoundClass()->m_pfrmMixer->Visible
                        && SoundClass()->m_pfrmMixer->FormStyle == fsStayOnTop);
      bool bTracks   = (!!SoundClass()->m_pfrmTracks
                        && SoundClass()->m_pfrmTracks->Visible
                        && SoundClass()->m_pfrmTracks->FormStyle == fsStayOnTop);
      if (bMix)
         SoundClass()->m_pfrmMixer->FormStyle = Forms::fsNormal;
      if (bTracks)
         SoundClass()->m_pfrmTracks->FormStyle = Forms::fsNormal;
      ShowModal();
      if (bMix)
         SoundClass()->m_pfrmMixer->FormStyle = fsStayOnTop;
      if (bTracks)
         SoundClass()->m_pfrmTracks->FormStyle = fsStayOnTop;
      }
   else
      ShowModal();
   return (int)Tag;
}
//------------------------------------------------------------------------------

//------------------------------------------------------------------------------
/// OnShow brings window to top
//------------------------------------------------------------------------------
#pragma argsused
void __fastcall TAboutBox::FormShow(TObject *Sender)
{
   UnicodeString us = VString();
   if (ms_usVerInfo.Length())
      us = us + " (" + ms_usVerInfo + ")";
   #ifndef _WIN64
   us = us + " (32-bit)";
   #else
   us = us + " (64-bit)";
   #endif
   lbVersionValue->Caption = us;
   SetForegroundWindow(Handle);
   BringToFront();
}
//------------------------------------------------------------------------------

//------------------------------------------------------------------------------
/// Calculates and shows perfomance/xrun info
//------------------------------------------------------------------------------
#pragma argsused
void __fastcall TAboutBox::PerformanceTimerTimer(TObject *Sender)
{
   PerformanceTimer->Enabled = false;
   if (SoundClass())
      SoundClass()->ShowPerformance();
   PerformanceTimer->Enabled = true;
   if (PerformanceTimer->Tag >= 0)
      UpdateDriverStatus((Asio::State)PerformanceTimer->Tag);
   PerformanceTimer->Tag = -1;
}
//------------------------------------------------------------------------------

//------------------------------------------------------------------------------
/// shows driver status string
//------------------------------------------------------------------------------
void TAboutBox::UpdateDriverStatus(Asio::State as)
{
   if (!SoundClass())
      return;
   if (as <= RUNNING)
      {
      if (SoundClass()->m_pfrmTracks)
         SoundClass()->m_pfrmTracks->SetStatusString(g_lpcszDriverStatus[as], 0);
      if (SoundClass()->m_pfrmMixer)
         SoundClass()->m_pfrmMixer->SetStatusString(g_lpcszDriverStatus[as], 0);
      }
   if (as < RUNNING)
      SoundClass()->ResetLevels();
}
//------------------------------------------------------------------------------

//------------------------------------------------------------------------------
/// OnClikc callback of btnCheckForUpdate: checks for updates
//------------------------------------------------------------------------------
#pragma argsused
void __fastcall TAboutBox::btnCheckForUpdateClick(TObject *Sender)
{
   CheckForUpdate(this);   
}
//------------------------------------------------------------------------------

