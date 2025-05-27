//------------------------------------------------------------------------------
/// \file formTrack.h
/// \author Berg
/// \brief Interface of class TTrackForm for visualization of audio tracks
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
#ifndef formTracksH
#define formTracksH
//------------------------------------------------------------------------------
#include <Classes.hpp>
#include <Controls.hpp>
#include <StdCtrls.hpp>
#include <Forms.hpp>
#include <ExtCtrls.hpp>
#include <Buttons.hpp>
#include <ComCtrls.hpp>
#include <ToolWin.hpp>
#include <ImgList.hpp>
#include <Chart.hpp>
#include <TeEngine.hpp>
#include <TeeProcs.hpp>
#include <Series.hpp>
#include <TeeShape.hpp>
#include <Menus.hpp>
#include "TeeGDIPlus.hpp"
#include "SoundDllPro_OutputChannelData.h"
#include <vector>
#include <valarray>
//------------------------------------------------------------------------------
// forward declarations
class TTracksForm;
class TTrackInfoFrame;
//------------------------------------------------------------------------------
/// Struct holding info for one data snippet
//------------------------------------------------------------------------------
struct TSampleData
{
   uint64_t  m_nTotalLen;      ///< length of snippet in samples
   uint64_t  m_nLoopCount;     ///< number of loops specified on loading
   uint64_t  m_nLoopLen;       ///< loop length specified on loading
   uint64_t  m_nStartOffset;   ///< start offset specified on loading
   uint64_t  m_nRampLen;       ///< ramp length specified on loading
   uint64_t  m_nLoopRampLen;   ///< loop ramp length specified on loading
   int64_t           m_nStartLastSamples; ///< internal value holding length of first loop
   bool              m_bCrossfade;     ///< flag, if crossfade was specified
   uint64_t          m_nCrossLenL;     ///< left crossfade length specified on loading
   uint64_t          m_nCrossLenR;     ///< right crossfade length specified on loading
   AnsiString        m_strID;          ///< internal unique ID of snippet
   int               m_nStatus;        ///< statis of snippet
   bool              m_bMultiply;      ///< flag if it's a multiplier snippet
   SDPOutputData*    m_psod;           ///< SDPOutputData that corresponds to snippet (source)
   TSampleData*      m_ptsdLast;       ///< link to next snippet
   std::vector<std::valarray<float > > m_vvaf; ///< vector of valrrays for wav data
};
//------------------------------------------------------------------------------


