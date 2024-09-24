//------------------------------------------------------------------------------
/// \file frameTrack.h
/// \author Berg
/// \brief Interface of class TTrackFrame (TFrame) for visualization of
/// audio data blocks in a in track. Used by TTracksFrame
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
#ifndef frameTrackInfoH
#define frameTrackInfoH
//------------------------------------------------------------------------------
#include <Classes.hpp>
#include <Controls.hpp>
#include <StdCtrls.hpp>
#include <Forms.hpp>
#include <ExtCtrls.hpp>
#include "HTLevel.h"
#include "TrackBarEx.h"

//------------------------------------------------------------------------------
/// TFrame for showing information on one track, used by TTracksForm
//------------------------------------------------------------------------------
class TTrackInfoFrame : public TFrame
{
   __published:
      TPanel *pnlTrack;
      TLabel *lbOut;
      TLabel *lbTrackNum;
      TLabel *lbIn;
      TPanel *pnlLevelMeter;
      THTLevel *LevelMeter;
      TPaintBox *pb;
      void __fastcall pbPaint(TObject *Sender);
   private:
   public:
      __fastcall TTrackInfoFrame(TComponent* Owner, int nIndex);
      void UpdateData();
      void UpdateLevel(float f);
      void ResetLevel();
};
//------------------------------------------------------------------------------
#endif
