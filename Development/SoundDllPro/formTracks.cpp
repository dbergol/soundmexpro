//------------------------------------------------------------------------------
/// \file formTrack.cpp
/// \author Berg
/// \brief Implementation of class TTrackForm for visualization of audio tracks
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
#include <limits.h>
#pragma hdrstop
#include "formTracks.h"
#include "frameTrackInfo.h"
#include "SoundDllPro_Main.h"
#include "SoundDllPro_Tools.h"
//---------------------------------------------------------------------------
#pragma package(smart_init)
#pragma resource "*.dfm"
#pragma warn -use
#pragma warn -aus
extern bool g_bUseRamps;
//------------------------------------------------------------------------------
/// local prototype
AnsiString GUIDToName(AnsiString str);

//---------------------------------------------------------------------------
/// converts GUID to valid control name (must not contain '-')
//---------------------------------------------------------------------------
AnsiString GUIDToName(AnsiString str)
{
   str = str.SubString(2, str.Length()-2);
   for (int i = 0; i < str.Length(); i++)
      {
      if (str[i+1] == '-')
         str[i+1] = '_';
      }
   return str;
}
//---------------------------------------------------------------------------

//------------------------------------------------------------------------------
/// Constructor. Sets defaults
//------------------------------------------------------------------------------
__fastcall TTracksForm::TTracksForm(TComponent* Owner)
   :  TForm(Owner),
      m_bShowWaveData(false),
      m_bBreak(false),
      m_bPositionsRead(false)
{
   InitializeCriticalSection(&m_csTrackView);
    // don't know why, but must set font sizes again here: in object inspector
   // is not sufficient...
   sbStatus->Font->Size = 7;
   chrtTrack->OnBeforeDrawSeries = chrtTrackBeforeDrawSeries;
   CursorSeries->AddXY(0,0);
   CursorSeries->AddXY(1,1);

   m_bIsDarkTheme = IsDarkTheme();
   if (m_bIsDarkTheme)
      {
      chrtTrack->Color = clBlack;
      chrtTrack->TopAxis->LabelsFont->Color = clWhite;
      chrtTrack->TopAxis->Ticks->Color = clWhite;
      }
   m_bSeriesDarkTheme = m_bIsDarkTheme;
   m_bSeriesDarkTheme = false; 

   #ifdef CURSORBYLINE
   // see below in SetCursorPosInternal !
   pbCursor->Visible = false;
   #endif
   pbCursor->Canvas->Pen->Mode = pmNot;
}
//------------------------------------------------------------------------------

//------------------------------------------------------------------------------
/// Destructor (cleanup)
//------------------------------------------------------------------------------
__fastcall TTracksForm::~TTracksForm()
{
   if (m_bPositionsRead)
      {
      try
         {
         SetProfileString("TrackViewLeft",   Left);
         SetProfileString("TrackViewTop",    Top);
         SetProfileString("TrackViewHeight", Height);
         SetProfileString("TrackViewWidth",  Width);
         SetProfileString("TrackViewSamples",  miSamples->Checked);
         }
      // clear exception silently
      catch (...)
         {
         }
   }
   Clear();
   DeleteCriticalSection(&m_csTrackView);
}
//------------------------------------------------------------------------------

//------------------------------------------------------------------------------
/// cleanup of chart series
//------------------------------------------------------------------------------
void __fastcall TTracksForm::Clear()
{
   while (chrtTrack->SeriesList->Count > 0)
      RemoveChartSeries(chrtTrack->Series[0]);
}
//------------------------------------------------------------------------------

//------------------------------------------------------------------------------
/// (Re-)Initializes form with tracks and cursor
//------------------------------------------------------------------------------
void TTracksForm::Init()
{
   EnterCriticalSection(&m_csTrackView);
   miRefresh->Enabled = false;
   try
      {
      // return if already initialized
      if (m_vTrackInfos.size() > 0)
         return;
      if (!SoundClass())
         throw Exception("Global SoundClass invalid");
      if (SoundClass()->DeviceIsRunning() && !SoundClass()->Paused())
         throw Exception("command not allowed if device is running");
      bool bShowSamples = false;
      m_bPositionsRead = true;
      try
         {
         Left           = GetProfileStringDefault("TrackViewLeft", 0);
         Top            = GetProfileStringDefault("TrackViewTop", 0);
         Height         = GetProfileStringDefault("TrackViewHeight", 500);
         Width          = GetProfileStringDefault("TrackViewWidth", 900);
         bShowSamples   = GetProfileStringDefault("TrackViewSamples",  0) == 1;
         }
      catch (...)
         {
         }
      m_dSampleRate        = SoundClass()->SoundGetSampleRate(); // SoundClass()->m_dSampleRate;
      m_nXViewStart        = 0;
      m_nXViewLen          = (int64_t)(10.0*m_dSampleRate);
      m_nXTotalLen         = m_nXViewLen;
      m_nNumTracks         = (unsigned int)SoundClass()->m_vTracks.size();
      m_nNumVisibleTracks  = m_nNumTracks;
      scbVert->Position    = 0;
      scbVert->Max         = 1;
      // total length constraint: INT_MAX samples or 12 hours (minimum wins)
      m_nXMaxTotalLen = INT_MAX;
      if ((double)m_nXMaxTotalLen / m_dSampleRate  > 43200)
         m_nXMaxTotalLen = (int64_t)(43200* m_dSampleRate);

      // remove existing track infos
      unsigned int i;
      for (i = 0; i < m_vTrackInfos.size(); i++)
         TRYDELETENULL(m_vTrackInfos[i]);
      m_vTrackInfos.clear();
      // create new track infos
      for (i = 0; i < m_nNumTracks; i++)
         m_vTrackInfos.push_back(new TTrackInfoFrame(pnlTrackInfo, (int)i));
      chrtTrack->LeftAxis->Maximum  = 2* m_nNumTracks;
      chrtTrack->RightAxis->Maximum = 2* m_nNumTracks;
      m_nCursorPosition = 0;
      pnlTracksResize(NULL);
      if (bShowSamples)
         miSamplesClick(NULL);
      else
         miTimeClick(NULL);
      AdjustHorizontalView();
      }
   __finally
      {
      miRefresh->Enabled = true;
      LeaveCriticalSection(&m_csTrackView);
      CursorTimer->Enabled = true;
      }
}
//------------------------------------------------------------------------------

//------------------------------------------------------------------------------
/// updates track content of all tracks
//------------------------------------------------------------------------------
void TTracksForm::UpdateTracks(bool bZoomToAll, bool bShowWaveData)
{
   if (!TryEnterCriticalSection(&m_csTrackView))
      return;
   miRefresh->Enabled = false;
   m_bShowWaveData = bShowWaveData;
   m_bBreak = false;
   TCursor cr  = Screen->Cursor;
   Screen->Cursor = crHourGlass;
   Application->ProcessMessages();
   try
      {
         
      // UpdateData returns 'largest sample number
      uint64_t nSamples = UpdateData();

      if (nSamples < 10 * m_dSampleRate)
         nSamples =  (uint64_t)(10 * m_dSampleRate);
      m_nXTotalLen   = (int64_t)nSamples;
      m_nXMaxDataLen = (int64_t)nSamples;

      UpdateTrackInfos();
      SetCursorPosInternal();

      if (!Visible)
         Show();
      if (bZoomToAll)
         btnHZoomAllClick(NULL);
      else
         AdjustHorizontalView();
      // wait for painting done!
      Application->ProcessMessages();

      // then draw sample values if requested
      if (bShowWaveData)
         UpdateWaveData();
      }
   __finally
      {
      miRefresh->Enabled = true;
      Screen->Cursor = cr;
      LeaveCriticalSection(&m_csTrackView);
      }
}
//------------------------------------------------------------------------------

