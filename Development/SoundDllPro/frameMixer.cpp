//------------------------------------------------------------------------------
/// \file frameMixer.cpp
/// \author Berg
/// \brief Implementation of frame class TMixerFrame used by TMixerForm
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
#include <vcl.h>
#pragma hdrstop
                                                                          
#include "frameMixer.h"
#include "SoundDllPro_Main.h"
//------------------------------------------------------------------------------
#pragma package(smart_init)
#pragma link "TrackBarEx"
#pragma link "HTLevel"
#pragma resource "*.dfm"

#pragma warn -use
//------------------------------------------------------------------------------

//------------------------------------------------------------------------------
/// static function to return height to use for mixer frame
//------------------------------------------------------------------------------
int TMixerFrame::GetHeight()
{
   return 308;
}
//------------------------------------------------------------------------------

//------------------------------------------------------------------------------
/// constructor. Sets basic properties
//------------------------------------------------------------------------------
__fastcall TMixerFrame::TMixerFrame(TComponent* Owner,
                                    TMixerForm* pMixerForm,
                                    TChannelType ct,
                                    int nIndex
                                    )
   :  TFrame(Owner),
      m_pMixerForm(pMixerForm),
      m_ct(ct)
{
   if (IsDarkTheme())
      pnlLink->Font->Color = clWhite;

   Width = pnlLevelMeter->Width + Slider->Width + 7;


   if (!nIndex)
      {
      pnlLink->Enabled = false;
      pnlLink->Caption = "";
      }
   Parent = dynamic_cast<TWinControl*>(Owner);

   if (Parent)
      {
      Parent->Width += Width;
      Left = nIndex * Width;
      }
   Name = AnsiString(ClassName()) + IntToStr(nIndex);
   Tag = nIndex;
   Slider->Tag = (NativeInt)this;
   pnlLink->Font->Size = 9;
   pnlLink->Color = pnlInfo->Color;

   SetMixerName("");
   SetRouting("");
}
//------------------------------------------------------------------------------

//------------------------------------------------------------------------------
/// Updates LevelMeter from linear level
//------------------------------------------------------------------------------
void TMixerFrame::UpdateLevel(float f, unsigned int nClip)
{
   float fdB = FactorTodB(f);
   LevelMeter->Value = (int)fdB;
   pnlClip->Color = nClip ? clRed : clMaroon;
   AnsiString str;
   str.printf("%.1f dB", (double)fdB);
   if (lbLevel->Caption != str)
      lbLevel->Caption = str;
}
//------------------------------------------------------------------------------

//------------------------------------------------------------------------------
/// adjusts gain caption below slider by log gain value
//------------------------------------------------------------------------------
void TMixerFrame::SetGainCaption(float f)
{
   if (f <= -Slider->Max)
      pnlGain->Caption = "-inf";
   else
      {
      AnsiString str;
      str.printf("%.1f", (double)f);
      pnlGain->Caption = str;
      }
}
//------------------------------------------------------------------------------

//------------------------------------------------------------------------------
/// Resets LevelMeter
//------------------------------------------------------------------------------
void TMixerFrame::ResetLevel()
{
   LevelMeter->Reset();
}
//------------------------------------------------------------------------------

//------------------------------------------------------------------------------
/// Updates slider position from linear gain
//------------------------------------------------------------------------------
void TMixerFrame::UpdateSlider(float f, bool bPropagateChange)
{
   // detach callback that calls SoundClass, if corresponding flag set!
   if (!bPropagateChange)
      {
      Slider->OnChange = NULL;
      }
   try
      {
      float fTmp = FactorTodB(f);
      Slider->Position = (int)-fTmp;
      SetGainCaption(fTmp);
      }
   __finally
      {
      if (Slider->OnChange == NULL)
         Slider->OnChange = SliderChange;
      }
}
//------------------------------------------------------------------------------

//------------------------------------------------------------------------------
/// Updates slider position directly from position
//------------------------------------------------------------------------------
void TMixerFrame::UpdateSlider(int n)
{
   // detach callback that calls SoundClass
   Slider->OnChange = NULL;
   try
      {
      Slider->Position = n;
      }
   __finally
      {
      Slider->OnChange = SliderChange;
      }
   SetGainCaption(-Slider->Position);
}
//------------------------------------------------------------------------------

