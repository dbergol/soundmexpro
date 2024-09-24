//------------------------------------------------------------------------------
/// \file frameMixer.h
/// \author Berg
/// \brief Implementation of frame class TMixerFrame used by TMixerForm
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
#ifndef frameMixerH
#define frameMixerH
//------------------------------------------------------------------------------
#include <Classes.hpp>
#include <Controls.hpp>
#include <StdCtrls.hpp>
#include <Forms.hpp>
#include "SoundDllPro_Main.h"
#include "TrackBarEx.h"
#include <ComCtrls.hpp>
#include <ExtCtrls.hpp>
#include <Buttons.hpp>
#include "formMixer.h"
#include "HTLevel.h"
//------------------------------------------------------------------------------
// forward declaration
class TMixerForm;

//------------------------------------------------------------------------------
/// TFrame for showing mixer for one channel, used by TTracksForm
//------------------------------------------------------------------------------
class TMixerFrame : public TFrame
{
   __published:	// Von der IDE verwaltete Komponenten
      TPanel *pnl;
      TPanel *pnlInfo;
      TPanel *pnlLevelMeter;
      TTrackBarEx *Slider;
      TPanel *pnlGain;
      TPanel *pnlLink;
      TPanel *pnlMute;
      TPanel *pnlSolo;
      TLabel *lbRoutingIn;
      TLabel *lbName;
      TLabel *lbRoutingOut;
      TPanel *pnlClip;
      THTLevel *LevelMeter;
      TPanel *pnlLevel;
      TLabel *lbLevel;
      void __fastcall SliderChange(TObject *Sender);
      void __fastcall pnlSoloClick(TObject *Sender);
      void __fastcall pnlMuteClick(TObject *Sender);
      void __fastcall SliderDblClick(TObject *Sender);
      void __fastcall SliderCustomDraw(TTrackBarEx *Sender,
          TrackBarExItem Item, const TRect &ARect, TCustomDrawState State,
          bool &DefaultDraw);
      void __fastcall pnlLinkClick(TObject *Sender);
      void __fastcall SliderKeyDown(TObject *Sender, WORD &Key,
             TShiftState Shift);
      void __fastcall ledClipClick(TObject *Sender);
   private:	// Anwender-Deklarationen
      TMixerForm*       m_pMixerForm;
      void              SetGainCaption(float f);
   public:		// Anwender-Deklarationen
      TChannelType      m_ct;
      __fastcall TMixerFrame( TComponent* Owner,
                              TMixerForm* pMixerForm,
                              TChannelType ct,
                              int nIndex
                              );
      void UpdateLevel(float f, unsigned int nClip);
      void ResetLevel();
      void UpdateSlider(float f, bool bPropagateChange = false);
      void UpdateSlider(int n);
      void UpdateMute(bool b);
      void UpdateSolo(bool b);
      void UpdateLink(bool b);
      bool IsMuted();
      bool IsSolo();
      bool IsLink();
      static int GetHeight();
      void SetRouting(AnsiString strOut, AnsiString strIn = "");
      void SetMixerName(AnsiString str);
};
//------------------------------------------------------------------------------
#endif
