//------------------------------------------------------------------------------
/// \file HtLevel.cpp
/// \author Berg
/// \brief Implementation of classes THTDrawBox THTLevel for showing a
/// 'LED-like LevelMeter'
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

#include "HTLevel.h"
#pragma package(smart_init)



//------------------------------------------------------------------------------
/// Borland generated
//------------------------------------------------------------------------------
static inline void ValidCtrCheck(THTLevel *)
{
   new THTLevel((TComponent*)NULL);
}
//------------------------------------------------------------------------------

//------------------------------------------------------------------------------
/// constructor, initializes members
//------------------------------------------------------------------------------
__fastcall THTDrawBox::THTDrawBox(TComponent* Owner)
   : TCustomControl(Owner), FBitmap(NULL)
{
   FBitmap  = new Graphics::TBitmap();
   Width    = 50;
   Height   = 50;
   BackGroundColor = clBlack;
   Resize();
   ClearBmp();
   m_nWidth = Width;
   m_nHeight = Height;

}
//------------------------------------------------------------------------------

//------------------------------------------------------------------------------
/// destructor (cleanup)
//------------------------------------------------------------------------------
__fastcall THTDrawBox::~THTDrawBox()
{
   if (FBitmap)
      {
      delete FBitmap;
      FBitmap = NULL;
      }
}
//------------------------------------------------------------------------------

//------------------------------------------------------------------------------
/// overloaded base class method. Calls base class and calls Resize()
//------------------------------------------------------------------------------
void __fastcall THTDrawBox::SetBounds(int ALeft, int ATop, int AWidth, int AHeight)
{
   TCustomControl::SetBounds(ALeft, ATop, AWidth, AHeight);
   Resize();
}
//------------------------------------------------------------------------------

//------------------------------------------------------------------------------
/// redrawing on Resize
//------------------------------------------------------------------------------
void __fastcall THTDrawBox::Resize(void)
{
   if (!FBitmap)
      return;
   if (m_nHeight != Height || m_nWidth != Width)
      {
      FBitmap->Width    = Width;
      FBitmap->Height   = Height;
      m_nWidth = Width;
      m_nHeight = Height;
      ChildResize();
      Redraw();
      Invalidate();
      }
}
//------------------------------------------------------------------------------

//------------------------------------------------------------------------------
/// overloaded WM_SIZE message handler, calls Resize and nvalidates control
//------------------------------------------------------------------------------
#pragma argsused
void __fastcall THTDrawBox::WMSize(Messages::TWMSize &Msg)
{
   //TCustomControl::WMSize(Msg);
   Resize();
   Invalidate();
}
//------------------------------------------------------------------------------

//------------------------------------------------------------------------------
/// virtual function called by Resize. Empty here, to be overloaded by inherited
/// classes
//------------------------------------------------------------------------------
void __fastcall THTDrawBox::ChildResize(void)
{
}
//------------------------------------------------------------------------------

//------------------------------------------------------------------------------
/// clears bitmap by drawing rectangle with background color on top
//------------------------------------------------------------------------------
void __fastcall THTDrawBox::ClearBmp(void)
{
  FBitmap->Canvas->Brush->Color = FBackGroundColor;
  FBitmap->Canvas->Brush->Style = bsSolid;
  FBitmap->Canvas->FillRect(Rect(0, 0, m_nWidth, m_nHeight));
}
//------------------------------------------------------------------------------

//------------------------------------------------------------------------------
/// overloaded base class method for painting control
//------------------------------------------------------------------------------
void __fastcall THTDrawBox::Paint(void)
{
   if (m_nHeight != Height || m_nWidth != Width)
      Resize();
   else
      {
       Canvas->CopyMode = cmSrcCopy;
       Canvas->Draw(0, 0, FBitmap);
      }
}
//------------------------------------------------------------------------------

//------------------------------------------------------------------------------
/// overloaded base class method for redrawing control
//------------------------------------------------------------------------------
void __fastcall THTDrawBox::Redraw(void)
{
   if (!FBitmap || !Parent)
      return;
   ClearBmp();
}
//------------------------------------------------------------------------------

