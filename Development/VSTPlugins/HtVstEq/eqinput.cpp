//------------------------------------------------------------------------------
/// \file eqinput.cpp
///
/// \author Berg
/// \brief Implementation form for displayling and editing spectral filters
///
/// Project SoundMexPro
/// Module  HtVSTEq.dll
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
#include "AHtVSTEq.h"

//------------------------------------------------------------------------------
#pragma package(smart_init)

#pragma resource "*.dfm"
//------------------------------------------------------------------------------

//------------------------------------------------------------------------------
/// constructor. initializes members
//------------------------------------------------------------------------------
__fastcall TfrmEqInput::TfrmEqInput(CHtVSTEq *pEqualizer)
   : TForm((TComponent*)NULL), m_pEqualizer(pEqualizer)
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
   m_nFFTLen = 0;
   m_fSampleRate = 0.0f;

   SetPosCaption(0, 1);
}
//------------------------------------------------------------------------------

//------------------------------------------------------------------------------
/// OnClick callback for load button. Loads filter from file
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
         m_pEqualizer->LoadFilterFile(od->FileName);
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
/// OnClick callback for save button. Saves filter to file
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
/// OnClick callback of zoom button. Allows/disallows zooming of chart
//------------------------------------------------------------------------------
#pragma argsused
void __fastcall TfrmEqInput::btnZoomClick(TObject *Sender)
{
   Chart->AllowZoom = btnZoom->Down;
}
//------------------------------------------------------------------------------

//------------------------------------------------------------------------------
/// Enables drawing (to be sure re-enables zoom) 
//------------------------------------------------------------------------------
#pragma argsused
void __fastcall TfrmEqInput::btnPaintClick(TObject *Sender)
{
   Chart->AllowZoom = btnZoom->Down;
}
//------------------------------------------------------------------------------


//------------------------------------------------------------------------------
/// Writes current mouse position to footer of chart
//------------------------------------------------------------------------------
void  TfrmEqInput::SetPosCaption(double dX, double dY)
{
   int n = (int)floor(dX);
   if (n < 0)
      n = 0;
   UnicodeString us;
   us.printf(L"Position:     %d Hz  %d dB", n, (int)floor(FactorTodB((float)dY)));
   Chart->Foot->Text->Text = us;
}
//------------------------------------------------------------------------------

