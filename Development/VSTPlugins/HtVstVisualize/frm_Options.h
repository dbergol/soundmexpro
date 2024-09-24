//------------------------------------------------------------------------------
/// \file frm_Options.h
/// \author Berg
/// \brief Implementation options form for VST plugin CHtVSTVisualize.
///
/// Project SoundMexPro
/// Module  HtVSTVisualize.dll
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
#ifndef frm_OptionsH
#define frm_OptionsH
//---------------------------------------------------------------------------
#include <Classes.hpp>
#include <Controls.hpp>
#include <StdCtrls.hpp>
#include <Forms.hpp>
#include <ExtCtrls.hpp>

#include "MMLEDS.hpp"
#include "MMObj.hpp"

#include "MMSwitch.hpp"
#include <Dialogs.hpp>
//---------------------------------------------------------------------------
class TfrmVisual;

class TfrmOptions : public TForm
{
__published:	// Von der IDE verwaltete Komponenten
   TButton *btnOk;
   TGroupBox *gbSpectrum;
   TGroupBox *gbSpectrogram;
   TGroupBox *gbWave;
   TCheckBox *cbScrollWave;
   TGroupBox *gbGeneral;
   TMMLEDSpin *SpinFFTLen;
   TButton *Button3;
   TButton *Button4;
   TMMLEDSpin *SpinFreqScale;
   TCheckBox *cbScrollSpectrogram;
   TMMSwitch *SwitchSpectrumKind;
   TCheckBox *cbDrawSpectrumAmpScale;
   TCheckBox *cbDrawSpectrumFreqScale;
   TLabel *lbDots;
   TLabel *lbLines;
   TLabel *lbVLines;
   TLabel *lbPeaks;
   TLabel *lbBars;
   TLabel *lbScroll;
   TLabel *Label1;
   TCheckBox *cbSpectrumDrawGrid;
   TCheckBox *cbSpectrumDrawInactive;
   TCheckBox *cbSpectrumDisplayPeak;
   TCheckBox *cbDrawSpectrogramFreqScale;
   TLabel *Label2;
   TLabel *Label3;
   TCheckBox *cbDrawWaveAmpScale;
   TCheckBox *cbDrawWaveTimeScale;
   TCheckBox *cbDrawWaveGrid;
   TCheckBox *cbDrawWaveMidLine;
   TGroupBox *gbSaveAs;
   TButton *btnSaveAs;
   TSaveDialog *sd;
   void __fastcall cbScrollWaveClick(TObject *Sender);
   void __fastcall SpinFFTLenDownClick(TObject *Sender);
   void __fastcall SpinFFTLenUpClick(TObject *Sender);
   void __fastcall SpinFreqScaleUpClick(TObject *Sender);
   void __fastcall SpinFreqScaleDownClick(TObject *Sender);
   void __fastcall cbScrollSpectrogramClick(TObject *Sender);
   void __fastcall cbDrawSpectrumAmpScaleClick(TObject *Sender);
   void __fastcall cbDrawSpectrumFreqScaleClick(TObject *Sender);
   void __fastcall SwitchSpectrumKindChange(TObject *Sender);
   void __fastcall cbSpectrumDrawGridClick(TObject *Sender);
   void __fastcall cbSpectrumDrawInactiveClick(TObject *Sender);
   void __fastcall btnOkClick(TObject *Sender);
   void __fastcall cbSpectrumDisplayPeakClick(TObject *Sender);
   void __fastcall cbDrawSpectrogramFreqScaleClick(TObject *Sender);
   void __fastcall cbDrawWaveAmpScaleClick(TObject *Sender);
   void __fastcall cbDrawWaveTimeScaleClick(TObject *Sender);
   void __fastcall cbDrawWaveGridClick(TObject *Sender);
   void __fastcall cbDrawWaveMidLineClick(TObject *Sender);
   void __fastcall btnSaveAsClick(TObject *Sender);
private:	// Anwender-Deklarationen
   TfrmVisual *frmParent; 
public:		// Anwender-Deklarationen
   __fastcall TfrmOptions(TfrmVisual* Owner);
};
//---------------------------------------------------------------------------
extern PACKAGE TfrmOptions *frmOptions;
//---------------------------------------------------------------------------
#endif