//------------------------------------------------------------------------------
/// Setter function for background color
//------------------------------------------------------------------------------
void __fastcall THTDrawBox::SetBackgroundColor(Graphics::TColor val)
{
  FBackGroundColor = val;
  Redraw();
  Invalidate();
}
//------------------------------------------------------------------------------

//------------------------------------------------------------------------------
/// overloaded message handler for erasing background
//------------------------------------------------------------------------------
void __fastcall THTDrawBox::WMEraseBkgnd(Messages::TWMEraseBkgnd &M)
{
   M.Result = LRESULT(FALSE);
}
//------------------------------------------------------------------------------


//------------------------------------------------------------------------------
///                         THTLevel
//------------------------------------------------------------------------------

//------------------------------------------------------------------------------
/// construcitr for LED-like LevelMeter, inherits from THTDrawBox
//------------------------------------------------------------------------------
__fastcall THTLevel::THTLevel(TComponent* Owner)
 : THTDrawBox(Owner)
{
   FPenWidth = 1;
   FMin = -90;
   FMax = 6;
   FMid = -0;
   FValue = FMin;
   FType = ltVertical;
}
//------------------------------------------------------------------------------

//------------------------------------------------------------------------------
/// destructor (empty)
//------------------------------------------------------------------------------
__fastcall THTLevel::~THTLevel()
{
}
//------------------------------------------------------------------------------

//------------------------------------------------------------------------------
/// Setter function for pen width
//------------------------------------------------------------------------------
void __fastcall THTLevel::SetPenWidth(int nValue)
{
   if (nValue != FPenWidth)
      {
      FPenWidth = nValue;
      Redraw();
      }
}
//------------------------------------------------------------------------------

//------------------------------------------------------------------------------
/// Setter function for type (horizontal/vertical)
//------------------------------------------------------------------------------
void __fastcall THTLevel::SetType(THTLType lt)
{
   if (FType != lt)
      {
      FType = lt;
      Redraw();
      }
}
//------------------------------------------------------------------------------

//------------------------------------------------------------------------------
/// Setter function for minimum value
//------------------------------------------------------------------------------
void __fastcall THTLevel::SetMin(int nValue)
{
   if (nValue != FMin && FMin < FMax && FMin < FMid)
      {
      FMin = nValue;
      Redraw();
      Invalidate();
      }
}
//------------------------------------------------------------------------------

//------------------------------------------------------------------------------
/// Setter function for maximum value
//------------------------------------------------------------------------------
void __fastcall THTLevel::SetMax(int nValue)
{
   if (nValue != FMax && FMin < FMax && FMax > FMid)
      {
      FMax = nValue;
      Redraw();
      Invalidate();
      }
}
//------------------------------------------------------------------------------

//------------------------------------------------------------------------------
/// Setter function for intermediate value
//------------------------------------------------------------------------------
void __fastcall THTLevel::SetMid(int nValue)
{
   if (nValue != FMid && FMid < FMax && FMid > FMin)
      {
      FMid = nValue;
      Redraw();
      Invalidate();
      }

}
//------------------------------------------------------------------------------