//------------------------------------------------------------------------------
/// form sfor visualization of audio tracks
//------------------------------------------------------------------------------
class TTracksForm : public TForm
{
   __published:	// Von der IDE verwaltete Komponenten
      TStatusBar *sbStatus;
      TPanel *pnlTrackContainer;
      TPanel *pnlTracks;
      TPanel *pnlLeft;
      TPanel *pnlTrackInfo;
      TPanel *pnlTopLeft;
      TPopupMenu *mnuTimeLine;
      TMenuItem *miTime;
      TMenuItem *miSamples;
      TMainMenu *MainMenu;
      TMenuItem *miAbout;
      TBevel *TopBevel;
      TMenuItem *miView;
      TMenuItem *miRefresh;
      TMenuItem *miDisplayFormat;
      TMenuItem *Time1;
      TMenuItem *Samples1;
      TPanel *pnlVertScroll;
      TScrollBar *scbVert;
      TPanel *pnlHorzScroll;
      TScrollBar *scbHorz;
      TPanel *pnlBottom;
      TPanel *pnlBottomLeft;
      TPanel *pnlHorzBtns;
      TSpeedButton *btnHZoomIn;
      TSpeedButton *btnHZoomOut;
      TSpeedButton *btnHZoomAll;
      TPanel *pnlVertBtns;
      TSpeedButton *btnVZoomIn;
      TSpeedButton *btnVZoomOut;
      TSpeedButton *btnVZoomAll;
      TTimer *CursorTimer;
      TChart *chrtTrack;
      TLineSeries *DummySeries;
      TFastLineSeries *CursorSeries;
      TPaintBox *pbCursor;
      void __fastcall pnlTracksResize(TObject *Sender);
      void __fastcall btnVZoomInClick(TObject *Sender);
      void __fastcall btnVZoomOutClick(TObject *Sender);
      void __fastcall btnVZoomAllClick(TObject *Sender);
      void __fastcall btnHZoomOutClick(TObject *Sender);
      void __fastcall btnHZoomInClick(TObject *Sender);
      void __fastcall miTimeClick(TObject *Sender);
      void __fastcall miSamplesClick(TObject *Sender);
      void __fastcall btnHZoomAllClick(TObject *Sender);
      void __fastcall scbHorzChange(TObject *Sender);
      void __fastcall scbVertChange(TObject *Sender);
      void __fastcall miAboutClick(TObject *Sender);
      void __fastcall miRefreshClick(TObject *Sender);
      void __fastcall FormKeyDown(TObject *Sender, WORD &Key,
          TShiftState Shift);
      void __fastcall CursorTimerTimer(TObject *Sender);
      void __fastcall chrtTrackGetAxisLabel(TChartAxis *Sender, TChartSeries *Series,
          int ValueIndex, UnicodeString &LabelText);
      void __fastcall chrtTrackMouseDown(TObject *Sender, TMouseButton Button, TShiftState Shift,
          int X, int Y);
      void __fastcall pbCursorPaint(TObject *Sender);
   private:	// Anwender-Deklarationen
      unsigned int   m_nNumTracks;           ///< total number of tracks
      unsigned int   m_nNumVisibleTracks;    ///< number of visible tracks
      int64_t        m_nCursorPosition;      ///< current cursor position
      int64_t        m_nXViewStart;          ///< start X-position of current view
      int64_t        m_nXViewLen;            ///< current X-view length
      int64_t        m_nXTotalLen;           ///< total X-view length
      int64_t        m_nXMaxTotalLen;        ///< allowed maximum total X-view length
      int64_t        m_nXMaxDataLen;         ///< allowed maximum total X data length
      double         m_dSampleRate;          ///< current sample rate
      bool           m_bShowWaveData;        ///< flag if sample dara (waveform) to be displayed
      bool           m_bBreak;               ///< gloabl break flag
      bool           m_bPositionsRead;       ///< flag if form positions already read from INI
      CRITICAL_SECTION  m_csTrackView;       ///< critical section for syncying
      std::vector<TTrackInfoFrame*>    m_vTrackInfos; ///< vector holding track infos per track
      bool           m_bIsDarkTheme;         ///< flag if current theme is dark theme
      bool           m_bSeriesDarkTheme;     ///< flag if series to be styled as well
      void           SetCursorPosInternal();
      void           AdjustVerticalView();
      void           AdjustHorizontalView();
      AnsiString     CursorPosition();
   public:		// Anwender-Deklarationen
      __fastcall     TTracksForm(TComponent* Owner);
      __fastcall     ~TTracksForm();
      void           Init();
      void           UpdateTracks(bool bZoomToAll = false, bool bShowWaveData = false);
      void           UpdateTrackInfos();
      void           SetCursorPos(int64_t nCursorPos);
      void           SetStatusString(AnsiString str, int nIndex);
      void           SetTrackLevelsInternal(const std::valarray<float>& vaf);
      void           ResetTrackLevelsInternal();
      void           ClearWaveData(TSampleData *psd);
      void           RemoveChartSeries(TChartSeries* pcs);
      uint64_t       UpdateData(bool bRemoveOnly = false);
      void           UpdateWaveData();
      float          GetSampleVis(TSampleData *psd, int64_t nDataPos);
      void __fastcall chrtTrackAfterDraw(TObject *Sender);
      void __fastcall chrtTrackBeforeDrawSeries(TObject *Sender);
      void __fastcall Clear();
};
//------------------------------------------------------------------------------
#endif
