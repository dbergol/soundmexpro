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
#ifndef eqinputH
#define eqinputH
//---------------------------------------------------------------------------

#include <Classes.hpp>
#include <Controls.hpp>
#include <StdCtrls.hpp>
#include <Forms.hpp>
#include <ComCtrls.hpp>
#include <ExtCtrls.hpp>
#include <ImgList.hpp>
#include <ToolWin.hpp>
#include <Buttons.hpp>
#include <Dialogs.hpp>
#include "Chart.hpp"
#include "Series.hpp"
#include "TeEngine.hpp"
#include "TeeProcs.hpp"






//---------------------------------------------------------------------------
class CHtVSTEqualizer;
class TfrmEqInput : public TForm
{
__published:	// Von der IDE verwaltete Komponenten
   TOpenDialog *od;
   TSaveDialog *sd;
   TChart *Chart;
   TFastLineSeries *PreSpecSeriesL;
   TFastLineSeries *PostSpecSeriesL;
   TFastLineSeries *FilterSeriesL;
   TPanel *Panel1;
   TSpeedButton *btnPaint;
   TSpeedButton *btnZoom;
   TSpeedButton *btnSpec;
   TSpeedButton *btnFlat;
   TSpeedButton *btnUndoZoom;
   TSpeedButton *btnLeft;
   TSpeedButton *btnRight;
   TFastLineSeries *FilterSeriesR;
   TSpeedButton *btnShift;
   TSpeedButton *btnFast;
   TFastLineSeries *PostSpecSeriesR;
   TFastLineSeries *PreSpecSeriesR;
   TSpeedButton *btnInput;
   TSpeedButton *btnOutput;
   TSpeedButton *btnLoad;
   TSpeedButton *btnSave;
   TFastLineSeries *FileFilterL;
   TFastLineSeries *FileFilterR;
   TLabel *lbXPos;
   TLabel *Label2;
   TLabel *lbYPos;
   TStatusBar *sb;
   TSpeedButton *btnSkip;
   TSpeedButton *btnMute;
   TLineSeries *Series1;
   void __fastcall btnLoadClick(TObject *Sender);
   void __fastcall btnSaveClick(TObject *Sender);
   void __fastcall btnZoomClick(TObject *Sender);
   void __fastcall btnPaintClick(TObject *Sender);
   void __fastcall ChartMouseMove(TObject *Sender, TShiftState Shift,
          int X, int Y);
   void __fastcall ChartMouseUp(TObject *Sender, TMouseButton Button,
          TShiftState Shift, int X, int Y);
   void __fastcall btnFlatClick(TObject *Sender);
   void __fastcall ChartMouseDown(TObject *Sender, TMouseButton Button,
          TShiftState Shift, int X, int Y);
   void __fastcall btnUndoZoomClick(TObject *Sender);
   void __fastcall btnFastClick(TObject *Sender);
   void __fastcall btnSpecClick(TObject *Sender);
   void __fastcall btnLeftClick(TObject *Sender);
   void __fastcall btnRightClick(TObject *Sender);
   void __fastcall btnInputClick(TObject *Sender);
   void __fastcall btnOutputClick(TObject *Sender);
   void __fastcall btnMuteClick(TObject *Sender);
   void __fastcall btnSkipClick(TObject *Sender);
   void __fastcall ChartBeforeDrawAxes(TObject *Sender);

private:	// Anwender-Deklarationen

   int iOldXPos;
   int iCapturedValue;   
   bool bChanged;
   CHtVSTEqualizer *m_pEqualizer;
public:		// Anwender-Deklarationen
   __fastcall TfrmEqInput(CHtVSTEqualizer *pEqualizer);
   void __fastcall DisableSpectra();
   void __fastcall EnableSpectra();
   void __fastcall Initialize(unsigned int nFFTLen, float fSampleRate);
   void __fastcall SetFilterProperties();
};
//---------------------------------------------------------------------------

//---------------------------------------------------------------------------
#endif
