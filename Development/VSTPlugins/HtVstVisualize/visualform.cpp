//------------------------------------------------------------------------------
/// \file visualform.cpp
/// \author Berg
/// \brief Implementation of main form for VST plugin CHtVSTVisualize.
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
#include <limits.h>
#include <math.h>
#pragma hdrstop


#include "visualform.h"
#include "frm_Options.h"
#include "AHtVSTVisualize.h"
//---------------------------------------------------------------------------
#pragma package(smart_init)

#pragma link "MMDIBCv"
#pragma link "MMLevel"
#pragma link "MMObj"
#pragma link "MMOscope"
#pragma link "MMSpectr"
#pragma link "MMSpGram"
#pragma resource "*.dfm"

// #define msg(p) MessageBox(0,p, "j", MB_OK);
#define msg(p) OutputDebugString(p)
#define PLUGIN_VER  MAKEWORD(2,2) //this Version 2.2 !!





//--------------------------------------------------------------------------
///
//--------------------------------------------------------------------------
AnsiString   IniFileName(int iIndex)
{
   return IncludeTrailingBackslash(GetCurrentDir()) + "HtVisualize_" + IntToStr(iIndex) + ".ini";
}

//---------------------------------------------------------------------------
//---------------------------------------------------------------------------
__fastcall TfrmVisual::TfrmVisual( TComponent* Owner)
   :  TForm(Owner), frmOptions(NULL), BufferConnector(NULL)
{
//OutputDebugString(__FUNC__);
try
   {
   //Constructor of TfrmPlugin must have set bIsValid flag to true on success!
   BufferConnector =  new TMMBufferConnector(this);
   BufferConnector->BufSize      = 8192;
   BufferConnector->Level1       = LevelL;
   BufferConnector->Oscope1      = Oscope;
   BufferConnector->Spectrogram1 = Spectrogram;
   BufferConnector->Spectrum1    = Spectrum;
   BufferConnector->Channels     = 1;


   sLastError = "Unknown error";

   if (!pnlLevel->Visible)
      Height -= pnlLevel->Height;
   if (!pnlSpectrum->Visible)
      Height -= pnlSpectrum->Height;
   if (!pnlSpectro->Visible)
      Height -= pnlSpectro->Height;
   if (!pnlWave->Visible)
      Height -= pnlWave->Height;
   }
catch (Exception &e)
   {

//   OutputDebugString("Error at 1");
   sLastError = e.Message;
   }
}
//---------------------------------------------------------------------------


//---------------------------------------------------------------------------
// Destructor
//---------------------------------------------------------------------------
__fastcall TfrmVisual::~TfrmVisual()
{
   try
      {
      WriteIni(m_strIniFile);
      }
   catch (...){;}

   try
      {
      if (BufferConnector)
         delete BufferConnector;
      BufferConnector = NULL;
      }
   catch (...)
      {
      }

   //options formis deletet automatically!
//   if (frmOptions)
//    delete frmOptions;
//   frmOptions = NULL;
}
//---------------------------------------------------------------------------


//---------------------------------------------------------------------------
//
//---------------------------------------------------------------------------
#pragma argsused
void __fastcall TfrmVisual::btnSpectroLogClick(TObject *Sender)
{
   Spectrogram->LogAmp  = btnSpectroLog->Down;
   Spectrum->LogAmp     = btnSpectrumLog->Down;

}
//---------------------------------------------------------------------------

//---------------------------------------------------------------------------
//
//---------------------------------------------------------------------------
#pragma argsused
void __fastcall TfrmVisual::btnSpectrumLogFClick(TObject *Sender)
{
   Spectrum->LogFreq = btnSpectrumLogF->Down;
}
//---------------------------------------------------------------------------


