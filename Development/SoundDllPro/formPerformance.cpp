//------------------------------------------------------------------------------
/// \file formPerformance.cpp
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
#include <vcl.h>
#include <math.h>
#pragma hdrstop

#include "formPerformance.h"
#include "SoundDllPro_Main.h"
//------------------------------------------------------------------------------
#pragma package(smart_init)
#pragma resource "*.dfm"

using namespace Asio;

static const char* g_lpcszCounters[PERF_COUNTER_LAST] =
   {
   "DSP",
   "VST-Track",
   "VST-Master",
   "MATLAB-Plugin",
   "Record",
   "Visualize"
   };
//------------------------------------------------------------------------------

//------------------------------------------------------------------------------
/// contructir, initializes listview
//------------------------------------------------------------------------------
__fastcall TPerformanceForm::TPerformanceForm(TComponent* Owner)
   : TForm(Owner)
{
   TListItem *pli;
   for (int i = 0; i < PERF_COUNTER_LAST; i++)
      {
      pli = lv->Items->Add();
      pli->Caption = g_lpcszCounters[i];
      pli->SubItems->Add("");
      pli->SubItems->Add("");
      }
   #ifdef PERFORMANCE_TEST
   for (int i = 0; i < NUM_TESTCOUNTER; i++)
      {
      pli = lv->Items->Add();
      pli->Caption = "Test " + IntToStr(i);
      pli->SubItems->Add("");
      pli->SubItems->Add("");
      }
   #endif

}
//------------------------------------------------------------------------------

//------------------------------------------------------------------------------
/// form callback to deny form closure
//------------------------------------------------------------------------------
#pragma argsused
void __fastcall TPerformanceForm::FormCloseQuery(TObject *Sender, bool &CanClose)
{
   CanClose = false;
}
//------------------------------------------------------------------------------

//------------------------------------------------------------------------------
/// timer callback: show performance measures
//------------------------------------------------------------------------------
#pragma argsused
void __fastcall TPerformanceForm::PerformanceTimerTimer(TObject *Sender)
{
   PerformanceTimer->Enabled = false;

   if (SoundClass() && Visible)
      {
      double dSecondsPerBuffer = SoundClass()->SecondsPerBuffer();
      int n;
      for (int i = 0; i < PERF_COUNTER_LAST; i++)
         {
         n = (int)floor(100.0*SoundClass()->m_pcProcess[PERF_COUNTER_DSP+i].DecayValue()/dSecondsPerBuffer);
         lv->Items->Item[i]->SubItems->Strings[0] = IntToStr(n);
         n = (int)floor(100.0*SoundClass()->m_pcProcess[PERF_COUNTER_DSP+i].MaxValue()/dSecondsPerBuffer);
         lv->Items->Item[i]->SubItems->Strings[1] = IntToStr(n);
         }

      std::valarray<unsigned int>& rvan = SoundClass()->GetXruns();

      AnsiString as = "xruns: " + IntToStr((int)rvan.sum()) + " (";

      if (SoundClass()->BufferedIO())
         as += IntToStr((int)rvan[XR_PROC]);
      else
         as += IntToStr((int)rvan[XR_RT]);
      as += "/" + IntToStr((int)rvan[XR_DONE]) + ")";


      sb->SimpleText = as;

      #ifdef PERFORMANCE_TEST
      for (int i = 0; i < NUM_TESTCOUNTER; i++)
         {
         n = (int)floor(100.0*SoundClass()->m_pcTest[i].DecayValue()/dSecondsPerBuffer);
         lv->Items->Item[PERF_COUNTER_LAST+i]->SubItems->Strings[0] = IntToStr(n);
         n = (int)floor(100.0*SoundClass()->m_pcTest[i].MaxValue()/dSecondsPerBuffer);
         lv->Items->Item[PERF_COUNTER_LAST+i]->SubItems->Strings[1] = IntToStr(n);
         }
      #endif
   }
   PerformanceTimer->Enabled = true;
}
//------------------------------------------------------------------------------