//------------------------------------------------------------------------------
/// Setter function for current value
//------------------------------------------------------------------------------
void __fastcall THTLevel::SetValue(int nNewValue)
{
   if (nNewValue < FMin)
      nNewValue = FMin;
   else if (nNewValue > FMax)
      nNewValue = FMax;

   if (FValue != nNewValue)
      {
      int nTotal = (FType == ltVertical) ? Height :  Width;
      FBitmap->Canvas->Pen->Width = FPenWidth;
      int nNumBars = nTotal / FPenWidth / 2;


      float f = 1.0 - (float)(FMid - FMin) / (float)(FMax - FMin);
      int nMid =  (int)((float)nTotal * f);

      f = 1.0 - (float)(FValue - FMin) / (float)(FMax - FMin);
      int nOld =  (int)((float)nTotal * f);
      f = 1.0 - (float)(nNewValue - FMin) / (float)(FMax - FMin);
      int nNew = (int)((float)nTotal * f);

      int nPos;
      if (nNewValue > FValue)
         {
         if (FType == ltVertical)
            {
            for (int i = 0; i <= nNumBars; i++)
               {
               nPos = 2*FPenWidth*i;
               if (nPos > nOld)
                  break;
               if (nPos < nNew)
                  continue;
               FBitmap->Canvas->Pen->Color = (nPos < nMid) ? clRed : clLime;
               FBitmap->Canvas->MoveTo(0, nPos);
               FBitmap->Canvas->LineTo(Width, nPos);
               }
            }
         else
            {
            for (int i = 0; i <= nNumBars; i++)
               {
               nPos = 2*FPenWidth*i;
               if (nPos > nOld)
                  break;
               if (nPos < nNew)
                  continue;
               FBitmap->Canvas->Pen->Color = (nPos < nMid) ? clRed : clLime;
               FBitmap->Canvas->MoveTo(Width-nPos, 0);
               FBitmap->Canvas->LineTo(Width-nPos, Height);
               }
            }
         }
      else  // new value lower
         {
         if (FType == ltVertical)
            {
            for (int i = 0; i <= nNumBars; i++)
               {
               nPos = 2*FPenWidth*i;
               if (nPos > nNew)
                  break;
               if (nPos < nOld)
                  continue;
               FBitmap->Canvas->Pen->Color = (nPos < nMid) ? clMaroon : clGreen;
               FBitmap->Canvas->MoveTo(0, nPos);
               FBitmap->Canvas->LineTo(Width, nPos);
               }
            }
         else
            {
            for (int i = 0; i <= nNumBars; i++)
               {
               nPos = 2*FPenWidth*i;
               if (nPos > nNew)
                  break;
               if (nPos < nOld)
                  continue;
               FBitmap->Canvas->Pen->Color = (nPos < nMid) ? clMaroon : clGreen;
               FBitmap->Canvas->MoveTo(Width-nPos, 0);
               FBitmap->Canvas->LineTo(Width-nPos, Height);
               }
            }
         }
      FValue = nNewValue;
      Paint();
      }
}
//------------------------------------------------------------------------------

//------------------------------------------------------------------------------
/// Internal function for redrawing control
//------------------------------------------------------------------------------
void __fastcall THTLevel::Redraw(void)
{
   if (!FBitmap || !FPenWidth)
      return;

   ClearBmp();

   FBitmap->Canvas->Pen->Width = FPenWidth;

   int nTotal = (FType == ltVertical) ? Height : Width;

   int nNumBars = nTotal / FPenWidth / 2;
   float f = 1.0 - (float)(FValue - FMin) / (float)(FMax - FMin);
   int   nValue = (int)((float)nTotal * f);

   f = 1.0 - (float)(FMid - FMin) / (float)(FMax - FMin);
   int nMid = (int)((float)nTotal * f);
   int nPos;
   if (FType == ltVertical)
      {
      for (int i = 0; i <= nNumBars; i++)
         {
         nPos = 2*FPenWidth*i;
         if (nPos < nValue)
            FBitmap->Canvas->Pen->Color = (nPos < nMid) ? clMaroon : clGreen;
         else
            FBitmap->Canvas->Pen->Color = (nPos < nMid) ? clRed : clLime;
         FBitmap->Canvas->MoveTo(0, nPos);
         FBitmap->Canvas->LineTo(Width, nPos);
         }
      }
   else
      {
      for (int i = 0; i <= nNumBars; i++)
         {
         nPos = 2*FPenWidth*i;
         if (nPos < nValue)
            FBitmap->Canvas->Pen->Color = (nPos < nMid) ? clMaroon : clGreen;
         else
            FBitmap->Canvas->Pen->Color = (nPos < nMid) ? clRed : clLime;
            FBitmap->Canvas->MoveTo(Width-nPos, 0);
            FBitmap->Canvas->LineTo(Width-nPos, Height);
         }
      }
}
//------------------------------------------------------------------------------

//------------------------------------------------------------------------------
/// overloaded base class method, calls Redraw()
//------------------------------------------------------------------------------
void __fastcall THTLevel::ChildResize(void)
{
   Redraw();
}
//------------------------------------------------------------------------------

//------------------------------------------------------------------------------
/// rest function stting current value to minimum value
//------------------------------------------------------------------------------
void __fastcall THTLevel::Reset()
{
   SetValue(FMin);
}
//------------------------------------------------------------------------------


