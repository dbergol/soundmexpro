//------------------------------------------------------------------------------
/// \file VSTParameterFrame.h
/// \author Berg
/// \brief Implementation of class TframeVSTParam. TFrame for showing and
/// changing a VST parameter. Used by TVSTPluginEditor
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
#ifndef VSTParameterFrameH
#define VSTParameterFrameH
//------------------------------------------------------------------------------
#include <Classes.hpp>
#include <Controls.hpp>
#include <StdCtrls.hpp>
#include <Forms.hpp>
#include <ComCtrls.hpp>

//------------------------------------------------------------------------------
class AEffect;
//------------------------------------------------------------------------------

//------------------------------------------------------------------------------
/// \class TframeVSTParam. TFrame for showing and changing a VST parameter.
//------------------------------------------------------------------------------
class TframeVSTParam : public TFrame
{
   __published:
      TStatusBar *sb;
      TProgressBar *pb;
   void __fastcall pbMouseDown(TObject *Sender, TMouseButton Button,
          TShiftState Shift, int X, int Y);
   void __fastcall pbMouseMove(TObject *Sender, TShiftState Shift, int X,
          int Y);
   private:
      AEffect* m_pEffect;
      int      m_nXValue;
      int      m_nOldPosition;
   public:
      __fastcall TframeVSTParam(TComponent* Owner, AEffect *peff, int nIndex);
      void UpdateValue(bool bValueFromEffect);
};
//------------------------------------------------------------------------------
#endif
