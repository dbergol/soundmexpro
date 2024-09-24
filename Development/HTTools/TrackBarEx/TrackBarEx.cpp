//------------------------------------------------------------------------------
/// \file TrackBarEx.cpp
/// \author Berg
/// \brief Implementation of class TTrackBarEx
///
/// Project SoundMexPro
/// Module  HTTools.bpl (Borland package)
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
#pragma hdrstop

#include "TrackBarEx.h"
#include "HTLevel.h"


#pragma package(smart_init)

#ifdef DEBUG_STATES
AnsiString DrawStageToStr(DWORD dwDrawStage);
AnsiString ItemStateToStr(UINT u);
#endif


//------------------------------------------------------------------------------
/// Borland generated
//------------------------------------------------------------------------------
static inline void ValidCtrCheck(TTrackBarEx *)
{
   new TTrackBarEx(NULL);
}
//------------------------------------------------------------------------------

//------------------------------------------------------------------------------
/// constructor: enable double clicks and creates internal canvas and bitmaps
//------------------------------------------------------------------------------
__fastcall TTrackBarEx::TTrackBarEx(TComponent* Owner)
   : TTrackBar(Owner), FCanvas(NULL), FNarrowChannel(true), FDrawFocusRect(true)
{
   ControlStyle = ControlStyle << csDoubleClicks;
   FCanvas = new TCanvas();
   for (int i = 0; i < 4; i++)
      FBitmaps[i] = new Graphics::TBitmap();

}
//------------------------------------------------------------------------------

//------------------------------------------------------------------------------
/// destructor (cleanup)
//------------------------------------------------------------------------------
__fastcall TTrackBarEx::~TTrackBarEx()
{
   if (FCanvas)
      {
      delete FCanvas;
      FCanvas = NULL;
      }
   for (int i = 0; i < 4; i++)
      {
      if (FBitmaps[i])
         {
         delete FBitmaps[i];
         FBitmaps[i] = NULL;
         }
      }
}
//------------------------------------------------------------------------------

//------------------------------------------------------------------------------
/// overloaded CreateParams, removes TBS_ENABLESELRANGE style if requested
//------------------------------------------------------------------------------
void __fastcall TTrackBarEx::CreateParams(TCreateParams &Params)
{
   TTrackBar::CreateParams(Params);
   if (FNarrowChannel)
      Params.Style = Params.Style & ~TBS_ENABLESELRANGE;
   else
      Params.Style = Params.Style | TBS_ENABLESELRANGE;
}

//------------------------------------------------------------------------------
/// Process double clicks only on thumb!
//------------------------------------------------------------------------------
void __fastcall TTrackBarEx::DblClick()
{

   // retrieve thumb rect
   RECT rc;
   SendMessage(Handle, TBM_GETTHUMBRECT, 0, (LPARAM)&rc);
   // check if mouse position is within this rect
   if (PtInRect(&rc, ScreenToClient(Mouse->CursorPos)))
      TTrackBar::DblClick();
}
//------------------------------------------------------------------------------



