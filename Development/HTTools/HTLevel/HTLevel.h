//------------------------------------------------------------------------------
/// \file HtLevel.h
/// \author Berg
/// \brief Implementation of classes THTDrawBox THTLevel for showing a
/// 'LED-like LevelMeter'
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
#ifndef HTLevelH
#define HTLevelH
//---------------------------------------------------------------------------
#include <SysUtils.hpp>
#include <Controls.hpp>
#include <Classes.hpp>
#include <Forms.hpp>
#include <ComCtrls.hpp>
//------------------------------------------------------------------------------

namespace HTTools
{
//------------------------------------------------------------------------------
/// \class THTDrawBox.
//------------------------------------------------------------------------------
class PACKAGE THTDrawBox : public TCustomControl
{
   typedef Controls::TControl inherited;
   private:
      int   m_nHeight;
      int   m_nWidth;
      Classes::TNotifyEvent FRedraw;
      Graphics::TColor FBackGroundColor;
      void __fastcall SetBackgroundColor(Graphics::TColor val);
      HIDESBASE void __fastcall Resize(void);
   protected:
      Graphics::TBitmap*   FBitmap;
      virtual void __fastcall Paint(void);
      HIDESBASE MESSAGE void __fastcall WMEraseBkgnd(Messages::TWMEraseBkgnd &M);
      HIDESBASE MESSAGE void __fastcall WMSize(Messages::TWMSize &Msg);
      virtual void __fastcall SetBounds(int ALeft, int ATop, int AWidth, int AHeight);
   public:
      __fastcall THTDrawBox(TComponent* Owner);
      #pragma warn -8118
      inline __fastcall THTDrawBox(HWND ParentWindow) : Controls::TCustomControl(ParentWindow) { }
      #pragma warn +8118
      __fastcall ~THTDrawBox();
      virtual void __fastcall Redraw(void);
      virtual void __fastcall ChildResize(void);
      void __fastcall ClearBmp(void);
   __published:
      __property Graphics::TColor BackGroundColor = {read=FBackGroundColor, write=SetBackgroundColor, default=12632256};
      __property Align;// = {read=FAlign, write=SetAlign, default=0};

};
//------------------------------------------------------------------------------
/// Type enum for THtLevel
//------------------------------------------------------------------------------
enum THTLType {
   ltHorizontal,  ///< horizontal LevelMeter
   ltVertical     ///< vertical LevelMeter
};
//------------------------------------------------------------------------------
//------------------------------------------------------------------------------
/// \class THTLevel. inherits from THTDrawBox.
//------------------------------------------------------------------------------
class PACKAGE THTLevel : public THTDrawBox
{
   typedef Controls::TCustomControl inherited;
   private:
      int   FValue;
      int   FPenWidth;
      int   FMin;
      int   FMax;
      int   FMid;
      THTLType FType;
      void __fastcall   SetPenWidth(int nValue);
      void __fastcall   SetValue(int nValue);
      void __fastcall   SetMin(int nValue);
      void __fastcall   SetMax(int nValue);
      void __fastcall   SetMid(int nValue);
      void __fastcall   SetType(THTLType lt);
   public:
      __fastcall THTLevel(TComponent* Owner);
      __fastcall ~THTLevel();
      void __fastcall Reset();
      #pragma warn -8118
      inline __fastcall THTLevel(HWND ParentWindow) : HTTools::THTDrawBox(ParentWindow) { }
      #pragma warn +8118
      virtual void __fastcall Redraw(void);
      virtual void __fastcall ChildResize(void);
   __published:
      __property int Min = {read=FMin, write=SetMin, default=-90};
      __property int Max = {read=FMax, write=SetMax, default=6};
      __property int Mid = {read=FMid, write=SetMid, default=0};
      __property int PenWidth = {read=FPenWidth, write=SetPenWidth, default=1};
      __property int Value = {read=FValue, write=SetValue, default=0};
      __property THTLType Type = {read=FType, write=SetType, default=ltVertical};
};

}	/* namespace HTTools */
#pragma clang diagnostic push
#pragma clang diagnostic ignored "-Wheader-hygiene"
using namespace HTTools;
#pragma clang diagnostic pop

//---------------------------------------------------------------------------
#endif