//------------------------------------------------------------------------------
/// updates track track info of all tracks
//------------------------------------------------------------------------------
void TTracksForm::UpdateTrackInfos()
{
   if (!TryEnterCriticalSection(&m_csTrackView))
      return;
   try
      {
      if (!m_vTrackInfos.size())
         return;
      for (unsigned int n = 0; n < m_vTrackInfos.size(); n++)
         m_vTrackInfos[n]->UpdateData();
      }
   __finally
      {
      LeaveCriticalSection(&m_csTrackView);
      }
}
//------------------------------------------------------------------------------

//------------------------------------------------------------------------------
/// public function to set cursor position: sets private position member and
/// updates cursor position
//------------------------------------------------------------------------------
void TTracksForm::SetCursorPos(int64_t nCursorPos)
{
   if (!m_vTrackInfos.size())
      return;
   m_nCursorPosition = nCursorPos;
}
//------------------------------------------------------------------------------

//------------------------------------------------------------------------------
/// internal function to set cursor position, called by CursorTimerTimer
//------------------------------------------------------------------------------
void TTracksForm::SetCursorPosInternal()
{
   if (!Visible)
      return;
   pnlBottomLeft->Caption = CursorPosition();

   // 'scroll' window if cursor at the right of current view (only if playing)
   if (SoundClass()->DeviceIsRunning() && m_nCursorPosition > (m_nXViewStart + m_nXViewLen))
      {
      m_nXViewStart = m_nCursorPosition;
      AdjustHorizontalView();
      }

   // CURSORBYLINE is more efficient, since only a line is drawn in NOT mode, but it's
   // error prone: lines keep existing, even if they shouldn't....
   #ifdef CURSORBYLINE
   // calculate cursor position in window
   // NOTE: here we paint a line by hand and do NOT use the cursor series: the repaint causes a
   // repaint of EVERYTHING which is to slow
   if (m_nCursorPosition != (int64_t)CursorSeries->XValues->Value[0])
      {
      CursorSeries->XValues->Value[0] = m_nCursorPosition;
      chrtTrack->Canvas->Pen->Mode = pmNot;
      int nPos = chrtTrack->BottomAxis->CalcXPosValue(CursorSeries->XValues->Value[1]);
      chrtTrack->Canvas->MoveTo(nPos, chrtTrack->ChartRect.Top);
      chrtTrack->Canvas->LineTo(nPos, chrtTrack->ChartRect.Bottom);
      nPos = chrtTrack->BottomAxis->CalcXPosValue(CursorSeries->XValues->Value[0]);
      if (nPos > chrtTrack->ChartRect.Right || nPos < chrtTrack->ChartRect.Left)
         return;
      chrtTrack->Canvas->MoveTo(nPos, chrtTrack->ChartRect.Top);
      chrtTrack->Canvas->LineTo(nPos, chrtTrack->ChartRect.Bottom);
      CursorSeries->XValues->Value[1] = CursorSeries->XValues->Value[0];
      }
   #else
   // set position of a paintbox.
   // calculate cursor position in window
   if (m_nCursorPosition != (int64_t)CursorSeries->XValues->Value[0])
      {
      CursorSeries->XValues->Value[0] = m_nCursorPosition;
      int nPos = chrtTrack->BottomAxis->CalcXPosValue(CursorSeries->XValues->Value[0]);
      if (nPos > chrtTrack->ChartRect.Right || nPos < chrtTrack->ChartRect.Left)
         return;
      CursorSeries->XValues->Value[1] = CursorSeries->XValues->Value[0];
      if (nPos + chrtTrack->Left != pbCursor->Left)
         pbCursor->Left = nPos + chrtTrack->Left;
      }
   #endif
}
//------------------------------------------------------------------------------

//------------------------------------------------------------------------------
/// converts current cursor position to a human readable string
//------------------------------------------------------------------------------
AnsiString TTracksForm::CursorPosition()
{
   if (miSamples->Checked)
      return m_nCursorPosition;
   AnsiString str;
   int64_t nMilliseconds = (int64_t)(m_nCursorPosition*1000.0/m_dSampleRate);
   WORD wH, wM, wS, wMS;
   wMS = (WORD)(nMilliseconds % 1000);
   nMilliseconds /= 1000;
   wS = (WORD)(nMilliseconds % 60);
   nMilliseconds /= 60;
   wM = (WORD)(nMilliseconds % 60);
   nMilliseconds /= 60;
   wH = (WORD)(nMilliseconds % 24);
   str.printf("%02d:%02d:%02d:%03d", wH, wM, wS, wMS);
   return str;
}
//------------------------------------------------------------------------------

//------------------------------------------------------------------------------
///  internal function to update track levels, called by SoundClass()->m_pfrmMixer
//------------------------------------------------------------------------------
void TTracksForm::SetTrackLevelsInternal(const std::valarray<float>& vaf)
{
   for (unsigned int n = 0; n < vaf.size(); n++)
      {
      if (n >= m_vTrackInfos.size())
         break;
      m_vTrackInfos[n]->UpdateLevel(vaf[n]);
      }
}
//------------------------------------------------------------------------------

//------------------------------------------------------------------------------
///  internal function to update track levels, called by SoundClass()->m_pfrmMixer
//------------------------------------------------------------------------------
void TTracksForm::ResetTrackLevelsInternal()
{
   for (unsigned int n = 0; n < m_vTrackInfos.size(); n++)
      {
      if (n >= m_vTrackInfos.size())
         break;
      m_vTrackInfos[n]->ResetLevel();
      }
}
//------------------------------------------------------------------------------

//------------------------------------------------------------------------------
/// Sizing...
//------------------------------------------------------------------------------
#pragma argsused
void __fastcall TTracksForm::pnlTracksResize(TObject *Sender)
{
   AdjustVerticalView();
   AdjustHorizontalView();
}
//------------------------------------------------------------------------------

//------------------------------------------------------------------------------
/// Vertical zoom buttons
//------------------------------------------------------------------------------
#pragma argsused
void __fastcall TTracksForm::btnVZoomInClick(TObject *Sender)
{
   if (m_nNumVisibleTracks > 1)
      m_nNumVisibleTracks--;
   AdjustVerticalView();
}
//------------------------------------------------------------------------------
#pragma argsused
void __fastcall TTracksForm::btnVZoomOutClick(TObject *Sender)
{
   if (m_nNumVisibleTracks < m_nNumTracks)
      m_nNumVisibleTracks++;
   AdjustVerticalView();
}
//------------------------------------------------------------------------------
#pragma argsused
void __fastcall TTracksForm::btnVZoomAllClick(TObject *Sender)
{
   m_nNumVisibleTracks = m_nNumTracks;
   AdjustVerticalView();
}
//------------------------------------------------------------------------------

