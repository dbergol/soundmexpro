//------------------------------------------------------------------------------
/// \file visualform.h
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
#ifndef visualformH
#define visualformH
//---------------------------------------------------------------------------
#include <Classes.hpp>
#include <Controls.hpp>
#include <StdCtrls.hpp>
#include <Forms.hpp>

#include <ExtCtrls.hpp>
#include <Buttons.hpp>
#include <Menus.hpp>
#include <ComCtrls.hpp>
#include <ToolWin.hpp>

#include "MMBufConnect.h"
#include "MMDIBCv.hpp"
#include "MMLevel.hpp"
#include "MMObj.hpp"
#include "MMOscope.hpp"
#include "MMSpectr.hpp"
#include "MMSpGram.hpp"
class TfrmOptions;

//---------------------------------------------------------------------------
class TfrmVisual : public TForm
{
__published:	// Von der IDE verwaltete Komponenten
   TPopupMenu *PopupMenu;
   TMenuItem *Options1;
   TGroupBox *gbMain;
   TSplitter *Splitter1;
   TSplitter *Splitter2;
   TSplitter *Splitter3;
   TPanel *pnlLevel;
   TMMLevel *LevelL;
   TMMLevelScale *LevelScale;
   TPanel *pnlSpectrum;
   TMMSpectrum *Spectrum;
   TPanel *pnlScopBtns;
   TSpeedButton *btnSpectrumLog;
   TSpeedButton *btnSpectrumLogF;
   TSpeedButton *btnSpectrumZoomIn;
   TSpeedButton *btnSpectrumZoomOut;
   TPanel *pnlSpectro;
   TMMSpectrogram *Spectrogram;
   TPanel *pnlSpectroBtns;
   TSpeedButton *btnSpectroLog;
   TPanel *pnlWave;
   TMMOscope *Oscope;
   TPanel *pnlOscBtns;
   TSpeedButton *btnWaveZoomIn;
   TSpeedButton *btnWaveZoomOut;
   TSpeedButton *btnWaveHZoomIn;
   TSpeedButton *btnWaveHZoomOut;
   void __fastcall pnlLevelResize(TObject *Sender);
   void __fastcall FormCloseQuery(TObject *Sender, bool &CanClose);
   void __fastcall btnSpectroLogClick(TObject *Sender);
   void __fastcall FormResize(TObject *Sender);
   void __fastcall FormShow(TObject *Sender);
   void __fastcall Splitter1Moved(TObject *Sender);
   void __fastcall Splitter2Moved(TObject *Sender);
   void __fastcall Splitter3Moved(TObject *Sender);
   void __fastcall btnSpectrumLogFClick(TObject *Sender);
   void __fastcall Options1Click(TObject *Sender);
   void __fastcall btnSpectrumZoomInClick(TObject *Sender);
   void __fastcall btnSpectrumZoomOutClick(TObject *Sender);
   void __fastcall btnWaveZoomInMouseDown(TObject *Sender,
          TMouseButton Button, TShiftState Shift, int X, int Y);
   void __fastcall btnWaveHZoomInClick(TObject *Sender);
private:	// Anwender-Deklarationen
   float fLevelHeight;
   float fWaveHeight;
   float fSpecHeight;
   TfrmOptions *frmOptions;
public:		// Anwender-Deklarationen
   __fastcall TfrmVisual( TComponent* Owner);
   __fastcall ~TfrmVisual();
   LPCSTR StringCommand(LPCSTR lpszCmd);
   AnsiString sLastError;
   TMMBufferConnector *BufferConnector;
   void __fastcall ReadIni(AnsiString sIniFile);
   void __fastcall WriteIni(AnsiString sIniFile);
   AnsiString  m_strIniFile;
};
//---------------------------------------------------------------------------
extern PACKAGE TfrmVisual *frmVisual;
//---------------------------------------------------------------------------
#endif
