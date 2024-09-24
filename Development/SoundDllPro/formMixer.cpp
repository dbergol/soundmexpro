//------------------------------------------------------------------------------
/// \file formMixer.cpp
/// \author Berg
/// \brief Implementation of class TMixerForm for visualization of audio data
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
#include <math.h>
#pragma hdrstop

#include "formMixer.h"
#include "System.UIConsts.hpp"

#include "formTracks.h"
#include "SoundDllPro_Main.h"

//------------------------------------------------------------------------------
#pragma package(smart_init)
#pragma resource "*.dfm"
//------------------------------------------------------------------------------

#pragma warn -use

//------------------------------------------------------------------------------
/// constructor
//------------------------------------------------------------------------------
__fastcall TMixerForm::TMixerForm(TComponent* Owner)
   : TForm(Owner), m_MixerWithFocus(NULL), m_bPositionsRead(false)
{
    // don't know why, but must set font sizes again here: in object inspector
   // is not sufficient...
   sbStatus->Font->Size = 7;
   InitializeCriticalSection(&m_csMixer);
   m_vvMixers.resize(3);

   bool bDark = IsDarkTheme();
   if (IsDarkTheme())  
      {
      pnlTracks->Color  = SMP_CHNL_BLUE_DARK;
      pnlOutputs->Color = SMP_CHNL_GREEN_DARK;
      pnlInputs->Color  = SMP_CHNL_ORANGE_DARK;
      }
   else
      {
      pnlTracks->Color  = SMP_CHNL_BLUE_LIGHT;
      pnlOutputs->Color = SMP_CHNL_GREEN_LIGHT;
      pnlInputs->Color  = SMP_CHNL_ORANGE_LIGHT;
      }

   pnlBtnTracks->Color  = pnlTracks->Color;
   pnlBtnOutputs->Color = pnlOutputs->Color;
   pnlBtnInputs->Color  = pnlInputs->Color;   
}
//------------------------------------------------------------------------------

//------------------------------------------------------------------------------
/// destructor: does cleanup and writes settings to inifile
//------------------------------------------------------------------------------
__fastcall TMixerForm::~TMixerForm()

{
   Exit();
   if (m_bPositionsRead)
      {
      try
         {
         SetProfileString("MixerLeft" +  m_asIniSuffix, Left);
         SetProfileString("MixerTop" + m_asIniSuffix, Top);
         SetProfileString("MixerWidth" + m_asIniSuffix, Width);
         }
      // clear exception silently
      catch (...)
         {
         }
      }
   DeleteCriticalSection(&m_csMixer);

}
//------------------------------------------------------------------------------


//------------------------------------------------------------------------------
/// initializes visual form: creates TLevelFrame for each input and each output
/// and sets sizes.
//------------------------------------------------------------------------------
void TMixerForm::Init()
{
   // return if initialized (!)
   if (m_vvMixers[CT_TRACK].size())
      return;
   if (!SoundClass())
      throw Exception("global SoundClass is invalid");

   unsigned int nIndex;

   // create track mixers
   pnlTracks->Width = 2;
   for (nIndex = 0; nIndex < SoundClass()->m_vTracks.size(); nIndex++)
      {
      m_vvMixers[CT_TRACK].push_back(new TMixerFrame(pnlTracks, this, CT_TRACK, (int)nIndex));
      }
   pnlTracks->Tag = pnlTracks->Width;
   m_vafTrackVolumes.resize(m_vvMixers[CT_TRACK].size());
   m_vanTrackClipCount.resize(m_vvMixers[CT_TRACK].size());

   UpdateChannelData(CT_TRACK);

   // create output channel mixers
   pnlOutputs->Width = 2;
   for (nIndex = 0; nIndex < SoundClass()->SoundActiveChannels(Asio::OUTPUT); nIndex++)
      m_vvMixers[CT_OUTPUT].push_back(new TMixerFrame(pnlOutputs, this, CT_OUTPUT, (int)nIndex));
   pnlOutputs->Tag = pnlOutputs->Width;
   UpdateChannelData(CT_OUTPUT);


   // create input channel mixers
   pnlInputs->Width = 2;
   for (nIndex = 0; nIndex < SoundClass()->SoundActiveChannels(Asio::INPUT); nIndex++)
      m_vvMixers[CT_INPUT].push_back(new TMixerFrame(pnlInputs, this, CT_INPUT, (int)nIndex));
   pnlInputs->Tag = pnlInputs->Width;
   UpdateChannelData(CT_INPUT);

   m_asIniSuffix = "_" + IntToStr((int)SoundClass()->m_vTracks.size())
                  + "_" + IntToStr((int)SoundClass()->SoundActiveChannels(Asio::OUTPUT))
                  + "_" + IntToStr((int)SoundClass()->SoundActiveChannels(Asio::INPUT));

   m_bCanResize = true;
   Width = 10;
   ClientHeight = TMixerFrame::GetHeight() + sbStatus->Height;
   m_bCanResize = false;
   AdjustWidth(true);

   try
      {
      Left           = GetProfileStringDefault("MixerLeft" + m_asIniSuffix, 0);
      Top            = GetProfileStringDefault("MixerTop" + m_asIniSuffix, 0);
      int n          = GetProfileStringDefault("MixerWidth" + m_asIniSuffix, 0);
      // currently
      if (n > 0 && n < Width)
         Width = n;
      m_bPositionsRead = true;
      }
   catch (...)
      {
      }

   try
      {
      Application->ActivateHint(Mouse->CursorPos);
      }
   catch(...){};
   LevelUpdateTimer->Enabled = true;
}
//------------------------------------------------------------------------------

