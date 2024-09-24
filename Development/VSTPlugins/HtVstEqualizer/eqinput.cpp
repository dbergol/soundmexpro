//------------------------------------------------------------------------------
/// \file eqinput.cpp
///
/// \author Berg
/// \brief Implementation form for displayling and editing spectral filters
///
/// Project SoundMexPro
/// Module  HtVSTEqualizer.dll
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
#include <limits.h>
#pragma hdrstop

#include "eqinput.h"
#include "AHtVSTEqualizer.h"

//---------------------------------------------------------------------------
#pragma package(smart_init)

#pragma link "Chart"
#pragma link "Series"
#pragma link "TeEngine"
#pragma link "TeeProcs"
#pragma resource "*.dfm"
//------------------------------------------------------------------------------


//------------------------------------------------------------------------------
///
//------------------------------------------------------------------------------
__fastcall TfrmEqInput::TfrmEqInput(CHtVSTEqualizer *pEqualizer)
   : TForm((void*)NULL), m_pEqualizer(pEqualizer)
{
   if (!m_pEqualizer)
      throw Exception("Error: invalid pointer passed to EqInput");
   FilterSeriesL->XValues->Order    = loAscending;
   FilterSeriesR->XValues->Order    = loAscending;
   PreSpecSeriesL->XValues->Order   = loAscending;
   PreSpecSeriesR->XValues->Order   = loAscending;
   PostSpecSeriesL->XValues->Order  = loAscending;
   PostSpecSeriesR->XValues->Order  = loAscending;
   FileFilterL->XValues->Order      = loAscending;
   FileFilterR->XValues->Order      = loAscending;
   FileFilterL->Active              = false;
   FileFilterR->Active              = false;

   AnsiString s;
   iOldXPos = -1;
   iCapturedValue = -1;
   bChanged = false;
}
//------------------------------------------------------------------------------

//------------------------------------------------------------------------------
///
//------------------------------------------------------------------------------
#pragma argsused
void __fastcall TfrmEqInput::btnLoadClick(TObject *Sender)
{
   Chart->Enabled = false;
   DisableSpectra();
   try
      {
      DisableSpectra();
      //FormStyle = fsNormal;
      if (od->Execute())
         m_pEqualizer->LoadFilter(od->FileName);
      //FormStyle = fsStayOnTop;
      }
   __finally
      {
      Application->ProcessMessages();
      EnableSpectra();
      Chart->Enabled = true;
      }
}
//------------------------------------------------------------------------------

//------------------------------------------------------------------------------
///
//------------------------------------------------------------------------------
#pragma argsused
void __fastcall TfrmEqInput::btnSaveClick(TObject *Sender)
{
   Chart->Enabled = false;
   DisableSpectra();
   try
      {
      //FormStyle = fsNormal;
      if (sd->Execute())
         m_pEqualizer->SaveFilter(sd->FileName);
      //FormStyle = fsStayOnTop;
      }
   __finally
      {
      Application->ProcessMessages();
      EnableSpectra();
      Chart->Enabled = true;
      }

}
//------------------------------------------------------------------------------

//------------------------------------------------------------------------------
///
//------------------------------------------------------------------------------
#pragma argsused
void __fastcall TfrmEqInput::btnZoomClick(TObject *Sender)
{
   Chart->AllowZoom = btnZoom->Down;
}
//------------------------------------------------------------------------------

//------------------------------------------------------------------------------
///
//------------------------------------------------------------------------------
#pragma argsused
void __fastcall TfrmEqInput::btnPaintClick(TObject *Sender)
{
   Chart->AllowZoom = btnZoom->Down;
}
//------------------------------------------------------------------------------