//------------------------------------------------------------------------------
/// Main drawing callback. Draws background/thumb images and calls user callbacsk
//------------------------------------------------------------------------------
void __fastcall TTrackBarEx::CustomDraw(Messages::TWMNotify &Message)
{
   bool bDefaultDraw = true;
   Message.Result = CDRF_DODEFAULT;

   LPNMCUSTOMDRAW lpNMCustomDraw = (LPNMCUSTOMDRAW)Message.NMHdr;

   if ((lpNMCustomDraw->dwDrawStage & CDDS_PREPAINT) == 0)
      return;

   // static variable to store last item that was drawn
   static TrackBarExItem tbiLast = tbiBackground;

   // retrieve current item to draw
   TrackBarExItem tbi = GetItem(lpNMCustomDraw->dwItemSpec);

   // check, if background to be painted. We paint _before_ the
   // first item is painted. First item is either:
   // - tbiTics, or
   // - tbiChannel, if last item was not tbiTics, or
   // - tbiThumb, if last item was not tbiChannel
   bool bPaintBackground = false;
   if (tbi == tbiTics)
      bPaintBackground = true;
   else if (tbi == tbiChannel && tbiLast != tbiTics)
      bPaintBackground = true;
   else if (tbi == tbiThumb && tbiLast != tbiChannel)
      bPaintBackground = true;

   // store last item
   tbiLast = tbi;

   // paint thumb-image?
   bool bBitmap = false;
   if (tbi == tbiThumb)
      {
      bBitmap = DrawThumbImg(lpNMCustomDraw->rc);
      }
   // paint background?
   else if (bPaintBackground && (lpNMCustomDraw->uItemState & CDIS_FOCUS) == 0)
      {
      DrawBackgroundImg();


      if (!!FOnCustomDraw)
         {
         StoreCanvasProps(false);
         FOnCustomDraw(this, tbiBackground, TRect(0,0,Width,Height), GetDrawState(lpNMCustomDraw->uItemState), bDefaultDraw);
         StoreCanvasProps(true);
         }
      }


   // call user function
   if (!!FOnCustomDraw)
      {
      StoreCanvasProps(false);
      bDefaultDraw = false;
      FOnCustomDraw(this, tbi, lpNMCustomDraw->rc, GetDrawState(lpNMCustomDraw->uItemState), bDefaultDraw);
      StoreCanvasProps(true);
      }
   if (!bDefaultDraw || bBitmap)
      Message.Result = CDRF_SKIPDEFAULT;
   // finally draw focus rect if necessary
   if (FDrawFocusRect && Focused() && bPaintBackground)
      Canvas->DrawFocusRect(TRect(0,0,Width,Height));

//   if (tbi == tbiThumb)
//      Invalidate();

}
//------------------------------------------------------------------------------

//------------------------------------------------------------------------------
/// Overloaded callback to handle custom-draw-messages
//------------------------------------------------------------------------------
MESSAGE void __fastcall TTrackBarEx::CNNotify(Messages::TWMNotify &Message)
{
   if (Message.NMHdr->code == NM_CUSTOMDRAW)
      {
      LPNMCUSTOMDRAW lpNMCustomDraw = (LPNMCUSTOMDRAW)Message.NMHdr;
      try
         {
         FCanvas->Lock();
         FCanvas->Handle = lpNMCustomDraw->hdc;
         // The first custom draw message sent to the component is CDDS_PREPAINT. At this
         // point tell Windows to also send messages for drawing the individual items. Later messages
         // will then include the CDDS_ITEM flag.
         if ((lpNMCustomDraw->dwDrawStage & CDDS_ITEM) == 0)
            Message.Result = CDRF_NOTIFYITEMDRAW;
         // otherwise call custom draw function
         else
            CustomDraw(Message);
         }
      __finally
         {
         FCanvas->Handle = NULL;
         FCanvas->Unlock();
         }
      }
   else
      TTrackBar::Dispatch(&Message);

}
//------------------------------------------------------------------------------

//------------------------------------------------------------------------------
/// Draws thumb image on canvas depending on availability and status. Returns
/// true if any picture was drawn
//------------------------------------------------------------------------------
bool __fastcall TTrackBarEx::DrawThumbImg(tagRECT& rc)
{
   // if disabled then draw 'disabled' image with fallback to enabled image
   if (!Enabled && !FBitmaps[bmThumbDisabled]->Empty)
      {
      Canvas->StretchDraw(rc, FBitmaps[bmThumbDisabled]);
      return true;
      }
   if (!FBitmaps[bmThumb]->Empty)
      {
      Canvas->StretchDraw(rc, FBitmaps[bmThumb]);
      return true;
      }
   return false;
}
//------------------------------------------------------------------------------

//------------------------------------------------------------------------------
/// Draws background image on canvas depending on availability and status. Returns
/// true if any picture was drawn
//------------------------------------------------------------------------------
bool __fastcall TTrackBarEx::DrawBackgroundImg()
{
   // if disabled then draw 'disabled' image with fallback to enabled image
   if (!Enabled && !FBitmaps[bmBackGroundDisabled]->Empty)
      {
      Canvas->StretchDraw(TRect(0,0,Width,Height), FBitmaps[bmBackGroundDisabled]);
      return true;
      }
   if (!FBitmaps[bmBackGround]->Empty)
      {
      Canvas->StretchDraw(TRect(0,0,Width,Height), FBitmaps[bmBackGround]);
      return true;
      }
   return false;
}
//------------------------------------------------------------------------------