//------------------------------------------------------------------------------
/// OnMouseMove callback of chart: paints new filter to chart (if enabled at all!)
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
      SetPosCaption(tmpX, tmpY);

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
                  float f4Step   = (float)(tl->YValues->Value[iTmp] - tmpY) / float(iPos - iOldXPos);
                  float f4Val    = (float)tl->YValues->Value[iTmp];

                  if (iOldXPos < iPos)
                     {
                     for (int i = iOldXPos; i <= iPos; i++)
                        {
                        // iTmp = tl->XValues->Locate(tl->XValues->Value[i]);
                        tl->YValues->Value[i] = (double)f4Val;
                        f4Val -= f4Step;
                        }
                     }
                  else
                     {
                     for (int i = iOldXPos; i >= iPos; i--)
                        {
                        // iTmp = tl->XValues->Locate(tl->XValues->Value[i]);
                        tl->YValues->Value[i] = (double)f4Val;
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
/// OnMouseUp callback of chart: stops painting new filter and applies it
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
/// OnMouseDown callback of chart: starts painting of new filter
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
/// OnClick callback of 'flat' button: resets filter to 0 dB
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
/// OnClick callback of UndoZoom button: resets zoom
//------------------------------------------------------------------------------
#pragma argsused
void __fastcall TfrmEqInput::btnUndoZoomClick(TObject *Sender)
{
   Chart->UndoZoom();
}
//------------------------------------------------------------------------------

//------------------------------------------------------------------------------
/// OnClick callback of 'fast' button, Toggles update speed of spectra
//------------------------------------------------------------------------------
#pragma argsused
void __fastcall TfrmEqInput::btnFastClick(TObject *Sender)
{
   m_pEqualizer->m_nUpdateInterval = btnFast->Down ? 15 : 50;
}
//------------------------------------------------------------------------------

//------------------------------------------------------------------------------
/// OnClick callback of spectrum button: calls EnableSpectra 
//------------------------------------------------------------------------------
#pragma argsused
void __fastcall TfrmEqInput::btnSpecClick(TObject *Sender)
{
   EnableSpectra();
}
//------------------------------------------------------------------------------

//------------------------------------------------------------------------------
/// OnClick callback of btnLeft: enables/disables display of left channel
//------------------------------------------------------------------------------
#pragma argsused
void __fastcall TfrmEqInput::btnLeftClick(TObject *Sender)
{
   PreSpecSeriesL->Active  = btnSpec->Down && btnLeft->Down && btnInput->Down;
   PostSpecSeriesL->Active = btnSpec->Down && btnLeft->Down && btnOutput->Down;
}
//------------------------------------------------------------------------------

//------------------------------------------------------------------------------
/// OnClick callback of btnRight: enables/disables display of right channel
//------------------------------------------------------------------------------
#pragma argsused
void __fastcall TfrmEqInput::btnRightClick(TObject *Sender)
{
   PreSpecSeriesR->Active  = btnSpec->Down && btnRight->Down && btnInput->Down;
   PostSpecSeriesR->Active = btnSpec->Down && btnRight->Down && btnOutput->Down;
}
//------------------------------------------------------------------------------

//------------------------------------------------------------------------------
/// OnClick callback of btnInput:  enables/disables display of filter input data
//------------------------------------------------------------------------------
#pragma argsused
void __fastcall TfrmEqInput::btnInputClick(TObject *Sender)
{
   PreSpecSeriesL->Active  = btnSpec->Down && btnLeft->Down && btnInput->Down;
   PreSpecSeriesR->Active  = btnSpec->Down && btnRight->Down && btnInput->Down;
}
//------------------------------------------------------------------------------

//------------------------------------------------------------------------------
/// OnClick callback of btnOutput:  enables/disables display of filter output data
//------------------------------------------------------------------------------
#pragma argsused
void __fastcall TfrmEqInput::btnOutputClick(TObject *Sender)
{
   PostSpecSeriesL->Active = btnSpec->Down && btnLeft->Down && btnOutput->Down;
   PostSpecSeriesR->Active = btnSpec->Down && btnRight->Down && btnOutput->Down;
}
//------------------------------------------------------------------------------

//------------------------------------------------------------------------------
/// disables spectra (in and out)
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
/// enables spectra (in and out) with respect to button states
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
/// Initializes all chart series
//------------------------------------------------------------------------------
void __fastcall TfrmEqInput::Initialize(unsigned int nFFTLen, float fSampleRate)
{
   // (re-)initialize only if properties have changed
   if (m_nFFTLen == nFFTLen && (int)m_fSampleRate == (int)fSampleRate)
      return;

   m_nFFTLen = nFFTLen;
   m_fSampleRate = fSampleRate;

   double dBinScale  = (double)fSampleRate / (double)nFFTLen;
   PreSpecSeriesL->Clear();
   PreSpecSeriesR->Clear();
   PostSpecSeriesL->Clear();
   PostSpecSeriesR->Clear();
   FilterSeriesL->Clear();
   FilterSeriesR->Clear();
   double d4X = 0.0;
   for (unsigned int i = 0; i < nFFTLen/2; i++)
      {
      d4X += dBinScale;
      PreSpecSeriesL->AddXY(d4X, 1, "", clTeeColor);
      PreSpecSeriesR->AddXY(d4X, 1, "", clTeeColor);
      PostSpecSeriesL->AddXY(d4X, 1, "", clTeeColor);
      PostSpecSeriesR->AddXY(d4X, 1, "", clTeeColor);
      FilterSeriesL->AddXY(d4X, 1, "", clTeeColor);
      FilterSeriesR->AddXY(d4X, 1, "", clTeeColor);
      }

   Chart->BottomAxis->Maximum = (double)fSampleRate/2.0;

   // call EnableSpectra to disable right spectra, if Mono!
   EnableSpectra();
}
//------------------------------------------------------------------------------

//------------------------------------------------------------------------------
/// sets filter properties and shows them in status bar
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
   str.printf( "FFT: %d", m_pEqualizer->m_nFFTLen);

   sb->Panels->Items[0]->Text = str;
}
//------------------------------------------------------------------------------

//------------------------------------------------------------------------------
/// OnClick callback of btnMute: toggles mute status
//------------------------------------------------------------------------------
#pragma argsused
void __fastcall TfrmEqInput::btnMuteClick(TObject *Sender)
{
   m_pEqualizer->m_bMuted = btnMute->Down;
}
//------------------------------------------------------------------------------

//------------------------------------------------------------------------------
/// OnClick callback of btnSkip: toggles bypass of filter
//------------------------------------------------------------------------------
#pragma argsused
void __fastcall TfrmEqInput::btnSkipClick(TObject *Sender)
{
   m_pEqualizer->m_fEnabled = btnSkip->Down ? 0.0f : 1.0f;
}
//------------------------------------------------------------------------------

//------------------------------------------------------------------------------
/// OnBeforeDrawAxes of chart: paints chart rect black
//------------------------------------------------------------------------------
#pragma argsused
void __fastcall TfrmEqInput::ChartBeforeDrawAxes(TObject *Sender)
{
   Chart->Canvas->Brush->Color = clBlack;
   Chart->Canvas->Pen->Color = clBlack;
   Chart->Canvas->FillRect(Chart->ChartRect);
}
//---------------------------------------------------------------------------

//------------------------------------------------------------------------------
/// timer callback: updates equalizer
//------------------------------------------------------------------------------
#pragma argsused
void __fastcall TfrmEqInput::UpdateTimerTimer(TObject *Sender)
{
   UpdateTimer->Enabled = false;
   if (UpdateTimer->Tag)
      m_pEqualizer->Update();

   UpdateTimer->Tag = 0;
   UpdateTimer->Enabled = true;
}
//---------------------------------------------------------------------------

//------------------------------------------------------------------------------
/// OnClick callback of cbLogFreq: toggles X-Axis lin/log
//------------------------------------------------------------------------------
#pragma argsused
void __fastcall TfrmEqInput::cbLogFreqClick(TObject *Sender)
{
   Chart->BottomAxis->Logarithmic = cbLogFreq->Checked;

   Chart->BottomAxis->Minimum = Chart->BottomAxis->Logarithmic ? FilterSeriesL->XValues->Value[0] : 0;
}
//---------------------------------------------------------------------------

