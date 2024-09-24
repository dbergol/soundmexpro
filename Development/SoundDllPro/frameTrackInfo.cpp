//------------------------------------------------------------------------------
/// \file frameTrack.cpp
/// \author Berg
/// \brief Implementation of class TTrackFrame (TFrame) for visualization of
/// audio data blocks in a in track. Used by TTracksFrame
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

#include "frameTrackInfo.h"
#include "SoundDllPro_Main.h"
//------------------------------------------------------------------------------
#pragma package(smart_init)
#pragma link "HTLevel"
#pragma link "TrackBarEx"
#pragma resource "*.dfm"
//------------------------------------------------------------------------------
#pragma warn -use

//------------------------------------------------------------------------------
/// Constructor, sets parent and basic properties
//------------------------------------------------------------------------------
__fastcall TTrackInfoFrame::TTrackInfoFrame(TComponent* Owner, int nIndex)
   : TFrame(Owner)
{
   if (!SoundClass())
      throw Exception("Global SoundClass invalid");
   if (SoundClass()->m_vTracks.size() <= (unsigned int)nIndex)
      throw Exception("Global track count inconsistent");

   Name     = AnsiString(ClassName()) + IntToStr(nIndex);
   Parent   = dynamic_cast<TWinControl*>(Owner);
   Tag      = nIndex;
   // NOTE: must set top position > 0 to have alignment of multiple tracks ok!
   Top = 100;
   lbTrackNum->Caption = SoundClass()->m_vTracks[(unsigned int)Tag]->Name();
   UpdateData();
   pnlTrack->Color = TColor(0x505050);
}
//------------------------------------------------------------------------------

//------------------------------------------------------------------------------
/// Updates track info data (output channel and mixed input channels)
//------------------------------------------------------------------------------
void TTrackInfoFrame::UpdateData()
{
   if (!SoundClass())
      throw Exception("Global SoundClass invalid");
   if (SoundClass()->m_vTracks.size() <= (unsigned int)Tag)
      throw Exception("Global track count inconsistent");

   lbOut->Caption = "Out: " + SoundClass()->m_vOutput[SoundClass()->m_vTracks[(unsigned int)Tag]->ChannelIndex()]->Name();
   lbOut->Hint = lbOut->Caption;

   std::vector<unsigned int> vi = SoundClass()->GetTrackInputChannels((unsigned int)Tag);
   AnsiString str;
   for (unsigned int n = 0; n < vi.size(); n++)
      {
      if (!str.IsEmpty())
         str += ", ";
      str += SoundClass()->m_vInput[vi[n]]->Name();
      }
   if (!str.IsEmpty())
      str = "In: " + str;
   lbIn->Caption = str;
   lbIn->Hint = lbIn->Caption;
}
//------------------------------------------------------------------------------

//------------------------------------------------------------------------------
/// Updates LevelMeter
//------------------------------------------------------------------------------
void TTrackInfoFrame::UpdateLevel(float f)
{
   LevelMeter->Value = (int)FactorTodB(f);
}
//------------------------------------------------------------------------------

//------------------------------------------------------------------------------
/// Reset LevelMeter
//------------------------------------------------------------------------------
void TTrackInfoFrame::ResetLevel()
{
   LevelMeter->Reset();
}
//------------------------------------------------------------------------------

//------------------------------------------------------------------------------
/// draws scale
//------------------------------------------------------------------------------
#pragma argsused
void __fastcall TTrackInfoFrame::pbPaint(TObject *Sender)
{
   pb->Canvas->Brush->Color = pb->Color;
   pb->Canvas->Brush->Color = TColor(0x808080);
   pb->Canvas->FillRect(pb->BoundsRect);
   pb->Canvas->Font->Color = clWhite;
   pb->Canvas->Font->Size = 6;
   pb->Canvas->TextOut(pb->Width/4-pb->Canvas->TextWidth("-75"), -1, "-75");
   pb->Canvas->TextOut(pb->Width/2-pb->Canvas->TextWidth("-45"), -1, "-55");
   pb->Canvas->TextOut(3*pb->Width/4-pb->Canvas->TextWidth("-15"), -1, "-15");

   pb->Canvas->Font->Color = clRed;
   pb->Canvas->TextOut(pb->Width-6, -1, "0");

}
//------------------------------------------------------------------------------

