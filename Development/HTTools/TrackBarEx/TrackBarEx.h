//------------------------------------------------------------------------------
/// \file TrackBarEx.h
/// \author Berg
/// \brief Implementation of class TTrackBarEx
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
#ifndef TrackBarExH
#define TrackBarExH
//---------------------------------------------------------------------------
#include <SysUtils.hpp>
#include <Controls.hpp>
#include <Classes.hpp>
#include <Forms.hpp>
#include <ComCtrls.hpp>
//------------------------------------------------------------------------------


namespace TrackBarEx
{
#pragma option push -b-
enum TrackBarExItem {tbiTics, tbiThumb, tbiChannel, tbiBackground};
enum TBBitmaps  {bmBackGround = 0, bmBackGroundDisabled, bmThumb, bmThumbDisabled};
#pragma option pop

class TTrackBarEx;
typedef void __fastcall (__closure *TTBEXCustomDrawEvent)(TTrackBarEx* Sender, TrackBarExItem Item, const Types::TRect &ARect, Comctrls::TCustomDrawState State, bool &DefaultDraw);

//------------------------------------------------------------------------------
/// \class TTrackBarEx. Subclassed TTrackBar
//------------------------------------------------------------------------------
class PACKAGE TTrackBarEx : public TTrackBar
{
   private:
      int nThumbMiddle;
      Graphics::TBitmap*   FBitmaps[4];   /// Bitmaps for enabled/disabled background and thumb
      Graphics::TCanvas*   FCanvas;
      bool                 FNarrowChannel;
      bool                 FDrawFocusRect;
      TTBEXCustomDrawEvent FOnCustomDraw;

      void __fastcall CustomDraw(Messages::TWMNotify &Message);
      bool __fastcall DrawThumbImg(tagRECT& rc);
      bool __fastcall DrawBackgroundImg();
      TrackBarExItem __fastcall GetItem(DWORD dw);
      Comctrls::TCustomDrawState __fastcall GetDrawState(unsigned int u);
      void __fastcall SetNarrowChannel(bool b);
      void __fastcall SetDrawFocusRect(bool b);
      Graphics::TBitmap* __fastcall GetImage(TBBitmaps tbbm);
      void __fastcall SetImage(Graphics::TBitmap* AValue, TBBitmaps tbbm);
      Graphics::TBitmap* __fastcall GetBackGroundImage();
      void __fastcall SetBackGroundImage(Graphics::TBitmap* AValue);
      Graphics::TBitmap* __fastcall GetBackGroundImageDisabled();
      void __fastcall SetBackGroundImageDisabled(Graphics::TBitmap* AValue);
      Graphics::TBitmap* __fastcall GetThumbImage();
      void __fastcall SetThumbImage(Graphics::TBitmap* AValue);
      Graphics::TBitmap* __fastcall GetThumbImageDisabled();
      void __fastcall SetThumbImageDisabled(Graphics::TBitmap* AValue);
      void __fastcall SetEnabled(bool AValue);
   private:
      TColor      FPenColor;
      TPenMode    FPenMode;
      TPenStyle   FPenStyle;
      int         FPenWidth;
      TColor      FBrushColor;
      TBrushStyle FBrushStyle;
      void __fastcall StoreCanvasProps(bool bRestore);
   protected:
      MESSAGE void __fastcall CNNotify(Messages::TWMNotify &Message);
      DYNAMIC  void __fastcall DblClick();
   public:
      __fastcall TTrackBarEx(TComponent* Owner);
      __fastcall ~TTrackBarEx();
      void __fastcall CreateParams(TCreateParams &Params);
      __property Graphics::TCanvas* Canvas = {read=FCanvas};
   __published:
      __property OnClick;
      __property OnDblClick;
      __property TTBEXCustomDrawEvent OnCustomDraw = {read=FOnCustomDraw, write=FOnCustomDraw};
      __property bool NarrowChannel = {read=FNarrowChannel, write=SetNarrowChannel, default=true};
      __property bool ShowFocusRect = {read=FDrawFocusRect, write=SetDrawFocusRect, default=true};
      __property Graphics::TBitmap* BackGroundImage          =  {read=GetBackGroundImage, write=SetBackGroundImage};
      __property Graphics::TBitmap* BackGroundImageDisabled  =  {read=GetBackGroundImageDisabled, write=SetBackGroundImageDisabled};
      __property Graphics::TBitmap* ThumbImage          =  {read=GetThumbImage, write=SetThumbImage};
      __property Graphics::TBitmap* ThumbImageDisabled  =  {read=GetThumbImageDisabled, write=SetThumbImageDisabled};
   public:
   #pragma warn -8027
   #pragma warn -8118
      BEGIN_MESSAGE_MAP
         VCL_MESSAGE_HANDLER(CN_NOTIFY, Messages::TWMNotify, CNNotify)
      END_MESSAGE_MAP(TTrackBar)
   #pragma warn +8027
   #pragma warn +8118

};
}	/* namespace TrackBarEx */
#pragma clang diagnostic push
#pragma clang diagnostic ignored "-Wheader-hygiene"
using namespace TrackBarEx;
#pragma clang diagnostic pop

//---------------------------------------------------------------------------
#endif