//****************************************************************************
// Sizing
//****************************************************************************
//---------------------------------------------------------------------------
//
//---------------------------------------------------------------------------
#pragma argsused
void __fastcall TfrmVisual::FormShow(TObject *Sender)
{

   if (gbMain->Height)
      {
      fLevelHeight   = (float)pnlLevel->Height/(float)gbMain->Height;
      fWaveHeight    = (float)pnlWave->Height/(float)gbMain->Height;
      fSpecHeight    = (float)pnlSpectrum->Height/(float)gbMain->Height;
      }
   BringToFront();
}
//---------------------------------------------------------------------------

//---------------------------------------------------------------------------
//
//---------------------------------------------------------------------------
#pragma argsused
void __fastcall TfrmVisual::FormCloseQuery(TObject *Sender, bool &CanClose)
{
   CanClose = false;
}
//---------------------------------------------------------------------------

//---------------------------------------------------------------------------
//
//---------------------------------------------------------------------------
#pragma argsused
void __fastcall TfrmVisual::FormResize(TObject *Sender)
{
   int iTmp = floor((float)gbMain->Height*fLevelHeight);
   if (iTmp > Splitter1->MinSize)
      pnlLevel->Height = iTmp;
   iTmp = floor((float)gbMain->Height*fWaveHeight);
   if (iTmp > Splitter2->MinSize)
      pnlWave->Height = iTmp;
   iTmp = floor((float)gbMain->Height*fSpecHeight);
   if (iTmp > Splitter3->MinSize)
      pnlSpectrum->Height = iTmp;
   }
//---------------------------------------------------------------------------

//---------------------------------------------------------------------------
//
//---------------------------------------------------------------------------
#pragma argsused
void __fastcall TfrmVisual::pnlLevelResize(TObject *Sender)
{
   LevelL->Height = (pnlLevel->Height- LevelScale->Height);
}
//---------------------------------------------------------------------------

//---------------------------------------------------------------------------
//
//---------------------------------------------------------------------------
#pragma argsused
void __fastcall TfrmVisual::Splitter1Moved(TObject *Sender)
{
   if (gbMain->Height)
      fLevelHeight = (float)pnlLevel->Height/(float)gbMain->Height;
}
//---------------------------------------------------------------------------

//---------------------------------------------------------------------------
//
//---------------------------------------------------------------------------
#pragma argsused
void __fastcall TfrmVisual::Splitter2Moved(TObject *Sender)
{
   if (gbMain->Height)
      fWaveHeight = (float)pnlWave->Height/(float)gbMain->Height;
}
//---------------------------------------------------------------------------

//---------------------------------------------------------------------------
//
//---------------------------------------------------------------------------
#pragma argsused
void __fastcall TfrmVisual::Splitter3Moved(TObject *Sender)
{
   if (gbMain->Height)
      fSpecHeight = (float)pnlSpectrum->Height/(float)gbMain->Height;
}
//---------------------------------------------------------------------------