//------------------------------------------------------------------------------
/// returns background and thumb images from index
//------------------------------------------------------------------------------
Graphics::TBitmap* __fastcall TTrackBarEx::GetImage(TBBitmaps tbbm)
{
   return FBitmaps[tbbm];
}
//------------------------------------------------------------------------------

//------------------------------------------------------------------------------
/// sets background and thumb images by index
//------------------------------------------------------------------------------
void __fastcall TTrackBarEx::SetImage(Graphics::TBitmap* AValue, TBBitmaps tbbm)
{
   FBitmaps[tbbm]->Assign(AValue);
   RecreateWnd();
}
//------------------------------------------------------------------------------

//------------------------------------------------------------------------------
/// Get- and Set- functions for background and thumb images 
//------------------------------------------------------------------------------
Graphics::TBitmap* __fastcall TTrackBarEx::GetBackGroundImage()
{
   return GetImage(bmBackGround);
}
void __fastcall TTrackBarEx::SetBackGroundImage(Graphics::TBitmap* AValue)
{
   SetImage(AValue, bmBackGround);
}
Graphics::TBitmap* __fastcall TTrackBarEx::GetBackGroundImageDisabled()
{
   return GetImage(bmBackGroundDisabled);
}
void __fastcall TTrackBarEx::SetBackGroundImageDisabled(Graphics::TBitmap* AValue)
{
   SetImage(AValue, bmBackGroundDisabled);
}
Graphics::TBitmap* __fastcall TTrackBarEx::GetThumbImage()
{
   return GetImage(bmThumb);
}
void __fastcall TTrackBarEx::SetThumbImage(Graphics::TBitmap* AValue)
{
   SetImage(AValue, bmThumb);
}
Graphics::TBitmap* __fastcall TTrackBarEx::GetThumbImageDisabled()
{
   return GetImage(bmThumbDisabled);
}
void __fastcall TTrackBarEx::SetThumbImageDisabled(Graphics::TBitmap* AValue)
{
   SetImage(AValue, bmThumbDisabled);
}
//------------------------------------------------------------------------------

//------------------------------------------------------------------------------
/// Sets FNarrowChannel class member and calls RecreateWnd if necessary
//------------------------------------------------------------------------------
void __fastcall TTrackBarEx::SetNarrowChannel(bool b)
{
   if (b != FNarrowChannel)
      {
      FNarrowChannel = b;
      RecreateWnd();
      }
}
//------------------------------------------------------------------------------

//------------------------------------------------------------------------------
/// Sets FDrawFocusRect class member and calls RecreateWnd if necessary
//------------------------------------------------------------------------------
void __fastcall TTrackBarEx::SetDrawFocusRect(bool b)
{
   if (b != FDrawFocusRect)
      {
      FDrawFocusRect = b;
      RecreateWnd();
      }
}
//------------------------------------------------------------------------------

//------------------------------------------------------------------------------
/// Calls base class and calls RecreateWnd if necessary
//------------------------------------------------------------------------------
void __fastcall TTrackBarEx::SetEnabled(bool AValue)
{
   bool bEnabled = Enabled;
   TTrackBar::SetEnabled(AValue);
   if (AValue != bEnabled)
      RecreateWnd();
}
//------------------------------------------------------------------------------

//------------------------------------------------------------------------------
/// Tool function returning TrackBarExItem that corresponds to dwItemSpec
/// member of NMCUSTOMDRAW struct
//------------------------------------------------------------------------------
TrackBarExItem __fastcall TTrackBarEx::GetItem(DWORD dw)
{
   switch (dw)
      {
      case TBCD_TICS:         return tbiTics;
      case TBCD_THUMB:        return tbiThumb;
      case TBCD_CHANNEL:      return tbiChannel;
      }
   return tbiChannel;
}
//------------------------------------------------------------------------------

//------------------------------------------------------------------------------
/// Tool function returning Comctrls::TCustomDrawState set created from uItemState
/// member of NMCUSTOMDRAW struct
//------------------------------------------------------------------------------
Comctrls::TCustomDrawState __fastcall TTrackBarEx::GetDrawState(unsigned int u)
{
   TCustomDrawState cds = TCustomDrawState();
   if (u & CDIS_CHECKED)
      cds << cdsChecked;
   if (u & CDIS_DEFAULT)
      cds << cdsDefault;
   if (u & CDIS_DISABLED)
      cds << cdsDisabled;
   if (u & CDIS_FOCUS)
      cds << cdsFocused;
   if (u & CDIS_GRAYED)
      cds << cdsGrayed;
   if (u & CDIS_HOT)
      cds << cdsHot;
   if (u & CDIS_INDETERMINATE)
      cds << cdsIndeterminate;
   if (u & CDIS_MARKED)
      cds << cdsMarked;
   if (u & CDIS_SELECTED)
      cds << cdsSelected;
   return cds;
}
//------------------------------------------------------------------------------

