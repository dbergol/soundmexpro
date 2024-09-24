//------------------------------------------------------------------------------
/// \file frm_Options.cpp
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
#include <vcl.h>
#pragma hdrstop

#include "frm_Options.h"
#include "visualform.h"
//---------------------------------------------------------------------------
#pragma package(smart_init)

#pragma link "MMLEDS"
#pragma link "MMObj"

#pragma link "MMSwitch"
#pragma resource "*.dfm"


//****************************************************************************
// Dialog
//****************************************************************************
//---------------------------------------------------------------------------
__fastcall TfrmOptions::TfrmOptions(TfrmVisual* Owner)
   : TForm(Owner)
{
   if (!Owner)
      throw Exception("Invalid pointer passed to visualize options dialog");
   frmParent = (TfrmVisual*)Owner;
   sd->InitialDir = GetCurrentDir();
}
//---------------------------------------------------------------------------
#pragma argsused
void __fastcall TfrmOptions::btnOkClick(TObject *Sender)
{
   Close();
}
//---------------------------------------------------------------------------
#pragma argsused
void __fastcall TfrmOptions::btnSaveAsClick(TObject *Sender)
{
   FormStyle = fsNormal;
   if (sd->Execute())
      frmParent->WriteIni(sd->FileName);
   FormStyle = fsStayOnTop;
}


//****************************************************************************
// General
//****************************************************************************
//---------------------------------------------------------------------------
#pragma argsused
void __fastcall TfrmOptions::SpinFFTLenDownClick(TObject *Sender)
{
   frmParent->Spectrum->FFTLength /= 2;
   frmParent->Spectrogram->FFTLength /= 2;
   SpinFFTLen->Value = frmParent->Spectrum->FFTLength;
}
//---------------------------------------------------------------------------
#pragma argsused
void __fastcall TfrmOptions::SpinFFTLenUpClick(TObject *Sender)
{
   frmParent->Spectrum->FFTLength *= 2;
   frmParent->Spectrogram->FFTLength *= 2;
   SpinFFTLen->Value = frmParent->Spectrum->FFTLength;
}
//---------------------------------------------------------------------------
#pragma argsused
void __fastcall TfrmOptions::SpinFreqScaleUpClick(TObject *Sender)
{
   frmParent->Spectrum->FrequencyScale    *= 2;
   frmParent->Spectrogram->FrequencyScale *= 2;
   SpinFreqScale->Value = frmParent->Spectrum->FrequencyScale;
}
//---------------------------------------------------------------------------
#pragma argsused
void __fastcall TfrmOptions::SpinFreqScaleDownClick(TObject *Sender)
{
   frmParent->Spectrum->FrequencyScale    /= 2;
   frmParent->Spectrogram->FrequencyScale /= 2;
   SpinFreqScale->Value = frmParent->Spectrum->FrequencyScale;
}


//****************************************************************************
// Spectrum
//****************************************************************************
#pragma argsused
void __fastcall TfrmOptions::cbDrawSpectrumAmpScaleClick(TObject *Sender)
{
   frmParent->Spectrum->DrawAmpScale = cbDrawSpectrumAmpScale->Checked;
}
//---------------------------------------------------------------------------
#pragma argsused
void __fastcall TfrmOptions::cbDrawSpectrumFreqScaleClick(TObject *Sender)
{
   frmParent->Spectrum->DrawFreqScale = cbDrawSpectrumFreqScale->Checked;
}
//---------------------------------------------------------------------------
#pragma argsused
void __fastcall TfrmOptions::cbSpectrumDrawGridClick(TObject *Sender)
{
   frmParent->Spectrum->DrawGrid = cbSpectrumDrawGrid->Checked;
}
//---------------------------------------------------------------------------
#pragma argsused
void __fastcall TfrmOptions::cbSpectrumDrawInactiveClick(TObject *Sender)
{
   frmParent->Spectrum->DrawInactive = cbSpectrumDrawInactive->Checked;
}
//---------------------------------------------------------------------------
#pragma argsused
void __fastcall TfrmOptions::cbSpectrumDisplayPeakClick(TObject *Sender)
{
   frmParent->Spectrum->DisplayPeak = cbSpectrumDisplayPeak->Checked;
}
//---------------------------------------------------------------------------
#pragma argsused
void __fastcall TfrmOptions::SwitchSpectrumKindChange(TObject *Sender)
{
   frmParent->Spectrum->Kind = TMMSpectrumKind(SwitchSpectrumKind->Position);
}

//****************************************************************************
// Spectrogramm
//****************************************************************************
#pragma argsused
void __fastcall TfrmOptions::cbDrawSpectrogramFreqScaleClick(
      TObject *Sender)
{
   frmParent->Spectrogram->DrawScale = cbDrawSpectrogramFreqScale->Checked;
}
//---------------------------------------------------------------------------
#pragma argsused
void __fastcall TfrmOptions::cbScrollSpectrogramClick(TObject *Sender)
{
   frmParent->Spectrogram->Scroll = cbScrollSpectrogram->Checked;
}
//---------------------------------------------------------------------------


//****************************************************************************
// Wave
//****************************************************************************
#pragma argsused
void __fastcall TfrmOptions::cbDrawWaveAmpScaleClick(TObject *Sender)
{
   frmParent->Oscope->DrawAmpScale = cbDrawWaveAmpScale->Checked;
}
//---------------------------------------------------------------------------
#pragma argsused
void __fastcall TfrmOptions::cbDrawWaveTimeScaleClick(TObject *Sender)
{
   frmParent->Oscope->DrawTimeScale = cbDrawWaveTimeScale->Checked;
}
//---------------------------------------------------------------------------
#pragma argsused
void __fastcall TfrmOptions::cbDrawWaveGridClick(TObject *Sender)
{
   frmParent->Oscope->DrawGrid = cbDrawWaveGrid->Checked;
}
//---------------------------------------------------------------------------
#pragma argsused
void __fastcall TfrmOptions::cbDrawWaveMidLineClick(TObject *Sender)
{
   frmParent->Oscope->DrawMidLine = cbDrawWaveMidLine->Checked;
}
//---------------------------------------------------------------------------
#pragma argsused
void __fastcall TfrmOptions::cbScrollWaveClick(TObject *Sender)
{
   frmParent->Oscope->Scroll = cbScrollWave->Checked;
}
//---------------------------------------------------------------------------




//---------------------------------------------------------------------------



