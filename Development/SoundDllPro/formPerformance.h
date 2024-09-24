//------------------------------------------------------------------------------
/// \file formPerformance.h
/// \author Berg
/// \brief Implementation a form for debugging purposes (showing some performance
/// measures)
///
/// Project SoundMexPro
/// Module  SoundDllPro.dll
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
#ifndef formPerformanceH
#define formPerformanceH
//------------------------------------------------------------------------------
#include <Classes.hpp>
#include <Controls.hpp>
#include <StdCtrls.hpp>
#include <Forms.hpp>
#include <ComCtrls.hpp>
#include <ExtCtrls.hpp>
//------------------------------------------------------------------------------
/// Form for showing performance measures (debugginf)
//------------------------------------------------------------------------------
class TPerformanceForm : public TForm
{
__published:	// Von der IDE verwaltete Komponenten
   TListView *lv;
   TStatusBar *sb;
   TTimer *PerformanceTimer;
   void __fastcall FormCloseQuery(TObject *Sender, bool &CanClose);
   void __fastcall PerformanceTimerTimer(TObject *Sender);
private:	// Anwender-Deklarationen
public:		// Anwender-Deklarationen
   __fastcall TPerformanceForm(TComponent* Owner);
};
//------------------------------------------------------------------------------
#endif