//------------------------------------------------------------------------------
///
//------------------------------------------------------------------------------
#pragma argsused
void __fastcall TfrmEqInput::ChartMouseMove(TObject *Sender,
      TShiftState Shift, int X, int Y)
{
   // painting filter
   if ( PtInRect((RECT*)&Chart->ChartRect, Point(X-Chart->Width3D,Y+Chart->Height3D)))
      {
      double tmpX,tmpY;
      // set label text
      FilterSeriesL->GetCursorValues(tmpX,tmpY);  // <-- get values under mouse cursor
      lbXPos->Caption = IntToStr((int)floor(Max(0.0,tmpX))) + " Hz";
      lbYPos->Caption = FormatFloat("#.#", FactorTodB(tmpY)) + " dB";

   //   lbPosition->Caption = FilterSeriesL->GetVertAxis->LabelValue(tmpX)
   //      + ", " + FilterSeriesL->GetHorizAxis->LabelValue(FactorTodB(tmpY));

      if (!btnPaint->Enabled)
         return;
      if (Shift.Contains(ssRight) || Shift.Contains(ssLeft) )
         {
         TFastLineSeries *tl = FilterSeriesL;
         if (Shift.Contains(ssRight))
            tl = FilterSeriesR;

         if (btnPaint->Down)
            {

            int iPos = -1;
            for (int i = 0; i < tl->XValues->Count; i++)
               {
               if (tmpX < tl->XValues->Value[i])
                  {
                  iPos = tl->XValues->Locate(tl->XValues->Value[i]);
                  break;
                  }
               }

            if (iPos != -1)
               {
               bChanged = true;
               if (iOldXPos == iPos || iOldXPos == -1)
                  tl->YValues->Value[iPos] = tmpY;
               else
                  {
                  int iTmp = tl->XValues->Locate(tl->XValues->Value[iOldXPos]);
                  float f4Step   = (tl->YValues->Value[iTmp] - tmpY) / float(iPos - iOldXPos);
                  float f4Val    = tl->YValues->Value[iTmp];

                  if (iOldXPos < iPos)
                     {
                     for (int i = iOldXPos; i <= iPos; i++)
                        {
                        // iTmp = tl->XValues->Locate(tl->XValues->Value[i]);
                        tl->YValues->Value[i] = f4Val;
                        f4Val -= f4Step;
                        }
                     }
                  else
                     {
                     for (int i = iOldXPos; i >= iPos; i--)
                        {
                        // iTmp = tl->XValues->Locate(tl->XValues->Value[i]);
                        tl->YValues->Value[i] = f4Val;
                        f4Val += f4Step;
                        }
                     }

                  }
               iOldXPos = iPos;
               tl->Repaint();
               }
            }
         else if (btnShift->Down && iCapturedValue > 0)
            {
            double tmpX,tmpY;
            tl->GetCursorValues(tmpX,tmpY);
            for (int i = 0; i < tl->YValues->Count; i++)
               {
               tl->YValues->Value[i] = tl->YScreenToValue(
                                       tl->CalcYPosValue(tl->YValues->Value[i])
                                       + Y - iCapturedValue
                                       );
               }
            bChanged = true;
            iCapturedValue = Y;
            tl->Repaint();
            }
         }
      }
}
//------------------------------------------------------------------------------

//------------------------------------------------------------------------------
///
//------------------------------------------------------------------------------
#pragma argsused
void __fastcall TfrmEqInput::ChartMouseUp(TObject *Sender,
      TMouseButton Button, TShiftState Shift, int X, int Y)
{
   iOldXPos = -1;
   iCapturedValue = -1;
   if (bChanged)
      m_pEqualizer->ChartToFilter();
   bChanged = false;
}
//------------------------------------------------------------------------------

//------------------------------------------------------------------------------
///
//------------------------------------------------------------------------------
#pragma argsused
void __fastcall TfrmEqInput::btnFlatClick(TObject *Sender)
{
   for (int i = 0; i < FilterSeriesL->XValues->Count; i++)
      {
      FilterSeriesL->YValues->Value[i] = 1;
      FilterSeriesR->YValues->Value[i] = 1;
      }
   FilterSeriesL->Repaint();
   FilterSeriesR->Repaint();
   m_pEqualizer->ChartToFilter();
}
//------------------------------------------------------------------------------

//------------------------------------------------------------------------------
///
//------------------------------------------------------------------------------
#pragma argsused
void __fastcall TfrmEqInput::ChartMouseDown(TObject *Sender,
      TMouseButton Button, TShiftState Shift, int X, int Y)
{
   if (  (PtInRect((RECT*)&Chart->ChartRect, Point(X-Chart->Width3D,Y+Chart->Height3D)))
      && (btnShift->Down)
      )
      {
      iCapturedValue = Y;
      }
   else if (Button == mbRight)
      Chart->UndoZoom();
}
//------------------------------------------------------------------------------

//------------------------------------------------------------------------------
///
//------------------------------------------------------------------------------
#pragma argsused
void __fastcall TfrmEqInput::btnUndoZoomClick(TObject *Sender)
{
   Chart->UndoZoom();
}
//------------------------------------------------------------------------------

//------------------------------------------------------------------------------
///
//------------------------------------------------------------------------------
#pragma argsused
void __fastcall TfrmEqInput::btnFastClick(TObject *Sender)
{
   m_pEqualizer->m_nUpdateInterval = btnFast->Down ? 15 : 50;
}
//------------------------------------------------------------------------------

//------------------------------------------------------------------------------
///
//------------------------------------------------------------------------------
#pragma argsused
void __fastcall TfrmEqInput::btnSpecClick(TObject *Sender)
{
   EnableSpectra();
}
//------------------------------------------------------------------------------

//------------------------------------------------------------------------------
///
//------------------------------------------------------------------------------
#pragma argsused
void __fastcall TfrmEqInput::btnLeftClick(TObject *Sender)
{
   PreSpecSeriesL->Active  = btnSpec->Down && btnLeft->Down && btnInput->Down;
   PostSpecSeriesL->Active = btnSpec->Down && btnLeft->Down && btnOutput->Down;
}
//------------------------------------------------------------------------------

//------------------------------------------------------------------------------
///
//------------------------------------------------------------------------------
#pragma argsused
void __fastcall TfrmEqInput::btnRightClick(TObject *Sender)
{
   PreSpecSeriesR->Active  = btnSpec->Down && btnRight->Down && btnInput->Down;
   PostSpecSeriesR->Active = btnSpec->Down && btnRight->Down && btnOutput->Down;

}
//------------------------------------------------------------------------------