//------------------------------------------------------------------------------
/// does cleanup: removes all instances of TMixerFrame for tracks, inputs and
/// outputs
//------------------------------------------------------------------------------
void TMixerForm::Exit()
{
   unsigned int nIndex;
   for (nIndex = 0; nIndex < m_vvMixers[CT_TRACK].size(); nIndex++)
      {
      TRYDELETENULL(m_vvMixers[CT_TRACK][nIndex]);
      }
   m_vvMixers[CT_TRACK].clear();

   for (nIndex = 0; nIndex < m_vvMixers[CT_OUTPUT].size(); nIndex++)
      {
      TRYDELETENULL(m_vvMixers[CT_OUTPUT][nIndex]);
      }
   m_vvMixers[CT_OUTPUT].clear();

   for (nIndex = 0; nIndex < m_vvMixers[CT_INPUT].size(); nIndex++)
      {
      TRYDELETENULL(m_vvMixers[CT_INPUT][nIndex]);
      }
   m_vvMixers[CT_INPUT].clear();

}
//------------------------------------------------------------------------------

//------------------------------------------------------------------------------
/// calls About()
//------------------------------------------------------------------------------
#pragma argsused
void __fastcall TMixerForm::About1Click(TObject *Sender)
{
   SoundClass()->About();
}
//------------------------------------------------------------------------------


//------------------------------------------------------------------------------
/// sets a status bar string
//------------------------------------------------------------------------------
void TMixerForm::SetStatusString(AnsiString str, int nIndex)
{
   if (!Visible || !sbStatus || nIndex >= sbStatus->Panels->Count)
      return;
   try
      {
      sbStatus->Panels->Items[nIndex]->Text = str;
      }
   catch (...)
      {
      }
}
//------------------------------------------------------------------------------

//------------------------------------------------------------------------------
/// updates track levels _and_ clipcounts
//------------------------------------------------------------------------------
void TMixerForm::UpdateLevels(std::valarray<float>& vaf,
                              std::valarray<unsigned int>& van)
{
   if (!m_vvMixers[CT_TRACK].size())
      return;
   // do not update more than once per 100 ms
   static DWORD dw = GetTickCount();
   if (ElapsedSince(dw) < 100)
      return;
   dw = GetTickCount();

   if (!TryEnterCriticalSection(&m_csMixer))
      return;
   try
      {
      m_vafTrackVolumes = vaf;
      m_vanTrackClipCount = van;
      }
   __finally
      {
      LeaveCriticalSection(&m_csMixer);
      }
}
//------------------------------------------------------------------------------