//------------------------------------------------------------------------------
/// stores/restores pen and brush properties of own canvas
//------------------------------------------------------------------------------
void __fastcall TTrackBarEx::StoreCanvasProps(bool bRestore)
{
   if (bRestore)
      {
      Canvas->Pen->Color   = FPenColor;
      Canvas->Pen->Mode    = FPenMode;
      Canvas->Pen->Style   = FPenStyle;
      Canvas->Pen->Width   = FPenWidth;
      Canvas->Brush->Color = FBrushColor;
      Canvas->Brush->Style = FBrushStyle;
      }
   else
      {
      FPenColor   = Canvas->Pen->Color;
      FPenMode    = Canvas->Pen->Mode;
      FPenStyle   = Canvas->Pen->Style;
      FPenWidth   = Canvas->Pen->Width;
      FBrushColor = Canvas->Brush->Color;
      FBrushStyle = Canvas->Brush->Style;
      }
}
//------------------------------------------------------------------------------


#ifdef DEBUG_STATES
//------------------------------------------------------------------------------
/// Debug function
//------------------------------------------------------------------------------
AnsiString DrawStageToStr(DWORD dwDrawStage)
{
   AnsiString str;
   switch (dwDrawStage)
      {
      case CDDS_PREPAINT:  str = "CDDS_PREPAINT";break;
      case CDDS_POSTPAINT:  str = "CDDS_POSTPAINT";break;
      case CDDS_PREERASE:  str = "CDDS_PREERASE";break;
      case CDDS_POSTERASE:  str = "CDDS_POSTERASE";break;
      case CDDS_ITEMPREPAINT:  str = "CDDS_ITEMPREPAINT";break;
      case CDDS_ITEMPOSTPAINT:  str = "CDDS_ITEMPOSTPAINT";break;
      case CDDS_ITEMPREERASE:  str = "CDDS_ITEMPREERASE";break;
      case CDDS_ITEMPOSTERASE:  str = "CDDS_ITEMPOSTERASE";break;
      default: str = "UNKNOWN DRAWSTAGE";
      }

   return str;
}
//------------------------------------------------------------------------------

//------------------------------------------------------------------------------
/// Debug function
//------------------------------------------------------------------------------
AnsiString ItemStateToStr(UINT u)
{
   AnsiString str;
   if ((u & CDIS_CHECKED) != 0)
      str += "CDIS_CHECKED, ";
   if ((u & CDIS_DEFAULT) != 0)
      str += "CDIS_DEFAULT, ";
   if ((u & CDIS_DISABLED) != 0)
      str += "CDIS_DISABLED, ";
   if ((u & CDIS_FOCUS) != 0)
      str += "CDIS_FOCUS, ";
   if ((u & CDIS_GRAYED) != 0)
      str += "CDIS_GRAYED, ";
   if ((u & CDIS_HOT) != 0)
      str += "CDIS_HOT, ";
   if ((u & CDIS_INDETERMINATE) != 0)
      str += "CDIS_INDETERMINATE, ";
   if ((u & CDIS_MARKED) != 0)
      str += "CDIS_MARKED, ";
   if ((u & CDIS_SELECTED) != 0)
      str += "CDIS_SELECTED, ";

   if (str.IsEmpty())
      str = "empty ItemState";
   return str;
}
#endif
//------------------------------------------------------------------------------

//------------------------------------------------------------------------------
/// component regitration function
//------------------------------------------------------------------------------
namespace Trackbarex
{
   void __fastcall PACKAGE Register()
   {
      TComponentClass classes[1] = {__classid(TTrackBarEx)};
      RegisterComponents("HTTools", classes, 0);
      TComponentClass classes2[1] = {__classid(THTLevel)};
      RegisterComponents("HTTools", classes2, 0);

   }
}
//------------------------------------------------------------------------------

