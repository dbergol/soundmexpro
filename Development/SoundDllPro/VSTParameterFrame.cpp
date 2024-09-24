//------------------------------------------------------------------------------
/// \file VSTParameterFrame.cpp
/// \author Berg
/// \brief Implementation of class TframeVSTParam. TFrame for showing and changing
/// a VST parameter. USed by TVSTPluginEditor
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
#include <math.h>
#pragma hdrstop

#include "VSTParameterFrame.h"
// switch off clang warnings from VST-SDK
#pragma clang diagnostic push
#pragma clang diagnostic ignored "-Wreserved-id-macro"
#pragma clang diagnostic ignored "-Wundef"
#pragma clang diagnostic ignored "-Wmismatched-tags"
#pragma clang diagnostic ignored "-Wextra-semi"
#pragma clang diagnostic ignored "-Wdocumentation-unknown-command"
#include "aeffectx.h"
#pragma clang diagnostic pop // restore options
//------------------------------------------------------------------------------
#pragma package(smart_init)
#pragma resource "*.dfm"

//------------------------------------------------------------------------------

//------------------------------------------------------------------------------
/// constructor. sets position and retrieves parameter properties and value
//------------------------------------------------------------------------------
__fastcall TframeVSTParam::TframeVSTParam(TComponent* Owner, AEffect *peff, int nIndex)
   : TFrame(Owner), m_pEffect(peff)
{
   if (!peff)
      throw Exception("innvalid VST effect passed");
   Parent   = dynamic_cast<TWinControl*>(Owner);
   Name     = AnsiString(ClassName()) + IntToStr(nIndex);
   Top      = nIndex * Height;
   Tag      = nIndex;
   pb->Tag  = nIndex;
   pb->Max  = pb->Width;
   char szParamName[256]     = {0};
   char szParamLabel[256]    = {0};

   m_pEffect->dispatcher (m_pEffect, effGetParamName, nIndex, 0, szParamName, 0);
   m_pEffect->dispatcher (m_pEffect, effGetParamLabel, nIndex, 0, szParamLabel, 0);

   AnsiString s;
   s.printf("%03d", nIndex);
   sb->Panels->Items[0]->Text = s;
   sb->Panels->Items[1]->Text = Trim(AnsiString(szParamName));
   sb->Panels->Items[3]->Text = Trim(AnsiString(szParamLabel));
   UpdateValue(true);
}
//------------------------------------------------------------------------------

//------------------------------------------------------------------------------
/// Updates value and/or value display
//------------------------------------------------------------------------------
void TframeVSTParam::UpdateValue(bool bValueFromEffect)
{
   if (bValueFromEffect)
      pb->Position = (int)floor((float)pb->Width*m_pEffect->getParameter (m_pEffect, (int)Tag));
   else
      {
      float fValue = (float)pb->Position / (float)pb->Width;
      m_pEffect->setParameter(m_pEffect, (int)Tag, fValue);
      }

   char szParamDisplay[4096]  = {0};
   m_pEffect->dispatcher (m_pEffect, effGetParamDisplay, (int)Tag, 0, szParamDisplay, 0);
   sb->Panels->Items[2]->Text = Trim(AnsiString(szParamDisplay));
   ZeroMemory(szParamDisplay, 4096);
   m_pEffect->dispatcher (m_pEffect, effGetParamLabel, (int)Tag, 0, szParamDisplay, 0);
   sb->Panels->Items[3]->Text = Trim(AnsiString(szParamDisplay));
   sb->Hint = sb->Panels->Items[2]->Text;
}
//------------------------------------------------------------------------------

//------------------------------------------------------------------------------
/// stores mouse and progressbar position for adjusting it (see pbMouseMove)
//------------------------------------------------------------------------------
#pragma argsused
void __fastcall TframeVSTParam::pbMouseDown(TObject *Sender,
      TMouseButton Button, TShiftState Shift, int X, int Y)
{
   m_nXValue       = X;
   m_nOldPosition  = pb->Position;
}
//------------------------------------------------------------------------------

//------------------------------------------------------------------------------
/// sets progress bar position and updates value display
//------------------------------------------------------------------------------
#pragma argsused
void __fastcall TframeVSTParam::pbMouseMove(TObject *Sender,
      TShiftState Shift, int X, int Y)
{
   if (Shift.Contains(ssLeft))
      {
      pb->Position = m_nOldPosition + (X - m_nXValue);
      UpdateValue(false);
      }
}
//------------------------------------------------------------------------------