//------------------------------------------------------------------------------
/// Adjust vertical sizing of tracks
//------------------------------------------------------------------------------
void  TTracksForm::AdjustVerticalView()
{
   if (!TryEnterCriticalSection(&m_csTrackView))
      return;
   try
      {
      // adjust scrollbars max value
      int n = (int)(m_nNumTracks - m_nNumVisibleTracks);
      if (n == 0)
         {
         // if now scrollig at all, we have to set Max to 1, otherwise
         // scbVert->PageSize = 1 will yield an error!
         scbVert->Max      = 1;
         scbVert->Position = 0;
         scbVert->Enabled  = false;
         }
      else
         {
         scbVert->Max      = n;
         scbVert->Enabled  = true;
         }
      // check tracks izes (to be sure)
      unsigned int nTracks = (unsigned int)m_vTrackInfos.size();
      // calculate current height ...
      int nHeight = (pnlTracks->Height - pnlTopLeft->Height) / (int)m_nNumVisibleTracks;
      // ... and set it
      for (unsigned i = 0; i < nTracks; i++)
         m_vTrackInfos[i]->Height = nHeight-1;
      // call scrollbar callback as well: if zoom was changed, then visibility of tracks
      // may have changed as well
      scbVertChange(NULL);
      }
   __finally
      {
      LeaveCriticalSection(&m_csTrackView);
      }
}
//------------------------------------------------------------------------------

//------------------------------------------------------------------------------
/// Vertical scrollbar callback: sets visibility of tracks depending on scrollbar
/// position
//------------------------------------------------------------------------------
#pragma argsused
void __fastcall TTracksForm::scbVertChange(TObject *Sender)
{
   if (!TryEnterCriticalSection(&m_csTrackView))
      return;
   try
      {
      int nTracks = (int)m_vTrackInfos.size();
      for (int i = nTracks-1; i >= 0; i--)
         m_vTrackInfos[(unsigned int)i]->Visible = i >= scbVert->Position;
      int nMax = 2* ((int)m_nNumTracks-scbVert->Position);
      int nMin = nMax - 2*(int)m_nNumVisibleTracks;
      chrtTrack->LeftAxis->SetMinMax(nMin, nMax);
      chrtTrack->RightAxis->SetMinMax(nMin, nMax);
      CursorSeries->YValues->Value[0] = chrtTrack->RightAxis->Minimum;
      CursorSeries->YValues->Value[1] = chrtTrack->RightAxis->Maximum;
      }
   __finally
      {
      LeaveCriticalSection(&m_csTrackView);
      }
}
//------------------------------------------------------------------------------

//------------------------------------------------------------------------------
/// Horizontal zoom buttons
//------------------------------------------------------------------------------
#pragma argsused
void __fastcall TTracksForm::btnHZoomOutClick(TObject *Sender)
{
   // calculate new view
   int64_t nXViewLen    = m_nXViewLen * 2;
   int64_t nXTotalLen   = m_nXViewStart + nXViewLen;
   // then check constraints
   // a. does zoom out exceed current total length?
   if (nXTotalLen > m_nXTotalLen)
      {
      // does new requested length exceed total maximum length?
      if (nXTotalLen > m_nXMaxTotalLen)
         {
         // try to adjust the rest with shifting view start
         int64_t nXViewStart = m_nXViewStart - (nXTotalLen - m_nXMaxTotalLen);
         // is new start < 0?
         if (nXViewStart < 0)
            {
            m_nXViewStart  = 0;
            m_nXTotalLen   = m_nXMaxTotalLen;
            m_nXViewLen    = m_nXMaxTotalLen;
            }
         // otherwise we can adjust everything
         else
            {
            m_nXViewStart  = nXViewStart;
            m_nXViewLen    = nXViewLen;
            m_nXTotalLen   = m_nXMaxTotalLen;
            }
         }
      // otherwise we can resize total length
      else
         {
         m_nXTotalLen   = nXTotalLen;
         m_nXViewLen    = nXViewLen;
         }
      }
   // no constraints violated, no size to be adjusted
   else
      {
      m_nXViewLen    = nXViewLen;
      }
   AdjustHorizontalView();
   SetCursorPosInternal();
}
//------------------------------------------------------------------------------
#pragma argsused
void __fastcall TTracksForm::btnHZoomInClick(TObject *Sender)
{
   if (m_nXViewLen < 1000)
      return;
   m_nXViewLen /= 2;
   AdjustHorizontalView();
   SetCursorPosInternal();
}
//------------------------------------------------------------------------------
#pragma argsused
void __fastcall TTracksForm::btnHZoomAllClick(TObject *Sender)
{
   m_nXTotalLen = m_nXMaxDataLen;
   m_nXViewStart  = 0;
   m_nXViewLen    = m_nXTotalLen + m_nXTotalLen/20;
   scbHorz->Position = 0;
   AdjustHorizontalView();
   SetCursorPosInternal();
}
//------------------------------------------------------------------------------

//------------------------------------------------------------------------------
/// Recalculates horizontal view
//------------------------------------------------------------------------------
void  TTracksForm::AdjustHorizontalView()
{
   if (!TryEnterCriticalSection(&m_csTrackView))
      return;
   try
      {
      // set axis of tracks always to samples (all connected to botttom axis)!
      chrtTrack->BottomAxis->SetMinMax(m_nXViewStart, m_nXViewStart + m_nXViewLen);
      if (miSamples->Checked)
         chrtTrack->TopAxis->SetMinMax(chrtTrack->BottomAxis->Minimum, chrtTrack->BottomAxis->Maximum);
      else
         chrtTrack->TopAxis->SetMinMax(chrtTrack->BottomAxis->Minimum*1000.0/m_dSampleRate,
                                       chrtTrack->BottomAxis->Maximum*1000.0/m_dSampleRate);

      // adjust scrollbar properties.
      int64_t nMax = (m_nXTotalLen) / 200;
      if (nMax > INT_MAX)
         nMax = INT_MAX;
      if (scbHorz->Position > nMax)
         scbHorz->Position = (int)nMax-1;
      scbHorz->Max         = (int)nMax;
      scbHorz->SmallChange = (Forms::TScrollBarInc)(m_nXViewLen/2000);
      if (scbHorz->SmallChange == 0)
         scbHorz->SmallChange = 1;
      scbHorz->LargeChange = (Forms::TScrollBarInc)(m_nXViewLen/400);
      if (scbHorz->LargeChange == 0)
         scbHorz->LargeChange = 1;
      scbHorz->Enabled     = scbHorz->Max > 0;
      // scbHorz->PageSize    = scbHorz->SmallChange;
      SetCursorPosInternal();
      }
   __finally
      {
      LeaveCriticalSection(&m_csTrackView);
      }
}
//------------------------------------------------------------------------------

//------------------------------------------------------------------------------
/// callback for horizontal scrollbar
//------------------------------------------------------------------------------
#pragma argsused
void __fastcall TTracksForm::scbHorzChange(TObject *Sender)
{
   m_nXViewStart = scbHorz->Position*200;
   AdjustHorizontalView();
}
//------------------------------------------------------------------------------

//------------------------------------------------------------------------------
/// menuitem for setting time scale
//------------------------------------------------------------------------------
#pragma argsused
void __fastcall TTracksForm::miTimeClick(TObject *Sender)
{
   if (miTime->Checked)
      return;
   chrtTrack->TopAxis->SetMinMax(chrtTrack->BottomAxis->Minimum*1000.0/m_dSampleRate,
                                 chrtTrack->BottomAxis->Maximum*1000.0/m_dSampleRate);
   miTime->Checked = true;
}
//------------------------------------------------------------------------------

//------------------------------------------------------------------------------
/// menuitem for setting sample scale
//------------------------------------------------------------------------------
#pragma argsused
void __fastcall TTracksForm::miSamplesClick(TObject *Sender)
{
   if (miSamples->Checked)
      return;
   chrtTrack->TopAxis->SetMinMax(chrtTrack->BottomAxis->Minimum, chrtTrack->BottomAxis->Maximum);
   miSamples->Checked = true;
}
//------------------------------------------------------------------------------