//****************************************************************************
// Inifile handling
//****************************************************************************
//---------------------------------------------------------------------------
//
//---------------------------------------------------------------------------
void __fastcall TfrmVisual::ReadIni(AnsiString sIniFile)
{
   sIniFile = ExpandFileName(sIniFile);
   if (ExpandFileName(m_strIniFile) == sIniFile)
      return;

   if (sIniFile.IsEmpty())
      return;
   if (!FileExists(sIniFile))
      return;

   TIniFile *IniFile = NULL;
   try
      {
      IniFile = new TIniFile(sIniFile);
      if (!IniFile)
         throw Exception("error creating Inifile for visualization plugin");
      IniFile->UpdateFile();
      m_strIniFile = sIniFile;

      //--------- General ------------------------------------------------------
      Spectrum->FFTLength        = IniFile->ReadInteger("General","FFTLength", 512);
      Spectrum->FrequencyScale   = IniFile->ReadInteger("General","FreqScale", 2);
      Spectrogram->FFTLength     = IniFile->ReadInteger("General","FFTLength", 512);
      Spectrogram->FrequencyScale= IniFile->ReadInteger("General","FreqScale", 2);
      // is a boundsrect specified?
      Left     = IniFile->ReadInteger("General","Left", 0);
      Top      = IniFile->ReadInteger("General","Top", 0);
      Width    = IniFile->ReadInteger("General","Width", 100);
      Height   = IniFile->ReadInteger("General","Height", 100);

      pnlLevel->Height         = IniFile->ReadInteger("General","LevelHeight", pnlLevel->Height);
      pnlWave->Height          = IniFile->ReadInteger("General","WaveHeight", pnlWave->Height);
      pnlSpectrum->Height      = IniFile->ReadInteger("General","SpectrumHeight", pnlSpectrum->Height);

      //--------- Levelmeter ---------------------------------------------------
      pnlLevel->Visible    = !IniFile->ReadBool("Level", "Hide", false);

      //--------- Wave ---------------------------------------------------------
      pnlWave->Visible        = !IniFile->ReadBool("Wave", "Hide", false);
      Oscope->DrawAmpScale    = IniFile->ReadBool("Wave", "DrawAmpScale", false);
      Oscope->DrawTimeScale   = IniFile->ReadBool("Wave", "DrawTimeScale", false);
      Oscope->DrawGrid        = IniFile->ReadBool("Wave", "DrawGrid", false);
      Oscope->DrawMidLine     = IniFile->ReadBool("Wave", "DrawMidLine", false);
      Oscope->Scroll          = IniFile->ReadBool("Wave", "ScrollDisplay", false);
      Oscope->SetExtendedZoom(IniFile->ReadInteger("Wave", "Zoom", 1));
      Oscope->SetExtendedGain(IniFile->ReadInteger("Wave", "Gain", 0));

      //--------- Spectrum -----------------------------------------------------
      pnlSpectrum->Visible    = !IniFile->ReadBool("Spectrum", "Hide", false);
      Spectrum->DrawAmpScale  = IniFile->ReadBool("Spectrum", "DrawAmpScale", false);
      Spectrum->DrawFreqScale = IniFile->ReadBool("Spectrum", "DrawFreqScale", false);
      Spectrum->DrawGrid      = IniFile->ReadBool("Spectrum", "DrawGrid", false);
      Spectrum->DrawInactive  = IniFile->ReadBool("Spectrum", "DrawInactive", false);
      Spectrum->DisplayPeak   = IniFile->ReadBool("Spectrum", "DisplayPeak", false);
      Spectrum->LogAmp        = IniFile->ReadBool("Spectrum", "LogAmp", false);
      Spectrum->LogFreq       = IniFile->ReadBool("Spectrum", "LogFreq", false);

      int i                   = IniFile->ReadInteger("Spectrum", "DisplayType", 3);
      if (i > 5) i = 5; if (i < 0) i = 0;
      Spectrum->Kind          = TMMSpectrumKind(i);
      Spectrum->SetLogBase(IniFile->ReadInteger("Spectrum", "Gain", 6));

      btnSpectrumLog->Down    =  Spectrum->LogAmp;
      btnSpectrumLogF->Down   =  Spectrum->LogFreq;

      //--------- Spectrogram --------------------------------------------------
      pnlSpectro->Visible     = !IniFile->ReadBool("Spectrogram", "Hide", false);
      Spectrogram->DrawScale  = IniFile->ReadBool("Spectrogram", "DrawFreqScale", false);
      Spectrogram->Scroll     = IniFile->ReadBool("Spectrogram", "ScrollDisplay", false);
      Spectrogram->LogAmp     = IniFile->ReadBool("Spectrogram", "LogAmp", false);
      btnSpectroLog->Down     = Spectrogram->LogAmp;
      }
   __finally
      {
      if (IniFile)
         delete IniFile;
      }
}
//---------------------------------------------------------------------------