//------------------------------------------------------------------------------
/// updates clipcounts
//------------------------------------------------------------------------------
void TMixerForm::UpdateClipCounts(std::valarray<unsigned int>& van)
{
   if (!m_vvMixers[CT_TRACK].size())
      return;

   if (!TryEnterCriticalSection(&m_csMixer))
      return;
   try
      {
      m_vanTrackClipCount = van;
      }
   __finally
      {
      LeaveCriticalSection(&m_csMixer);
      }
}
//------------------------------------------------------------------------------

//------------------------------------------------------------------------------
/// Resets track levels
//------------------------------------------------------------------------------
void TMixerForm::ResetLevels()
{
   if (!m_vafTrackVolumes.size())
      return;

   EnterCriticalSection(&m_csMixer);
   try
      {
      m_vafTrackVolumes = 0.0f;
      }
   __finally
      {
      LeaveCriticalSection(&m_csMixer);
      }
}
//------------------------------------------------------------------------------

//------------------------------------------------------------------------------
/// Updates levels on all mixers
//------------------------------------------------------------------------------
void TMixerForm::SetLevelsInternal()
{
   if (!TryEnterCriticalSection(&m_csMixer))
      return;
   try
      {
      if (pnlTracks->Visible)
         {
         for (unsigned int n = 0; n < m_vafTrackVolumes.size(); n++)
            {
            if (n >= m_vvMixers[CT_TRACK].size())
               break;
            m_vvMixers[CT_TRACK][n]->UpdateLevel(m_vafTrackVolumes[n], m_vanTrackClipCount[n]);
            }
         }
      if (pnlOutputs->Visible)
         {
         const std::vector<unsigned int>& rvanOut = SoundClass()->GetClipCount(Asio::OUTPUT);
         for (unsigned int n = 0; n < m_vvMixers[CT_OUTPUT].size(); n++)
            {
            if (n >= SoundClass()->m_vfOutChannelLevel.size())
               break;
            m_vvMixers[CT_OUTPUT][n]->UpdateLevel(SoundClass()->m_vfOutChannelLevel[n], rvanOut[n]);
            }
         }
      if (pnlInputs->Visible)
         {
         const std::vector<unsigned int>& rvanIn = SoundClass()->GetClipCount(Asio::INPUT);
         for (unsigned int n = 0; n < m_vvMixers[CT_INPUT].size(); n++)
            {
            if (n >= SoundClass()->m_vfInChannelLevel.size())
               break;
            m_vvMixers[CT_INPUT][n]->UpdateLevel(SoundClass()->m_vfInChannelLevel[n], rvanIn[n]);
            }
         }

      if (SoundClass() && SoundClass()->m_pfrmTracks->Visible)
         SoundClass()->m_pfrmTracks->SetTrackLevelsInternal(m_vafTrackVolumes);
      }
   __finally
      {
      LeaveCriticalSection(&m_csMixer);
      }
}
//------------------------------------------------------------------------------

//------------------------------------------------------------------------------
/// Resets track levels
//------------------------------------------------------------------------------
void TMixerForm::ResetLevelsInternal()
{
   if (Visible)
      {
      for (unsigned int n = 0; n < m_vafTrackVolumes.size(); n++)
         {
         if (n >= m_vvMixers[CT_TRACK].size())
            break;
         m_vvMixers[CT_TRACK][n]->ResetLevel();
         }
      for (unsigned int n = 0; n < m_vvMixers[CT_OUTPUT].size(); n++)
         {
         if (n >= SoundClass()->m_vfOutChannelLevel.size())
            break;
         m_vvMixers[CT_OUTPUT][n]->ResetLevel();
         }
      for (unsigned int n = 0; n < m_vvMixers[CT_INPUT].size(); n++)
         {
         if (n >= SoundClass()->m_vfInChannelLevel.size())
            break;
         m_vvMixers[CT_INPUT][n]->ResetLevel();
         }
      }

   if (SoundClass() && SoundClass()->m_pfrmTracks->Visible)
      SoundClass()->m_pfrmTracks->ResetTrackLevelsInternal();


}
//------------------------------------------------------------------------------

//------------------------------------------------------------------------------
/// Posts message for a track volume slider update
//------------------------------------------------------------------------------
void TMixerForm::UpdateTrackSliders(bool bForce)
{
   if (!Visible && !bForce)
      return;
   SetTrackSlidersInternal();
}
//------------------------------------------------------------------------------