//------------------------------------------------------------------------------
///
//------------------------------------------------------------------------------
#pragma argsused
void __fastcall TfrmEqInput::btnInputClick(TObject *Sender)
{
   PreSpecSeriesL->Active  = btnSpec->Down && btnLeft->Down && btnInput->Down;
   PreSpecSeriesR->Active  = btnSpec->Down && btnRight->Down && btnInput->Down;
}
//------------------------------------------------------------------------------

//------------------------------------------------------------------------------
///
//------------------------------------------------------------------------------
#pragma argsused
void __fastcall TfrmEqInput::btnOutputClick(TObject *Sender)
{
   PostSpecSeriesL->Active = btnSpec->Down && btnLeft->Down && btnOutput->Down;
   PostSpecSeriesR->Active = btnSpec->Down && btnRight->Down && btnOutput->Down;
}
//------------------------------------------------------------------------------

//------------------------------------------------------------------------------
///
//------------------------------------------------------------------------------
void __fastcall TfrmEqInput::DisableSpectra()
{
   PreSpecSeriesL->Active  = false;
   PostSpecSeriesL->Active = false;
   PreSpecSeriesR->Active  = false;
   PostSpecSeriesR->Active = false;
}
//------------------------------------------------------------------------------

//------------------------------------------------------------------------------
///
//------------------------------------------------------------------------------
void __fastcall TfrmEqInput::EnableSpectra()
{
   PreSpecSeriesL->Active  = btnSpec->Down && btnLeft->Down && btnInput->Down;
   PostSpecSeriesL->Active = btnSpec->Down && btnLeft->Down && btnOutput->Down;
   PreSpecSeriesR->Active  = btnSpec->Down && btnRight->Down && btnInput->Down;
   PostSpecSeriesR->Active = btnSpec->Down && btnRight->Down && btnOutput->Down;
}
//------------------------------------------------------------------------------


//------------------------------------------------------------------------------
///
//------------------------------------------------------------------------------
void __fastcall TfrmEqInput::Initialize(unsigned int nFFTLen, float fSampleRate)
{
   float f4BinScale  = fSampleRate / (float)nFFTLen;
   PreSpecSeriesL->Clear();
   PreSpecSeriesR->Clear();
   PostSpecSeriesL->Clear();
   PostSpecSeriesR->Clear();
   FilterSeriesL->Clear();
   FilterSeriesR->Clear();
   float f4X = 0.0f;
   for (unsigned int i = 0; i < nFFTLen/2; i++)
      {
      f4X += f4BinScale;
      PreSpecSeriesL->AddXY(f4X, 1, "", clTeeColor);
      PreSpecSeriesR->AddXY(f4X, 1, "", clTeeColor);
      PostSpecSeriesL->AddXY(f4X, 1, "", clTeeColor);
      PostSpecSeriesR->AddXY(f4X, 1, "", clTeeColor);
      FilterSeriesL->AddXY(f4X, 1, "", clTeeColor);
      FilterSeriesR->AddXY(f4X, 1, "", clTeeColor);
      }

   Chart->BottomAxis->Maximum = fSampleRate/2.0;

   //disable right spectra, if Mono
   EnableSpectra();
}
//------------------------------------------------------------------------------

//------------------------------------------------------------------------------
///
//------------------------------------------------------------------------------
void __fastcall TfrmEqInput::SetFilterProperties()
{
   btnPaint->Enabled = !m_pEqualizer->m_bComplex;
   btnShift->Enabled = !m_pEqualizer->m_bComplex;
   btnFlat->Enabled = !m_pEqualizer->m_bComplex;
   if (m_pEqualizer->m_bComplex)
      sb->Panels->Items[1]->Text = "complex filter (read only)";
   else
      sb->Panels->Items[1]->Text = "real filter";

   AnsiString str;
   str.printf( "FFT: %d, Window: %d, Shift: 1/%d",
               m_pEqualizer->m_nFFTLen,
               m_pEqualizer->m_nWindowLen,
               1 << m_pEqualizer->m_nFWindowFeed+1
               );

   sb->Panels->Items[0]->Text = str;
}
#pragma argsused
void __fastcall TfrmEqInput::btnMuteClick(TObject *Sender)
{
   m_pEqualizer->m_bMuted = btnMute->Down;
}
//---------------------------------------------------------------------------
#pragma argsused
void __fastcall TfrmEqInput::btnSkipClick(TObject *Sender)
{
   m_pEqualizer->m_fEnabled = btnSkip->Down ? 0.0f : 1.0f;
}
//---------------------------------------------------------------------------


#pragma argsused
void __fastcall TfrmEqInput::ChartBeforeDrawAxes(TObject *Sender)
{
   Chart->Canvas->Brush->Color = clBlack;
   Chart->Canvas->Pen->Color = clBlack;
   Chart->Canvas->FillRect(Chart->ChartRect);
}
//---------------------------------------------------------------------------