//------------------------------------------------------------------------------
/// menuitem for showing about box
//------------------------------------------------------------------------------
#pragma argsused
void __fastcall TTracksForm::miAboutClick(TObject *Sender)
{
   if (!SoundClass())
      return;
   SoundClass()->About();
}
//------------------------------------------------------------------------------

//------------------------------------------------------------------------------
/// sets a status bar string
//------------------------------------------------------------------------------
void TTracksForm::SetStatusString(AnsiString str, int nIndex)
{
   if (!sbStatus || nIndex >= sbStatus->Panels->Count)
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
/// menu call to refresh data
//------------------------------------------------------------------------------
#pragma argsused
void __fastcall TTracksForm::miRefreshClick(TObject *Sender)
{
   if (!miRefresh->Enabled)
      return;
   if (!SoundClass())
      throw Exception("Global SoundClass invalid");
   if (SoundClass()->DeviceIsRunning() && !SoundClass()->Paused())
      throw Exception("command not allowed if device is running");
   UpdateTracks(false, m_bShowWaveData);
}
//------------------------------------------------------------------------------

//------------------------------------------------------------------------------
/// Keypreview function for cancelling (wave) data update and calling Refresh
//------------------------------------------------------------------------------
#pragma argsused
void __fastcall TTracksForm::FormKeyDown(TObject *Sender, WORD &Key,
	  TShiftState Shift)
{
   if (Key == VK_ESCAPE)
      m_bBreak = true;
   else if (Key == VK_F5)
      miRefreshClick(NULL);
}
//------------------------------------------------------------------------------

//------------------------------------------------------------------------------
/// timer callback: updates cursor position
//------------------------------------------------------------------------------
#pragma argsused
void __fastcall TTracksForm::CursorTimerTimer(TObject *Sender)
{
   CursorTimer->Enabled = false;
   // reset internal cursr position if device is stopped (avoid 'hangig' cursor 'somwhere')
   if (!SoundClass()->DeviceIsRunning() && m_nCursorPosition != 0)
      m_nCursorPosition = 0;
   SetCursorPosInternal();
   CursorTimer->Enabled = true;
}
//------------------------------------------------------------------------------

//------------------------------------------------------------------------------
/// converts labeltext to time, if necessary
//------------------------------------------------------------------------------
#pragma argsused
void __fastcall TTracksForm::chrtTrackGetAxisLabel(TChartAxis *Sender, TChartSeries *Series,
          int ValueIndex, UnicodeString &LabelText)
{
   if (miSamples->Checked)
      return;
   if (Sender != chrtTrack->TopAxis)
      return;
   int64_t n;
   if (!TryStrToInt64(LabelText, n))
      {
      LabelText = "";
      return;
      }
   TDateTime dt;
   dt =  n / (1000.0 * 86400); // convert days
   LabelText = FormatDateTime ("hh:nn:ss:zzz", dt);
}
//------------------------------------------------------------------------------

//------------------------------------------------------------------------------
/// sets play position if left mpouse click with pressed Crtl on time scale
//------------------------------------------------------------------------------
#pragma argsused
void __fastcall TTracksForm::chrtTrackMouseDown(TObject *Sender, TMouseButton Button,
         TShiftState Shift, int X, int Y)
{
   if (  Button == mbLeft
      && Shift.Contains(ssCtrl)
      && chrtTrack->RightAxis->CalcPosPoint(Y) > chrtTrack->RightAxis->Maximum
      )
      {
      // set it!
      CursorTimer->Enabled = false;
      SoundClass()->SetPosition((uint64_t)chrtTrack->BottomAxis->CalcPosPoint(X));
      CursorTimer->Enabled = true;
      }
}
//------------------------------------------------------------------------------

//------------------------------------------------------------------------------
/// Updates all data blocks of the track
//------------------------------------------------------------------------------
uint64_t    TTracksForm::UpdateData(bool bRemoveOnly)
{
   uint64_t nNumSamples   = 0;
   uint64_t nNumTrackSamples;

   if (SoundClass()->m_bNoGUI)
      return nNumSamples;


   if (!SoundClass())
      throw Exception("Global SoundClass invalid");
   // check which data to be reloaded and which are still/already there
   // (all where stored with unique GUID)
   chrtTrack->OnAfterDraw = NULL;
   TStringList *psl = new TStringList();
   int nNumTracks = (int)SoundClass()->m_vTracks.size();
   try
      {
      int nCount = (int)chrtTrack->SeriesList->Count;
      // move CursorSeries back (position 1)
      chrtTrack->ExchangeSeries(1, CursorSeries->SeriesIndex);
      for (int nTag = 0; nTag < nNumTracks; nTag ++)
         {
         if (SoundClass()->m_vTracks.size() <= (unsigned int)nTag)
            throw Exception("Global track count inconsistent");
         SDPOutputData* psdpodTmp = SoundClass()->m_vTracks[(unsigned int)nTag]->m_psdpod[0];
         // build list with data segments that are 'still there' in track
         while (psdpodTmp)
            {
            psl->Add(psdpodTmp->Id());
            psdpodTmp = psdpodTmp->m_psdpodNext;
            }
         }
      int nIndex;
      // then remove chart series, that are _not_ in this list and
      for (int i = nCount - 1; i >= 0 ; i--)
         {
         // only handle the shape series!
         TChartShape* pcs = dynamic_cast<TChartShape*>(chrtTrack->Series[i]);
         if (!pcs || !pcs->Tag)
            continue;
         try
            {
            nIndex = psl->IndexOf( ( (TSampleData*) (pcs->Tag) )->m_strID);
            // series present, where track data not exist: remove series
            if (nIndex  < 0)
               {
               RemoveChartSeries(pcs);
               }
            // series present, where still track data exist: remove from psl (then
            // no new series will be created)
            else
               {
               psl->Delete(nIndex);
               }
            }
         catch (...)
            {
            RemoveChartSeries(pcs);
            }
         }
      if (bRemoveOnly)
         return 0;

      for (int nTag = 0; nTag < nNumTracks; nTag ++)
         {
         // access first data block
         SDPOutputData* psdpodTmp = SoundClass()->m_vTracks[(unsigned int)nTag]->m_psdpod[0];
         TChartShape* pcsShape;
         AnsiString strName;
         TSampleData* ptsdLast = NULL;
         //TColor acl[4] = {clRed, clLime, clAqua, clYellow};
         //int x = 0;
         while (psdpodTmp)
            {
            // create series only, if Id() in list
            if (psl->IndexOf(psdpodTmp->Id()) > -1)
               {               
               strName = GUIDToName(psdpodTmp->Id());
               pcsShape                   = new TChartShape(this);
               pcsShape->Name             = "Series" + strName;
               pcsShape->Selected->Hover->Visible = False;
               pcsShape->ParentChart      = chrtTrack;
               pcsShape->HorizAxis        = aBottomAxis;
               pcsShape->Style            = chasRectangle;
               //if (x++ < 4)
               //pcsShape->Brush->Color     = acl[x];
               //else

               if (m_bSeriesDarkTheme)
                  {
                  if (SoundClass()->m_vTracks[(unsigned int)nTag]->Multiply())
                     pcsShape->Brush->Color     = SMP_TRCK_ORANGE_DARK;
                  else if (psdpodTmp->IsFile())
                     pcsShape->Brush->Color     = SMP_TRCK_GREEN_DARK;
                  else
                     pcsShape->Brush->Color     = SMP_TRCK_BLUE_DARK;
                  }
               else
                  {
                  if (SoundClass()->m_vTracks[(unsigned int)nTag]->Multiply())
                     pcsShape->Brush->Color     = SMP_TRCK_ORANGE_LIGHT;
                  else if (psdpodTmp->IsFile())
                     pcsShape->Brush->Color     = SMP_TRCK_GREEN_LIGHT;
                  else
                     pcsShape->Brush->Color     = SMP_TRCK_BLUE_LIGHT;
                  }
                  
               pcsShape->SeriesColor      = pcsShape->Brush->Color;
               pcsShape->Title            = psdpodTmp->Name();
               pcsShape->Alignment        = taLeftJustify;
               pcsShape->Y0               = 2*(nNumTracks-nTag-1)+0.01;
               pcsShape->Y1               = 2*(nNumTracks-nTag) ;
               pcsShape->X0               = psdpodTmp->GetGlobalPosition();
               pcsShape->VertAxis         = aRightAxis;
               if (psdpodTmp->LoopCount() == 0)
                  pcsShape->X1 = 1;
               else
                  pcsShape->X1 = pcsShape->X0 + (int)psdpodTmp->TotalLength();

               // create/store data needed for painting of this series. NOTE: here
               // we copy all data from m_psod->m_psod needed in painting,
               // because pointer may be invalid later! Pointer is only used
               // in wave-data reading function UpdateWaveData
               TSampleData* ptsd    = new TSampleData();
               ptsd->m_nTotalLen    = psdpodTmp->TotalLength();
               ptsd->m_nLoopCount   = psdpodTmp->LoopCount();
               ptsd->m_nStartOffset = psdpodTmp->GetStartOffset();
               ptsd->m_nLoopLen     = psdpodTmp->LengthSingleLoop();
               ptsd->m_nLoopRampLen = psdpodTmp->m_sdopAudio.nLoopRampLenght;
               ptsd->m_bCrossfade   = psdpodTmp->m_sdopAudio.bLoopCrossfade;
               ptsd->m_nRampLen     = psdpodTmp->m_sdopAudio.nRampLenght;
               ptsd->m_nCrossLenL   = psdpodTmp->m_sdopAudio.nCrossfadeLengthLeft;
               ptsd->m_nCrossLenR   = psdpodTmp->m_sdopAudio.nCrossfadeLengthRight;
               ptsd->m_strID        = psdpodTmp->Id();
               ptsd->m_bMultiply    = SoundClass()->m_vTracks[(unsigned int)nTag]->Multiply();
               ptsd->m_psod         = psdpodTmp;
               ptsd->m_ptsdLast     = ptsdLast;
               // resize vector: we create three wav-data buffers: one for a loop one for
               // 'first' samples, one for 'last' samples in loop and/or crossfade mode!
               ptsd->m_vvaf.resize(3);
               // set status to 1 to tell TTrackFrame::UpdateWaveData that data have to be loaded
               ptsd->m_nStatus      = 1;
               pcsShape->Tag        = (NativeInt)ptsd;
               ptsdLast             = ptsd;
               }
            psdpodTmp = psdpodTmp->m_psdpodNext;
            }

         // set internal data width
         nNumTrackSamples   = SoundClass()->m_vTracks[(unsigned int)Tag]->NumTrackSamples();
         if (nNumTrackSamples > nNumSamples)
            nNumSamples = nNumTrackSamples;
         }
      }
   __finally
      {
      TRYDELETENULL(psl);
      // put CursorSeries to top again
//      chrtTrack->ExchangeSeries(CursorSeries->SeriesIndex, (int)chrtTrack->SeriesList->Count-1);
      chrtTrack->OnAfterDraw = chrtTrackAfterDraw;
      }
   return nNumSamples;
}
//------------------------------------------------------------------------------

//------------------------------------------------------------------------------
/// clears audio data in passed TSampleData
//------------------------------------------------------------------------------
void TTracksForm::ClearWaveData(TSampleData *psd)
{
   if (psd)
      {
      for (unsigned int i = 0; i < psd->m_vvaf.size(); i++)
         psd->m_vvaf[i].resize(0);
      psd->m_vvaf.resize(3);
      }
}
//------------------------------------------------------------------------------

//------------------------------------------------------------------------------
/// removes a chart shape and the connected line series and TSampleData
//------------------------------------------------------------------------------
void TTracksForm::RemoveChartSeries(TChartSeries* pcs)
{
   TChartShape* pcsTmp = dynamic_cast<TChartShape*>(pcs);
   if (!!pcsTmp)
      {
      TSampleData *psd = (TSampleData*)(pcsTmp->Tag);
      TRYDELETENULL(psd);
      }
   chrtTrack->RemoveSeries(pcs);
   TRYDELETENULL(pcs);
}
//------------------------------------------------------------------------------

//------------------------------------------------------------------------------
/// Updates all data samples of the track. IMPORTANT NOTE: must be called
/// directly after UpdateData() without any changes to global ASIO track buffers
/// Is split to give caller tha chance to first 'draw' empty blocks for all tracks
/// in a first loop and lenghty sample drawing afterwards (!) in a second loop
//------------------------------------------------------------------------------
void TTracksForm::UpdateWaveData()
{
   TChartShape* pcs;
   chrtTrack->OnAfterDraw = NULL;
   float* lpf;
   int nCount = (int)chrtTrack->SeriesList->Count;
   for (int i = 0; i < nCount; i++)
      {
      // apply only to shape series!
      pcs = dynamic_cast<TChartShape*>(chrtTrack->Series[i]);
      if (!pcs || !pcs->Tag)
         continue;
      TSampleData *psd = (TSampleData*)(pcs->Tag);
      try
         {
         // if it's tag is (already) != 1, then don't load wave data: already done
         // in earlier call ()
         if (psd->m_nStatus != 1)
            {
            continue;
            }
         ClearWaveData(psd);
         // #define NO_PSOD_COPY2
         #ifdef NO_PSOD_COPY2
         uint64_t nOldPos = psd->m_psod->GetPosition();
         // lead to XRuns.... But we do reommend NOT to draw data while
         // output is running
         EnterCriticalSection(&SoundClass()->m_csProcess);
         try
            {
            psd->m_psod->SetPosition(0);
            int64_t  nSamples;
            // if only one loop within we read the complete data as they are
            if ( psd->m_nLoopCount == 1)
               {
               nSamples = psd->m_nTotalLen;
               psd->m_vvaf[0].resize((int)nSamples);
               lpf = &psd->m_vvaf[0][0];
               // NOTE: GetSample already takes care of the start offset, therefore we can
               // start with painting in chrtTrackAfterDraw directly with first sample!
               while (nSamples--)
                  {
                  if (m_bBreak)
                     break;
                  *lpf++ = psd->m_psod->GetSample(true);
                  }
               }
            else
               {
               // in loop mode we will read:
               // - psd->m_nLoopRampLen very first samples
               // - one complete loop including loopramp and/or crossfade: will be template for all other loops
               //   NOTE: we can start reading at LoopRampLen samples BEFORE end of first loop, because
               //   startoffset is not allowed within that range! We NEED to do this because if loopcount is 2
               //   then the second loop will NOT contain the loopramp doen and/or corresponding crossfade
               //   NOTE: we will re-sort this in buffer to have the loop 'straight'
               // - psd->m_nLoopRampLen very last samples (if not empty loop)
               // read starting samples, if m_nStartOffset is <  m_nLoopRampLen
               if (psd->m_nStartOffset < psd->m_nLoopRampLen)
                  {
                  nSamples = psd->m_nLoopRampLen - psd->m_nStartOffset;
                  psd->m_vvaf[1].resize((int)nSamples);
                  lpf = &psd->m_vvaf[1][0];
                  while (nSamples--)
                     {
                     if (m_bBreak)
                        break;
                     *lpf++ = psd->m_psod->GetSample(true);
                     }
                  }
               if (!!psd->m_nLoopRampLen)
                  {
                  psd->m_psod->SetPosition(psd->m_nTotalLen - psd->m_nLoopRampLen);
                  nSamples = psd->m_nLoopRampLen;
                  psd->m_vvaf[2].resize((int)nSamples);
                  lpf = &psd->m_vvaf[2][0];
                  while (nSamples--)
                     {
                     if (m_bBreak)
                        break;
                     *lpf++ = psd->m_psod->GetSample(true);
                     }
                  }
               // read complete loop AND sort it
               psd->m_psod->SetPosition(psd->m_nLoopLen - psd->m_nStartOffset - psd->m_nLoopRampLen);
               psd->m_vvaf[0].resize((int)psd->m_nLoopLen);
               // read first samples (loop ramp down!) to end (!) of buffer
               nSamples = psd->m_nLoopRampLen;
               lpf = &psd->m_vvaf[0][(int)(psd->m_nLoopLen - psd->m_nLoopRampLen)];
               while (nSamples--)
                  {
                  if (m_bBreak)
                     break;
                  *lpf++ = psd->m_psod->GetSample(true);
                  }
               // read other samples to beginning of buffer
               nSamples = psd->m_nLoopLen - psd->m_nLoopRampLen;
               lpf = &psd->m_vvaf[0][0];
               while (nSamples--)
                  {
                  if (m_bBreak)
                     break;
                  *lpf++ = psd->m_psod->GetSample(true);
                  }
               }
            psd->m_nStatus = 0;
            psd->m_nStartLastSamples = psd->m_nTotalLen - psd->m_vvaf[2].size();
            }
         __finally
            {
            // reset position!
            psd->m_psod->SetPosition(nOldPos);
            LeaveCriticalSection(&SoundClass()->m_csProcess);
            }
         #else  // WITH PSOD COPY!!
         // create a copy of psd->m_psod object
         SDPOutputData* psod = NULL;
         EnterCriticalSection(&SoundClass()->m_csProcess);
         try
            {
            psod = new SDPOutputData(psd->m_psod->m_sdopAudio, psd->m_psod);
            psod->m_psdpodNext = psd->m_psod->m_psdpodNext;
            }
         __finally
            {
            LeaveCriticalSection(&SoundClass()->m_csProcess);
            }
         try
            {
            int64_t  nSamples;
            // if only one loop within we read the complete data as they are
            if ( psd->m_nLoopCount == 1)
               {
               nSamples = (int64_t)psd->m_nTotalLen;
               psd->m_vvaf[0].resize((unsigned int)nSamples);
               lpf = &psd->m_vvaf[0][0];
               // NOTE: GetSample already takes care of the start offset, therefore we can
               // start with painting in chrtTrackAfterDraw directly with first sample!
               while (nSamples--)
                  {
                  if (m_bBreak)
                     break;
                  *lpf++ = psod->GetSample(true);
                  }
               }
            else
               {
               // in loop mode we will read:
               // - psd->m_nLoopRampLen very first samples
               // - one complete loop including loopramp and/or crossfade: will be template for all other loops
               //   NOTE: we can start reading at LoopRampLen samples BEFORE end of first loop, because
               //   startoffset is not allowed within that range! We NEED to do this because if loopcount is 2
               //   then the second loop will NOT contain the loopramp doen and/or corresponding crossfade
               //   NOTE: we will re-sort this in buffer to have the loop 'straight'
               // - psd->m_nLoopRampLen very last samples (if not empty loop)
               // read starting samples, if m_nStartOffset is <  m_nLoopRampLen
               if (psd->m_nStartOffset < psd->m_nLoopRampLen)
                  {
                  nSamples = (int64_t)psd->m_nLoopRampLen - (int64_t)psd->m_nStartOffset;
                  psd->m_vvaf[1].resize((unsigned int)nSamples);
                  lpf = &psd->m_vvaf[1][0];
                  while (nSamples--)
                     {
                     if (m_bBreak)
                        break;
                     *lpf++ = psod->GetSample(true);
                     }
                  }
               if (!!psd->m_nLoopRampLen)
                  {
                  psod->SetPosition(psd->m_nTotalLen - psd->m_nLoopRampLen);
                  nSamples = (int64_t)psd->m_nLoopRampLen;
                  psd->m_vvaf[2].resize((unsigned int)nSamples);
                  lpf = &psd->m_vvaf[2][0];
                  while (nSamples--)
                     {
                     if (m_bBreak)
                        break;
                     *lpf++ = psod->GetSample(true);
                     }
                  }
               // read complete loop AND sort it
               psod->SetPosition(psd->m_nLoopLen - psd->m_nStartOffset - psd->m_nLoopRampLen);
               psd->m_vvaf[0].resize((unsigned int)psd->m_nLoopLen);
               // read first samples (loop ramp down!) to end (!) of buffer
               nSamples = (int64_t)psd->m_nLoopRampLen;
               lpf = &psd->m_vvaf[0][(unsigned int)(psd->m_nLoopLen - psd->m_nLoopRampLen)];
               while (nSamples--)
                  {
                  if (m_bBreak)
                     break;
                  *lpf++ = psod->GetSample(true);
                  }
               // read other samples to beginning of buffer
               nSamples = (int64_t)psd->m_nLoopLen - (int64_t)psd->m_nLoopRampLen;
               lpf = &psd->m_vvaf[0][0];
               while (nSamples--)
                  {
                  if (m_bBreak)
                     break;
                  *lpf++ = psod->GetSample(true);
                  }
               }
            // adjust psd
            psd->m_nStatus = 0;
            psd->m_nStartLastSamples = (int64_t)psd->m_nTotalLen - (int64_t)psd->m_vvaf[2].size();
            }
         __finally
            {
            TRYDELETENULL(psod);
            }
         #endif
         }
      catch (Exception &e)
         {
         ClearWaveData(psd);
         OutputDebugStringW((L"error reading wave data for visualization: " + e.Message).w_str());
         }
      }
   //OutputDebugStringW((UnicodeString(__FUNC__) + ": " + IntToStr((int)(GetTick
   chrtTrack->OnAfterDraw = chrtTrackAfterDraw;
   chrtTrack->Repaint();
   #ifdef DEBUG_UPDATEDATA
   OutputDebugString(__FUNC__ " done");
   #endif
}
//------------------------------------------------------------------------------

//------------------------------------------------------------------------------
/// retrieve a sample for painting
//------------------------------------------------------------------------------
float TTracksForm::GetSampleVis(TSampleData *psd, int64_t nDataPos)
{
   float dValue = 0.0f;
   int64_t nPos;

   // only one loop? All data are in buffer 0
   if (psd->m_nLoopCount == 1)
      {
      if (nDataPos < (int64_t)psd->m_vvaf[0].size())
         dValue = psd->m_vvaf[0][(unsigned int)nDataPos];
      }
   else
      {
      // first values may be in buffer 1
      if (nDataPos < (int64_t)psd->m_vvaf[1].size())
         dValue = psd->m_vvaf[1][(unsigned int)nDataPos];
      // last values may be in buffer 2
      else if (!!psd->m_nLoopCount && nDataPos >= psd->m_nStartLastSamples)
         {
         nPos = nDataPos-psd->m_nStartLastSamples;
         if (nPos < (int64_t)psd->m_vvaf[2].size())
            dValue = psd->m_vvaf[2][(unsigned int)nPos];
         }
      // other values are in buffer 0, use 'modulo' for repeating loop
      else
         {
         nPos = ((nDataPos+(int64_t)psd->m_nStartOffset) % (int64_t)psd->m_nLoopLen);
         if (nPos < (int64_t)psd->m_vvaf[0].size())
            dValue = psd->m_vvaf[0][(unsigned int)nPos];
         }
      }
   // paint crossfade region by appliying ramp and retrieve sample from last snippet
   // and ramp it as well
   if (psd->m_nCrossLenL && psd->m_ptsdLast && nDataPos < (int64_t)psd->m_nCrossLenL)
      {
      float f2 = GetSampleVis(psd->m_ptsdLast, (int64_t)psd->m_nTotalLen - (int64_t)psd->m_ptsdLast->m_nCrossLenR + nDataPos);
      if (g_bUseRamps)
         {
         dValue *= GetHanningRamp((unsigned int)nDataPos, (unsigned int)psd->m_nCrossLenL, true);
         f2     *= GetHanningRamp((unsigned int)nDataPos, (unsigned int)psd->m_nCrossLenL, false);
         }
      dValue += f2;
      }

   // apply overall ramp (if any)
   if (g_bUseRamps && psd->m_nRampLen)
      {
      if (nDataPos < (int64_t)psd->m_nRampLen)
         dValue *= GetHanningRamp((unsigned int)nDataPos, (unsigned int)psd->m_nRampLen, true);
      else if (nDataPos > (int64_t)(psd->m_nTotalLen - psd->m_nRampLen))
         dValue *= GetHanningRamp((unsigned int)((int64_t)psd->m_nTotalLen - nDataPos), (unsigned int)psd->m_nRampLen, true);
      }

   // clip value for display
   if (dValue > 1.0f)
      dValue = 1.0f;
   else if (dValue < -0.99f)
      dValue = -0.99f;

   return dValue;
}
//------------------------------------------------------------------------------

//------------------------------------------------------------------------------
/// paint loops
//------------------------------------------------------------------------------
#pragma argsused
void __fastcall TTracksForm::chrtTrackAfterDraw(TObject *Sender)
{
   chrtTrack->Canvas->Pen->Color = clBlack;         
//   DWORD dw = GetTickCount();
   m_bBreak = false;
   TChartShape *pcs;
   TRect rc;
   int64_t nDataPos, nCrossfadePos, nLastPos, nPos, nX1Pos, nStartPos, nStartSample, nX0, nX1, nY0, nY1, nY;
   int nCount = (int)chrtTrack->SeriesList->Count;
   double dValue;

   for (int i = 0; i < nCount; i++)
      {
      // apply only to the shape series!
      pcs = dynamic_cast<TChartShape*>(chrtTrack->Series[i]);
      if (!pcs || !pcs->Tag)
         continue;
      chrtTrack->Canvas->Brush->Color = pcs->Brush->Color;
      // NOTE: access to X0 and  X1 seems to be VERY slow! So we do this in outer loop only once
      nX0 = (int64_t)pcs->X0;
      nX1 = (int64_t)pcs->X1;
      nY0 = (int64_t)pcs->Y0;
      nY1 = (int64_t)pcs->Y1;
      nY = (nY0 + nY1)/2;
      nX1Pos = pcs->CalcXPosValue(nX1);
      // access loopdata pointer
      TSampleData *psd = (TSampleData*)(pcs->Tag);
      if (!psd)
         continue;
      // calculate start iteration value
      // - first visible axis value
      nStartPos = (int64_t)nX0;
      // - first visble sample value (within loop)
      nStartSample = 0;
      if (nStartPos < chrtTrack->BottomAxis->Minimum)
         {
         nStartPos      = (int64_t)chrtTrack->BottomAxis->Minimum;
         nStartSample   = (int64_t)(chrtTrack->BottomAxis->Minimum - nX0) % (int64_t)psd->m_nLoopLen;
         }
      // draw baseline
      chrtTrack->Canvas->Pen->Color = m_bSeriesDarkTheme ? clCream : clBlack;
      chrtTrack->Canvas->Pen->Style = psDot;
      chrtTrack->Canvas->MoveTo(pcs->CalcXPosValue(nX0), pcs->CalcYPosValue(nY));
      chrtTrack->Canvas->LineTo(pcs->CalcXPosValue(nX1), pcs->CalcYPosValue(nY));
      // do we have to paint wave data??
      if (!psd->m_vvaf[0].size())
         continue;
      try
         {
         chrtTrack->Canvas->Pen->Style = psSolid;
         // paint every single sample if not so many shown - or if it is a 'multiply' track
         if (psd->m_bMultiply || ((chrtTrack->BottomAxis->Maximum-chrtTrack->BottomAxis->Minimum) / (double)Width) < 40.0)
            {
            bool bFirstDone = false;
            for (int j = 0; j < INT_MAX; j++)
               {
               // calculate position in axis/chart series values
               nDataPos = nStartPos + j;
               nPos = pcs->CalcXPosValue(nDataPos);
               // break if not visible any more
               if (nDataPos > chrtTrack->BottomAxis->Maximum || nPos >= nX1Pos)
                  break;
               // re-calculate data position relativ to first sample
               nDataPos -= nX0;
               // do NOT paint the crossfade region: is done by next snippet
               if (psd->m_nCrossLenR && nDataPos > (int64_t)(psd->m_nTotalLen - psd->m_nCrossLenR))
                  break;
               dValue = (double)GetSampleVis(psd, nDataPos);
               if (!bFirstDone)
                  {
                  bFirstDone = true;
                  chrtTrack->Canvas->MoveTo((int)nPos, (int)pcs->CalcYPosValue(dValue + nY));
                  }
               else
                  chrtTrack->Canvas->LineTo((int)nPos, (int)pcs->CalcYPosValue(dValue + nY));
               if (m_bBreak)
                  break;
               }
            }
         // if we have more than 40000 we only draw vertical lines from Min to Max for
         // each x-pixel for performance reasons
         else
            {
            double dMin = 0.0;
            double dMax = 0.0;
            nLastPos = -2;
            int nMax, nMin, nLastMax, nLastMin, nPosInt;
            nLastMax = nLastMin = 0;
            for (int j = 0; j < INT_MAX; j++)
               {
               chrtTrack->Canvas->Pen->Color = m_bSeriesDarkTheme ? clCream : clBlack;
               // calculate position in axis/chart series values
               nDataPos = nStartPos + j;
               nPos = pcs->CalcXPosValue(nDataPos);
               // break if not visible any more or reached the end
               if (nDataPos > chrtTrack->BottomAxis->Maximum || nPos >= nX1Pos)
                  break;
               // re-calculate data position relativ to first sample
               nDataPos -= nX0;
               // do NOT paint the crossfade region: is done by next snippet
               if (psd->m_nCrossLenR && nDataPos > (int64_t)(psd->m_nTotalLen - psd->m_nCrossLenR))
                  break;
               dValue = (double)GetSampleVis(psd, nDataPos);

               // if no advance in x-position, only store min/max
               if (nPos - nLastPos < 1)
                  {
                  if (dValue > dMax)
                     dMax = dValue;
                  else if (dValue < dMin)
                     dMin = dValue;
                  continue;
                  }
               // paint line from max to min
               nMax = (int)pcs->CalcYPosValue(dMax + nY);
               nMin = (int)pcs->CalcYPosValue(dMin + nY);
               nPosInt = (int)nPos;
               
               chrtTrack->Canvas->MoveTo(nPosInt, nMax);
               chrtTrack->Canvas->LineTo(nPosInt, nMin);

               
               if (m_bSeriesDarkTheme && nLastPos > -1)
                  {
                  chrtTrack->Canvas->Pen->Color = clBlack;
                  chrtTrack->Canvas->MoveTo((int)nLastPos, nLastMin);
                  chrtTrack->Canvas->LineTo(nPosInt, nMin);
                  chrtTrack->Canvas->MoveTo((int)nLastPos, nLastMax);
                  chrtTrack->Canvas->LineTo(nPosInt, nMax);
                  }
               nLastMax = nMax;
               nLastMin = nMin;
               
               if (m_bBreak)
                  break;
               nLastPos = nPos;
               dMin = 0.0;
               dMax = 0.0;
               }
            }
         }
      catch (Exception &e)
         {
         AnsiString str;
         str.sprintf("paint error (%s)", AnsiString(e.Message).c_str());
         OutputDebugString(str.c_str());
         }
      chrtTrack->Canvas->Pen->Color = m_bSeriesDarkTheme ? clCream : clBlack;

      if (psd->m_nCrossLenL)
         {
         // paint crossfade
         chrtTrack->Canvas->Pen->Style = psDot;
         nPos           = pcs->CalcXPosValue(nX0);
         nCrossfadePos  = pcs->CalcXPosValue(nX0 + (int64_t)psd->m_nCrossLenL);
         chrtTrack->Canvas->MoveTo((int)nPos, (int)pcs->CalcYPosValue(nY0));
         chrtTrack->Canvas->LineTo((int)nCrossfadePos, (int)pcs->CalcYPosValue(nY1));
         chrtTrack->Canvas->LineTo((int)nCrossfadePos, (int)pcs->CalcYPosValue(nY0));
         chrtTrack->Canvas->LineTo((int)nPos, (int)pcs->CalcYPosValue(nY1));
         chrtTrack->Canvas->LineTo((int)nPos, (int)pcs->CalcYPosValue(nY0));
         }
      
      // then paint vertical loop lines
      if (psd->m_nLoopCount != 1)
         {
         // set cliprect for not draw crossfades outside of shape
         chrtTrack->Canvas->ClipRectangle(TRect(
            (int)pcs->CalcXPosValue(nX0),
            (int)pcs->CalcYPosValue(nY0),
            (int)pcs->CalcXPosValue(nX1),
            (int)pcs->CalcYPosValue(nY1)
            ));
         nLastPos = -6;
         chrtTrack->Canvas->Pen->Style = psDot;
         for (int64_t j = 1; j < INT_MAX; j++)
            {
            // calculate position in axis/chart series values
            nDataPos = nStartPos - nStartSample + j*(int64_t)psd->m_nLoopLen - (int64_t)psd->m_nStartOffset;
            // do not paint if not visible yet
            if (nDataPos < chrtTrack->BottomAxis->Minimum)
               continue;
            // break if not visible or last loop in non-endless-loop
            if (  nDataPos > chrtTrack->BottomAxis->Maximum
               || (!!psd->m_nLoopCount && nDataPos > (nX1-(int64_t)psd->m_nLoopLen))
               )
               break;
            nPos = pcs->CalcXPosValue(nDataPos);
            // don't paint lines too narrow
            if (nPos - nLastPos < 5)
               continue;
            // paint line
            chrtTrack->Canvas->MoveTo((int)nPos, (int)pcs->CalcYPosValue(nY0));
            chrtTrack->Canvas->LineTo((int)nPos, (int)pcs->CalcYPosValue(nY1));
            if (psd->m_bCrossfade)
               {
               // paint crossfade
               nCrossfadePos = pcs->CalcXPosValue(nDataPos + (int64_t)psd->m_nLoopRampLen);
               chrtTrack->Canvas->LineTo((int)nCrossfadePos, (int)pcs->CalcYPosValue(nY0));
               chrtTrack->Canvas->LineTo((int)nCrossfadePos, (int)pcs->CalcYPosValue(nY1));
               chrtTrack->Canvas->LineTo((int)nPos, (int)pcs->CalcYPosValue(nY0));
               }
            nLastPos = nPos;
            }
         chrtTrack->Canvas->UnClipRectangle();
         }

      // and finally name of object (file or vector)
      if (!pcs->Title.IsEmpty())
         {
         chrtTrack->Canvas->Pen->Style = psSolid;
         rc = pcs->Bounds;
         chrtTrack->Canvas->ClipRectangle(rc);
         chrtTrack->Canvas->Brush->Color = m_bSeriesDarkTheme ? SMP_TRCK_BLUE_LIGHT : SMP_TRCK_BLUE_DARK;
         chrtTrack->Canvas->Brush->Style = bsSolid;
         //chrtTrack->Canvas->Rectangle(Rect(rc.Left+2, rc.Bottom+2, rc.Left+chrtTrack->Canvas->TextWidth(pcs->Title), rc.Bottom+chrtTrack->Canvas->TextHeight(pcs->Title)));
         chrtTrack->Canvas->Font->Size = 7;
         chrtTrack->Canvas->Font->Color = m_bSeriesDarkTheme ? clBlack : clWhite;
         chrtTrack->Canvas->TextOut(rc.Left+3, rc.Bottom+1, pcs->Title);
         chrtTrack->Canvas->UnClipRectangle();
         }
      }
}
//------------------------------------------------------------------------------

//------------------------------------------------------------------------------
/// adjust right value (X1) for endless looped files/vectors
//------------------------------------------------------------------------------
#pragma argsused
void __fastcall TTracksForm::chrtTrackBeforeDrawSeries(TObject *Sender)
{
   // draw track pattern
   int nMin = (int)floor(chrtTrack->LeftAxis->Minimum);
   int nMax = (int)ceil(chrtTrack->LeftAxis->Maximum);
   TColor cl;
   for (int n = nMin; n < nMax; n+=2)
      {
      if (m_bIsDarkTheme)
         cl = (n % 4 == 0) ? TColor(0x00505050) : TColor(0x00A0A0A0) ;
      else
         cl = (n % 4 == 0) ? TColor(0x00C0C0C0) : TColor(0x00DDDDDD);

      chrtTrack->Canvas->Brush->Color = cl;
      chrtTrack->Canvas->FillRect(Rect(0, chrtTrack->LeftAxis->CalcYPosValue(n+2), chrtTrack->Width, chrtTrack->LeftAxis->CalcYPosValue(n)));
      }
   // store eventual endless loop
   TChartShape *pcs;
   for (int i = 0; i < chrtTrack->SeriesList->Count; i++)
      {
      // apply only to the shape series!
      pcs = dynamic_cast<TChartShape*>(chrtTrack->Series[i]);
      if (!pcs || !pcs->Tag)
         continue;
      // access loopdata pointer
      if (((TSampleData*)(pcs->Tag))->m_nLoopCount == 0)
         pcs->X1 = chrtTrack->BottomAxis->Maximum;
      }
}
//------------------------------------------------------------------------------

//------------------------------------------------------------------------------
/// OnPaint callback for cursor TPaintBox. Draws line fullheight (pbCursor pen
/// style isset to pmNot in constructor
//------------------------------------------------------------------------------
#pragma argsused
void __fastcall TTracksForm::pbCursorPaint(TObject *Sender)
{
   pbCursor->Canvas->MoveTo(0,0);
   pbCursor->Canvas->LineTo(0,pbCursor->Height);
}
//------------------------------------------------------------------------------