//------------------------------------------------------------------------------
/// Sets output sliders directly: never called in processing thread
//------------------------------------------------------------------------------
void TMixerForm::UpdateOutputSliders(const std::vector<float> & vfGain, bool bForce)
{
   if (!Visible && !bForce)
      return;
   for (unsigned int n = 0; n < m_vvMixers[CT_OUTPUT].size(); n++)
      {
      if (n >= vfGain.size())
         break;
      m_vvMixers[CT_OUTPUT][n]->UpdateSlider(vfGain[n]);
      }
}
//------------------------------------------------------------------------------

//------------------------------------------------------------------------------
/// Sets output sliders directly: never called in processing thread
//------------------------------------------------------------------------------
void TMixerForm::UpdateInputSliders(const std::vector<float> & vfGain, bool bForce)
{
   if (!Visible && !bForce)
      return;
   for (unsigned int n = 0; n < m_vvMixers[CT_INPUT].size(); n++)
      {
      if (n >= vfGain.size())
         break;
      m_vvMixers[CT_INPUT][n]->UpdateSlider(vfGain[n]);
      }
}
//------------------------------------------------------------------------------

//------------------------------------------------------------------------------
/// Updates position of track sliders (called by WndProc)
//------------------------------------------------------------------------------
void TMixerForm::SetTrackSlidersInternal()
{
  for (unsigned int n = 0; n < m_vvMixers[CT_TRACK].size(); n++)
      {
      if (n >= SoundClass()->GetTrackGain().size())
         break;
      m_vvMixers[CT_TRACK][n]->UpdateSlider(SoundClass()->GetTrackGain()[n]);
      }
 }
//------------------------------------------------------------------------------

//------------------------------------------------------------------------------
/// calls UpdateButtons for all types of buttons
//------------------------------------------------------------------------------
void TMixerForm::UpdateAllButtons(bool bForce)
{
   UpdateButtons(CT_TRACK, bForce);
   UpdateButtons(CT_OUTPUT, bForce);
   UpdateButtons(CT_INPUT, bForce);
}
//------------------------------------------------------------------------------


//------------------------------------------------------------------------------
/// Updates status of buttons directly: never called in processing thread
//------------------------------------------------------------------------------
void TMixerForm::UpdateButtons(TChannelType ct, bool bForce)
{
   if (!Visible && !bForce)
      return;
   const std::valarray<bool >& vrbMute = SoundClass()->GetChannelMute(ct);
   const std::valarray<bool >& vrbSolo = SoundClass()->GetChannelSolo(ct);
   for (unsigned int n = 0; n < vrbMute.size(); n++)
      {
      if (n >= m_vvMixers[ct].size() || n >= vrbSolo.size())
         break;

      m_vvMixers[ct][n]->UpdateSolo(vrbSolo[n]);
      m_vvMixers[ct][n]->UpdateMute(vrbMute[n]);
      }
 }
//------------------------------------------------------------------------------

//------------------------------------------------------------------------------
/// Sets track mute status depending on button status
//------------------------------------------------------------------------------
void TMixerForm::SetMute(TChannelType ct, unsigned int nIndex)
{
   std::valarray<bool> vb = SoundClass()->GetChannelMute(ct);

   vb[nIndex] = m_vvMixers[ct][nIndex]->IsMuted();

   unsigned int n;

   // if Shift is pressed, we set all other buttons false!
   if (GetAsyncKeyState(VK_SHIFT)  & 0x8000)
      {
      for (n = 0; n < m_vvMixers[ct].size(); n++)
         {
         if (n != nIndex)
            vb[n] = false;
         }
      }
   // otherwise we adjust linked buttons as well
   else
      {
      // check for linked buttons to the right...
      for (n = nIndex+1; n < m_vvMixers[ct].size(); n++)
         {
         if (m_vvMixers[ct][n]->IsLink())
            vb[n] = vb[nIndex];
         else
            break;
         }
      // ... and to the left
      for (n = nIndex; n > 0; n--)
         {
         if (m_vvMixers[ct][n]->IsLink())
            vb[n-1] = vb[nIndex];
         else
            break;
            
         }
      }
   SoundClass()->SetChannelMute(vb, ct, true);
}
//------------------------------------------------------------------------------

