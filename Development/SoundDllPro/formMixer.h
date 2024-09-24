//------------------------------------------------------------------------------
/// \file formMixer.h
/// \author Berg
/// \brief Implementation of class TMixerForm for visualization of audio data
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
#ifndef formMixerH
#define formMixerH
//---------------------------------------------------------------------------
#include <Classes.hpp>
#include <Controls.hpp>
#include <StdCtrls.hpp>
#include <Forms.hpp>
#include <ExtCtrls.hpp>
#include <ComCtrls.hpp>
#include <Menus.hpp>
#include <Buttons.hpp>
#include <Graphics.hpp>
#include <vector>
#include <valarray>
#include "formAbout.h"
#include "frameMixer.h"
#include <Buttons.hpp>
#include <Graphics.hpp>

class TMixerFrame;

//------------------------------------------------------------------------------
/// \class TMixerForm for visualization of audio data
//------------------------------------------------------------------------------
class TMixerForm : public TForm
{
   __published:
      TStatusBar *sbStatus;
      TMainMenu *MainMenu;
      TMenuItem *About1;
      TPanel *pnlTracks;
      TPanel *pnlOutputs;
      TPanel *pnlInputs;
      TPanel *pnlBtnTracks;
      TPanel *pnlBtnInputs;
      TPanel *pnlBtnOutputs;
      TImage *ThumbImage;
      TTimer *LevelUpdateTimer;
      void __fastcall About1Click(TObject *Sender);
      void __fastcall pnlTracksClick(TObject *Sender);
      void __fastcall pnlOutputsClick(TObject *Sender);
      void __fastcall pnlInputsClick(TObject *Sender);
      void __fastcall FormCanResize(TObject *Sender, int &NewWidth,
             int &NewHeight, bool &Resize);
      void __fastcall FormShow(TObject *Sender);
      void __fastcall FormKeyDown(TObject *Sender, WORD &Key,
             TShiftState Shift);
      void __fastcall LevelUpdateTimerTimer(TObject *Sender);
   private:
      TMixerFrame*                     m_MixerWithFocus;
      bool                             m_bCanResize;
      bool                             m_bPositionsRead;
      int                              m_nMaxWidth;
      CRITICAL_SECTION                 m_csMixer;
      std::vector<std::vector<TMixerFrame*> >       m_vvMixers;
      std::valarray<float>             m_vafTrackVolumes;
      std::valarray<unsigned int>      m_vanTrackClipCount;
      AnsiString                       m_asIniSuffix;
      void SetLevelsInternal();
      void ResetLevelsInternal();
      void SetTrackSlidersInternal();
      void SetFocusToFirstSlider();
   public:
      __fastcall TMixerForm(TComponent* Owner);
      __fastcall ~TMixerForm();
      void Init();
      void Exit();
      void __fastcall AdjustWidth(bool b = false);
      void SetStatusString(AnsiString str, int nIndex);
      void UpdateTrackSliders(bool bForce = false);
      void UpdateOutputSliders(const std::vector<float> & vfGain, bool bForce = false);
      void UpdateInputSliders(const std::vector<float> & vfGain, bool bForce = false);
      void UpdateLevels(std::valarray<float>& vaf, std::valarray<unsigned int>& van);
      void UpdateClipCounts(std::valarray<unsigned int>& van);
      void UpdateAllButtons(bool bForce = false);
      void UpdateButtons(TChannelType ct, bool bForce = false);
      void ResetLevels();
      void SetSolo(TChannelType ct, unsigned int nIndex);
      void SetMute(TChannelType ct, unsigned int nIndex);
      void SetGain(TChannelType ct, unsigned int nIndex, float f);
      void TabClick(TChannelType ct, int nIndex, bool bInverse);
      void __fastcall ResetClip();
      void UpdateChannelData(TChannelType ct);
};
//------------------------------------------------------------------------------
#endif