//------------------------------------------------------------------------------
/// Updates mute button (panel) status
//------------------------------------------------------------------------------
void TMixerFrame::UpdateMute(bool b)
{
   pnlMute->BevelOuter = b ? bvLowered : bvRaised;
   pnlMute->Color = b ? clRed : cl3DDkShadow;
}
//------------------------------------------------------------------------------

//------------------------------------------------------------------------------
/// Updates solo button (panel) status
//------------------------------------------------------------------------------
void TMixerFrame::UpdateSolo(bool b)
{
   pnlSolo->BevelOuter = b ? bvLowered : bvRaised;
   pnlSolo->Color = b ? clYellow : cl3DDkShadow;
   pnlSolo->Font->Color = b ? clBlack : clWhite;
}
//------------------------------------------------------------------------------

//------------------------------------------------------------------------------
/// Updates link button (panel) status
//------------------------------------------------------------------------------
void TMixerFrame::UpdateLink(bool b)
{                      
   pnlLink->BevelOuter = b ? bvLowered : bvRaised;
   pnlLink->Color = b ? TColor(0x00FF8000) : pnlInfo->Color;
   if (!IsDarkTheme())
      pnlLink->Font->Color = b ? clWhite : clBlack;
}
//------------------------------------------------------------------------------

//------------------------------------------------------------------------------
/// returns mute status
//------------------------------------------------------------------------------
bool TMixerFrame::IsMuted()
{
   return pnlMute->BevelOuter == bvLowered;
}
//------------------------------------------------------------------------------

//------------------------------------------------------------------------------
/// returns solo status
//------------------------------------------------------------------------------
bool TMixerFrame::IsSolo()
{
   return pnlSolo->BevelOuter == bvLowered;
}
//------------------------------------------------------------------------------

//------------------------------------------------------------------------------
/// returns link status
//------------------------------------------------------------------------------
bool TMixerFrame::IsLink()
{
   return pnlLink->BevelOuter == bvLowered;
}
//------------------------------------------------------------------------------

//------------------------------------------------------------------------------
/// Calls SetGain of mixer on slider change
//------------------------------------------------------------------------------
#pragma argsused
void __fastcall TMixerFrame::SliderChange(TObject *Sender)
{
   static int nLastPos = Slider->Position;
   if (Slider->Position != nLastPos)
      {
      if (Slider->Position == Slider->Max)
         m_pMixerForm->SetGain(m_ct, (unsigned int)Tag, 0.0f);
      else
         m_pMixerForm->SetGain(m_ct, (unsigned int)Tag, dBToFactor(-Slider->Position));
      SetGainCaption(-Slider->Position);
      nLastPos = Slider->Position;

      if (m_pMixerForm->Tag)
         SoundClass()->ResetClipCount();
      }
}
//------------------------------------------------------------------------------

//------------------------------------------------------------------------------
/// Calls SetMute of mixer on pnl click
//------------------------------------------------------------------------------
#pragma argsused
void __fastcall TMixerFrame::pnlMuteClick(TObject *Sender)
{
   UpdateMute(pnlMute->BevelOuter == bvRaised);
   m_pMixerForm->SetMute(m_ct, (unsigned int)Tag);
}
//------------------------------------------------------------------------------

//------------------------------------------------------------------------------
/// Calls SetSolo of mixer on pnl click
//------------------------------------------------------------------------------
#pragma argsused
void __fastcall TMixerFrame::pnlSoloClick(TObject *Sender)
{
   UpdateSolo(pnlSolo->BevelOuter == bvRaised);
   m_pMixerForm->SetSolo(m_ct, (unsigned int)Tag);
   Application->ProcessMessages();
}
//------------------------------------------------------------------------------

//------------------------------------------------------------------------------
/// Updates 'button' (pnl) satus
//------------------------------------------------------------------------------
#pragma argsused
void __fastcall TMixerFrame::pnlLinkClick(TObject *Sender)
{
   UpdateLink(pnlLink->BevelOuter == bvRaised);
}
//------------------------------------------------------------------------------