//------------------------------------------------------------------------------
/// Sets track solo status depending on button status
//------------------------------------------------------------------------------
void TMixerForm::SetSolo(TChannelType ct, unsigned int nIndex)
{
   std::valarray<bool> vb = SoundClass()->GetChannelSolo(ct);
   vb[nIndex] = m_vvMixers[ct][nIndex]->IsSolo();

   unsigned int n;
   // if Shift is pressed, we set all other buttons false!
   if (GetAsyncKeyState(VK_SHIFT)  & 0x8000)
      {
      for (n = 0; n < m_vvMixers[ct].size(); n++)
         {
         if (n != nIndex)
            vb[n] = false;
         }
      }
   // otherwise we adjust linked buttons as well
   else
      {
      // check for linked buttons to the right...
      for (n = nIndex+1; n < m_vvMixers[ct].size(); n++)
         {
         if (m_vvMixers[ct][n]->IsLink())
            vb[n] = vb[nIndex];
         else
            break;
         }
      // ... and to the left
      for (n = nIndex; n > 0; n--)
         {
         if (m_vvMixers[ct][n]->IsLink())
            vb[n-1] = vb[nIndex];
         else
            break;
         }
      }
   SoundClass()->SetChannelSolo(vb, ct, true);
}
//------------------------------------------------------------------------------

//------------------------------------------------------------------------------
/// Sets output or track gain of all channels
//------------------------------------------------------------------------------
void TMixerForm::SetGain(TChannelType ct, unsigned int nIndex, float f)
{
   // retrieve position of slider directly to avoid rounding errors
   int nPos = m_vvMixers[ct][nIndex]->Slider->Position;

   // retrieve current gains from asio class
   std::vector<float> vfGain;
   if (ct == CT_TRACK)
      vfGain = SoundClass()->GetTrackGain();
   else if (ct == CT_OUTPUT)
      vfGain = SoundClass()->GetGain();
   else
      vfGain = SoundClass()->GetRecGain();
   // set gain in questions
   vfGain[nIndex] = f;

   // check for linked sliders to the right...
   std::vector<unsigned int> vn;
   unsigned int n;
   for (n = nIndex+1; n < m_vvMixers[ct].size(); n++)
      {
      if (m_vvMixers[ct][n]->IsLink())
         {
         vfGain[n] = f;
         // store this mixer for updating slider!
         vn.push_back(n);
         }
      else
         break;
      }
   // ... and to the left
   for (n = nIndex; n > 0; n--)
      {
      if (m_vvMixers[ct][n]->IsLink())
         {
         vfGain[n-1] = f;
         // store this mixer for updating slider!
         vn.push_back(n-1);
         }
      else
         break;
      }

   // set ne gains in asio class
   if (ct == CT_TRACK)
      SoundClass()->SetTrackGainDirect(vfGain);
   else if (ct == CT_OUTPUT)
      SoundClass()->SetGainDirect(vfGain);
   else
      SoundClass()->SetRecGain(vfGain);

   // update slider positions directly
   for (n = 0; n < vn.size(); n++)
      m_vvMixers[ct][vn[n]]->UpdateSlider(nPos);
}
//------------------------------------------------------------------------------

//------------------------------------------------------------------------------
/// toggles visibility of track mixers
//------------------------------------------------------------------------------
#pragma argsused
void __fastcall TMixerForm::pnlTracksClick(TObject *Sender)
{
   try
      {
      Application->ActivateHint(Mouse->CursorPos);
      }
   catch(...){};

   pnlTracks->Width = (pnlTracks->Width == 0) ? (int)pnlTracks->Tag : 0;
   AdjustWidth(true);
}
//------------------------------------------------------------------------------

//------------------------------------------------------------------------------
/// toggles visibility of output mixers
//------------------------------------------------------------------------------
#pragma argsused
void __fastcall TMixerForm::pnlOutputsClick(TObject *Sender)
{
   pnlOutputs->Width = (pnlOutputs->Width == 0) ? (int)pnlOutputs->Tag : 0;
   AdjustWidth(true);
}
//------------------------------------------------------------------------------