//---------------------------------------------------------------------------
//
//---------------------------------------------------------------------------
void __fastcall TfrmVisual::WriteIni(AnsiString sIniFile)
{
   if (sIniFile.IsEmpty())
      return;
   m_strIniFile = ExpandFileName(sIniFile);
   TIniFile *IniFile = NULL;
   try
      {
      IniFile = new TIniFile(sIniFile);
      if (!IniFile)
         throw Exception("error creating Inifile for visualization plugin");
      
      //--------- General -----------------------------------------------------
      IniFile->WriteInteger("General","FFTLength", Spectrum->FFTLength);
      IniFile->WriteInteger("General","FreqScale", Spectrum->FrequencyScale);
      // if we are actually undocked we have to retrieve actual BoundsRect!
      IniFile->WriteInteger("General","Left", Left);
      IniFile->WriteInteger("General","Top", Top);
      IniFile->WriteInteger("General","Width", Width);
      IniFile->WriteInteger("General","Height", Height);
      IniFile->WriteInteger("General","LevelHeight", pnlLevel->Height);
      IniFile->WriteInteger("General","WaveHeight", pnlWave->Height);
      IniFile->WriteInteger("General","SpectrumHeight", pnlSpectrum->Height);

      //--------- Level -----------------------------------------------------
      IniFile->WriteBool("Level", "Hide", !pnlLevel->Visible);

      //--------- Wave -----------------------------------------------------
      IniFile->WriteBool("Wave", "Hide", !pnlWave->Visible);
      IniFile->WriteBool("Wave", "DrawAmpScale", Oscope->DrawAmpScale);
      IniFile->WriteBool("Wave", "DrawTimeScale", Oscope->DrawTimeScale);
      IniFile->WriteBool("Wave", "DrawGrid", Oscope->DrawGrid);
      IniFile->WriteBool("Wave", "DrawMidLine", Oscope->DrawMidLine);
      IniFile->WriteBool("Wave", "ScrollDisplay", Oscope->Scroll);
      IniFile->WriteInteger("Wave", "Zoom", Oscope->Zoom);
      IniFile->WriteInteger("Wave", "Gain", Oscope->Gain);


      //--------- Spectrum -----------------------------------------------------
      IniFile->WriteBool("Spectrum", "Hide", !pnlSpectrum->Visible);
      IniFile->WriteBool("Spectrum", "DrawAmpScale", Spectrum->DrawAmpScale);
      IniFile->WriteBool("Spectrum", "DrawFreqScale", Spectrum->DrawFreqScale);
      IniFile->WriteBool("Spectrum", "DrawGrid", Spectrum->DrawGrid);
      IniFile->WriteBool("Spectrum", "DrawInactive", Spectrum->DrawInactive);
      IniFile->WriteBool("Spectrum", "DisplayPeak", Spectrum->DisplayPeak);
      IniFile->WriteInteger("Spectrum", "DisplayType", (int)Spectrum->Kind);
      IniFile->WriteInteger("Spectrum", "Gain", Spectrum->GetLogBase());
      IniFile->WriteBool("Spectrum", "LogAmp", Spectrum->LogAmp);
      IniFile->WriteBool("Spectrum", "LogFreq", Spectrum->LogFreq);



      //--------- Spectrogram -----------------------------------------------------
      IniFile->WriteBool("Spectrogram", "Hide", !pnlSpectro->Visible);
      IniFile->WriteBool("Spectrogram", "DrawFreqScale", Spectrogram->DrawScale);
      IniFile->WriteBool("Spectrogram", "ScrollDisplay", Spectrogram->Scroll);
      IniFile->WriteBool("Spectrogram", "LogAmp", Spectrogram->LogAmp);

      IniFile->UpdateFile();
      }
   __finally
      {
      if (IniFile)
         delete IniFile;
      }

}
//---------------------------------------------------------------------------