//------------------------------------------------------------------------------
/// DoubleClick callback of slider's thumb. Sets Slider to 0 or last gain
/// respectively
//------------------------------------------------------------------------------
#pragma argsused
void __fastcall TMixerFrame::SliderDblClick(TObject *Sender)
{
   static int nLastPosition = Slider->Position;
   if (Slider->Position != 0)
      {
      nLastPosition = Slider->Position;
      Slider->Position = 0;
      }
   else
      Slider->Position = nLastPosition;
}
//------------------------------------------------------------------------------

//------------------------------------------------------------------------------
/// Draws slider's thumb, channel, background
//------------------------------------------------------------------------------
#pragma argsused
void __fastcall TMixerFrame::SliderCustomDraw(TTrackBarEx *Sender,
      TrackBarExItem Item, const TRect &ARect, TCustomDrawState State,
      bool &DefaultDraw)
{
   // it's the thumb
   if (Item == TrackBarEx::tbiThumb)
      {
      // calculate rect where to stretchdraw thumb image
      TRect rc = ARect;
      rc.left += Sender->Width/2 - 14;
      rc.right = rc.left + 16;
      int nMiddle = rc.top+(rc.bottom-rc.top)/2;
      Sender->Canvas->StretchDraw(rc, m_pMixerForm->ThumbImage->Picture->Bitmap);

      // draw focus-indicator or 'postion not zero'-indicator
      if (Sender->Focused())
         Sender->Canvas->Pen->Color = clRed;
      else if (Sender->Position != 0)
         Sender->Canvas->Pen->Color = clLime;
      else // otherwise: no indicator
         return;
      Sender->Canvas->MoveTo(rc.left+1, nMiddle);
      Sender->Canvas->LineTo(rc.right-1, nMiddle);
      }
   else if (Item == TrackBarEx::tbiChannel)
      {
      TRect rc = ARect;
      rc.top += 5;
      rc.bottom -= 5;
      rc.left = Sender->Width/2 - 6;
      rc.right = rc.left+3;
      DrawEdge(Sender->Canvas->Handle, &rc, EDGE_SUNKEN, BF_RECT);
      }
   else
      DefaultDraw = false;

}
//------------------------------------------------------------------------------

//------------------------------------------------------------------------------
/// Shortcuts and tabbing
//------------------------------------------------------------------------------
#pragma argsused
void __fastcall TMixerFrame::SliderKeyDown(TObject *Sender, WORD &Key,
      TShiftState Shift)
{
   if (m_ct == CT_INPUT)
      return;

   switch (Key)
      {
      case VK_SPACE:    SliderDblClick(NULL);   break;
      case 'M':         pnlMuteClick(NULL);     break;
      case 'S':         pnlSoloClick(NULL);     break;
      case 'L':         if (pnlLink->Enabled)
                           pnlLinkClick(NULL);
                        break;
      case VK_TAB:      m_pMixerForm->TabClick(m_ct, (int)Tag, Shift.Contains(ssShift));
      }
}
//------------------------------------------------------------------------------

//------------------------------------------------------------------------------
/// Resets clips
//------------------------------------------------------------------------------
#pragma argsused
void __fastcall TMixerFrame::ledClipClick(TObject *Sender)
{
   m_pMixerForm->ResetClip();
}
//------------------------------------------------------------------------------

//------------------------------------------------------------------------------
/// Sets routing captions and hints
//------------------------------------------------------------------------------
void TMixerFrame::SetRouting(AnsiString strOut, AnsiString strIn)
{
   lbRoutingIn->Caption   = strIn;
   lbRoutingIn->Hint      = strIn;
   lbRoutingOut->Caption   = strOut;
   lbRoutingOut->Hint      = strOut;
//   UnicodeString us = lbName->Caption + ": " + lbRoutingOut->Caption;
//   OutputDebugStringW(us.w_str());
}
//------------------------------------------------------------------------------

//------------------------------------------------------------------------------
/// Sets routing captions and hints
//------------------------------------------------------------------------------
void TMixerFrame::SetMixerName(AnsiString str)
{
   lbName->Caption   = str;
   lbName->Hint      = str;
}
//------------------------------------------------------------------------------