//------------------------------------------------------------------------------
/// toggles visibility of input mixers
//------------------------------------------------------------------------------
#pragma argsused
void __fastcall TMixerForm::pnlInputsClick(TObject *Sender)
{
   pnlInputs->Width = (pnlInputs->Width == 0) ? (int)pnlInputs->Tag : 0;
   AdjustWidth(true);
}
//------------------------------------------------------------------------------

//------------------------------------------------------------------------------
/// Adjusts form width
//------------------------------------------------------------------------------
void __fastcall TMixerForm::AdjustWidth(bool b)
{
   m_bCanResize = true;
   int nWidth = pnlBtnTracks->Width + pnlBtnOutputs->Width + pnlBtnInputs->Width*pnlBtnInputs->Visible
               + pnlTracks->Width + pnlOutputs->Width + pnlInputs->Width*pnlInputs->Visible;

   int nScreenWidth = Screen->Width;
   int i, nWidthTmp;
   for (i = 0; i < Screen->MonitorCount; i++)
      {
      nWidthTmp = Screen->Monitors[i]->WorkareaRect.Right - Screen->Monitors[i]->WorkareaRect.Left;
      if (Left >= Screen->Monitors[i]->Left && Left < Screen->Monitors[i]->Left + Screen->Monitors[i]->Width)
         {
         nScreenWidth = nWidthTmp;
         break;
         }
      }
   if (nWidth > nScreenWidth)
      nWidth = nScreenWidth-30;

   m_nMaxWidth = nWidth + (Width - ClientWidth);
   if (b || ClientWidth > nWidth)
      ClientWidth = nWidth;

   m_bCanResize = false;
   if (!!m_MixerWithFocus)
      {
      if (  (m_MixerWithFocus->m_ct == CT_TRACK    && !pnlTracks->Width)
         || (m_MixerWithFocus->m_ct == CT_OUTPUT   && !pnlOutputs->Width)
         )
         SetFocusToFirstSlider();
      }
}
//------------------------------------------------------------------------------

//------------------------------------------------------------------------------
/// Adjusts form width
//------------------------------------------------------------------------------
#pragma argsused
void __fastcall TMixerForm::FormCanResize(TObject *Sender, int &NewWidth,
      int &NewHeight, bool &Resize)
{
   if (!m_bCanResize)
      {
      // if not allowed (due to internal adjustment) then do NOT allow to change Height
      NewHeight = Height;
      // and do not allow wider than all sliders visible
      if (NewWidth > m_nMaxWidth)
         NewWidth = m_nMaxWidth;
      }
}
//------------------------------------------------------------------------------

//------------------------------------------------------------------------------
/// updates all controls and sets focus to first slider
//------------------------------------------------------------------------------
#pragma argsused
void __fastcall TMixerForm::FormShow(TObject *Sender)
{
   // force updating all controls to correct values
   UpdateAllButtons(true);
   UpdateTrackSliders(true);
   UpdateOutputSliders(SoundClass()->GetGain(), true);
   UpdateInputSliders(SoundClass()->GetRecGain(), true);
   pnlBtnInputs->Visible = m_vvMixers[CT_INPUT].size() > 0;
   AdjustWidth();
   SetFocusToFirstSlider();
}
//------------------------------------------------------------------------------

//------------------------------------------------------------------------------
/// sets focus to first slider if TAB pressed and no slider already has focus
//------------------------------------------------------------------------------
#pragma argsused
void __fastcall TMixerForm::FormKeyDown(TObject *Sender, WORD &Key,
      TShiftState Shift)
{
   if (Key == VK_TAB)
      {
      if (ActiveControl->Name != "Slider")
         SetFocusToFirstSlider();
      }
}
//------------------------------------------------------------------------------