//---------------------------------------------------------------------------
//
//---------------------------------------------------------------------------
#pragma argsused
void __fastcall TfrmVisual::Options1Click(TObject *Sender)
{
   if (!frmOptions)
      {
      frmOptions = new TfrmOptions(this);
      if (!frmOptions)
         throw Exception("Error creating visualization options dialog");
      //write actual settings to options dialog!

      frmOptions->SpinFFTLen->Value    = Spectrum->FFTLength;
      frmOptions->SpinFreqScale->Value = Spectrum->FrequencyScale;
                        
      frmOptions->cbDrawWaveAmpScale->Checked   = Oscope->DrawAmpScale;
      frmOptions->cbDrawWaveTimeScale->Checked  = Oscope->DrawTimeScale;
      frmOptions->cbDrawWaveGrid->Checked       = Oscope->DrawGrid;
      frmOptions->cbDrawWaveMidLine->Checked    = Oscope->DrawMidLine;
      frmOptions->cbScrollWave->Checked         = Oscope->Scroll;

      frmOptions->cbDrawSpectrumAmpScale->Checked  = Spectrum->DrawAmpScale;
      frmOptions->cbDrawSpectrumFreqScale->Checked = Spectrum->DrawFreqScale;
      frmOptions->cbSpectrumDrawGrid->Checked      = Spectrum->DrawGrid;
      frmOptions->cbSpectrumDrawInactive->Checked  = Spectrum->DrawInactive;
      frmOptions->cbSpectrumDisplayPeak->Checked   = Spectrum->DisplayPeak;
      frmOptions->SwitchSpectrumKind->Position     = (int)Spectrum->Kind;

      frmOptions->cbDrawSpectrogramFreqScale->Checked = Spectrogram->DrawScale;
      frmOptions->cbScrollSpectrogram->Checked        = Spectrogram->LogAmp;
      }

   frmOptions->Show();
}
//---------------------------------------------------------------------------

//---------------------------------------------------------------------------
//
//---------------------------------------------------------------------------
#pragma argsused
void __fastcall TfrmVisual::btnSpectrumZoomInClick(TObject *Sender)
{
   if (Spectrum->LogAmp)
      {
      Spectrum->SetLogBase(Spectrum->GetLogBase()+1);
      }
   else
      {
      int iNew;
      if (Spectrum->VerticalScale < 15)
         iNew = Spectrum->VerticalScale - 1;
      else
         iNew = Spectrum->VerticalScale - 10;
      if (iNew > 100)
         Spectrum->VerticalScale = 100;
      else
         Spectrum->VerticalScale = iNew;
      }
}
//---------------------------------------------------------------------------

//---------------------------------------------------------------------------
//
//---------------------------------------------------------------------------
#pragma argsused
void __fastcall TfrmVisual::btnSpectrumZoomOutClick(TObject *Sender)
{
   if (Spectrum->LogAmp)
      {
      Spectrum->SetLogBase(Spectrum->GetLogBase()-1);
      }
   else
      {
      int iNew;
      if (Spectrum->VerticalScale < 10)
         iNew = Spectrum->VerticalScale + 1;
      else
         iNew = Spectrum->VerticalScale + 10;
      if (iNew > 100)
         Spectrum->VerticalScale = 100;
      else
         Spectrum->VerticalScale = iNew;
      }

}
//---------------------------------------------------------------------------

//---------------------------------------------------------------------------
//
//---------------------------------------------------------------------------
#pragma argsused
void __fastcall TfrmVisual::btnWaveZoomInMouseDown(TObject *Sender,
      TMouseButton Button, TShiftState Shift, int X, int Y)
{
int iChange = ((TSpeedButton*)Sender)->Tag;
if (Shift.Contains(ssShift))
   iChange *= 10;

if (Oscope->Gain + iChange < 0)
   Oscope->Gain = 0;
else
   {
   Oscope->SetExtendedGain(Oscope->Gain + iChange);
   }
}
//---------------------------------------------------------------------------

void __fastcall TfrmVisual::btnWaveHZoomInClick(TObject *Sender)
{
int iNew = Oscope->Zoom + ((TSpeedButton*)Sender)->Tag;
//Tag haelt Vorzeichen!


//0 muss uebersprungen werden!
if (iNew == 0)
   {
   if (((TSpeedButton*)Sender)->Tag > 0)
      Oscope->Zoom = 1;
   else
      Oscope->Zoom = -1;
   }
else
   {
   Oscope->SetExtendedZoom(iNew);
   }
}
//---------------------------------------------------------------------------