//------------------------------------------------------------------------------
/// called by MixerFrames on TAB to go to next frame
//------------------------------------------------------------------------------
void TMixerForm::TabClick(TChannelType ct, int nIndex, bool bInverse)
{
   if (bInverse)
      nIndex--;
   else
      nIndex++;

   // do we have to change track/output (and can we)?
   if (nIndex < 0)
      {
      if (ct == CT_TRACK && !!pnlOutputs->Width)
         ct = CT_OUTPUT;
      else if (ct == CT_OUTPUT && !!pnlTracks->Width)
         ct = CT_TRACK;
      nIndex = (int)m_vvMixers[ct].size()-1;
      }
   else if (nIndex >= (int)m_vvMixers[ct].size())
      {
      if (ct == CT_TRACK && !!pnlOutputs->Width)
         ct = CT_OUTPUT;
      else if (ct == CT_OUTPUT && !!pnlTracks->Width)
         ct = CT_TRACK;
      nIndex = 0;
      }
   m_vvMixers[ct][(unsigned int)nIndex]->Slider->SetFocus();
   m_MixerWithFocus = m_vvMixers[ct][(unsigned int)nIndex];
}
//------------------------------------------------------------------------------

//------------------------------------------------------------------------------
/// selects first available slider
//------------------------------------------------------------------------------
void TMixerForm::SetFocusToFirstSlider()
{
   if (m_vvMixers[CT_TRACK].size() )
      {
      m_vvMixers[CT_TRACK][0]->Slider->SetFocus();
      m_MixerWithFocus = m_vvMixers[CT_TRACK][0];
      }
   else if (m_vvMixers[CT_OUTPUT].size())
      {
      m_vvMixers[CT_OUTPUT][0]->Slider->SetFocus();
      m_MixerWithFocus = m_vvMixers[CT_OUTPUT][0];
      }
   else if (m_vvMixers[CT_INPUT].size())
      {
      m_vvMixers[CT_INPUT][0]->Slider->SetFocus();
      m_MixerWithFocus = m_vvMixers[CT_INPUT][0];
      }
}
//------------------------------------------------------------------------------

//------------------------------------------------------------------------------
/// Resets clip counts
//------------------------------------------------------------------------------
void __fastcall TMixerForm::ResetClip()
{
   SoundClass()->ResetClipCount();
}
//------------------------------------------------------------------------------

//------------------------------------------------------------------------------
/// Update channel data i.e. names and routings
//------------------------------------------------------------------------------
void TMixerForm::UpdateChannelData(TChannelType ct)
{
   if (ct == CT_TRACK)
      {
      AnsiString str, strIn;
      // go through tracks
      for (unsigned int n = 0; n < m_vvMixers[CT_TRACK].size(); n++)
         {
         m_vvMixers[CT_TRACK][n]->SetMixerName(SoundClass()->m_vTracks[n]->Name());

         str = SoundClass()->m_vOutput[SoundClass()->m_vTracks[n]->ChannelIndex()]->Name();

         strIn = "";
         // retrieve input map for this track
         std::vector<unsigned int> vi = SoundClass()->GetTrackInputChannels(n);
         // go through all inputs mapped to this track

         for (unsigned int m = 0; m < vi.size(); m++)
            {
            if (!strIn.IsEmpty())
               strIn += ", ";
            strIn += SoundClass()->m_vInput[vi[m]]->Name();
            }

         m_vvMixers[CT_TRACK][n]->SetRouting(str, strIn);
         }
     }
   else if (ct == CT_OUTPUT)
      {
      for (unsigned int n = 0; n < m_vvMixers[CT_OUTPUT].size(); n++)
         {
         m_vvMixers[CT_OUTPUT][n]->SetMixerName(SoundClass()->m_vOutput[n]->Name());
         m_vvMixers[CT_OUTPUT][n]->SetRouting(SoundClass()->SoundGetActiveChannelName(n, Asio::OUTPUT));
         }
      }
   if (ct == CT_INPUT)
      {
      for (unsigned int n = 0; n < m_vvMixers[CT_INPUT].size(); n++)
         {
         m_vvMixers[CT_INPUT][n]->SetMixerName(SoundClass()->m_vInput[n]->Name());
         m_vvMixers[CT_INPUT][n]->SetRouting(SoundClass()->SoundGetActiveChannelName(n, Asio::INPUT));
         }
      }
}
//------------------------------------------------------------------------------

//------------------------------------------------------------------------------
/// Updates level data in levelmeters
//------------------------------------------------------------------------------
#pragma argsused
void __fastcall TMixerForm::LevelUpdateTimerTimer(TObject *Sender)
{
   LevelUpdateTimer->Enabled = false;
   SetLevelsInternal();
   LevelUpdateTimer->Enabled = true;
}
//------------------------------------------------------------------------------




