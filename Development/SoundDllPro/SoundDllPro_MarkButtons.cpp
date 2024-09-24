//------------------------------------------------------------------------------
/// \file SoundDllPro_MarkButtons.cpp
/// \author Berg
/// \brief Implementation of classes ButtonProperties and SDPMarkButtons for
/// synchronized button marking
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
#pragma hdrstop
#include "SoundDllPro_MarkButtons.h"
//------------------------------------------------------------------------------

#define MAX_BTN   8

//-------------------------------------------------------------------------
// Callbackfunction used in AddButton
//-------------------------------------------------------------------------
BOOL CALLBACK EnumWindowsProc(HWND hwnd, LPARAM lParam);
//---------------------------------------------------------------------------


//------------------------------------------------------------------------------
/// constructor. Initialises members
//------------------------------------------------------------------------------
ButtonProperties::ButtonProperties(HWND hWindowHandle, RECT &rc, int64_t nStart, int64_t nStop, bool bHandleIsButtonHandle)
   :  m_hWindowHandle(hWindowHandle),
      m_rc(rc),
      m_nMarkStart(nStart),
      m_nMarkStop(nStop),
      m_bIsMarked(false),
      m_bDone(false),
      m_bHandleIsButtonHandle(bHandleIsButtonHandle)
{
}
//------------------------------------------------------------------------------

FindStruct SDPMarkButtons::sm_fs;

//------------------------------------------------------------------------------
/// constructor.
//------------------------------------------------------------------------------
SDPMarkButtons::SDPMarkButtons()
{
   InitializeCriticalSection(&m_cs);
}
//------------------------------------------------------------------------------

//------------------------------------------------------------------------------
/// destructor.
//------------------------------------------------------------------------------
SDPMarkButtons::~SDPMarkButtons()
{
   DeleteCriticalSection(&m_cs);
}
//------------------------------------------------------------------------------

//------------------------------------------------------------------------------
/// adds a button to internal button vector
//------------------------------------------------------------------------------
void SDPMarkButtons::AddButton(HWND hWindowHandle, RECT &rc, int64_t nStart, int64_t nLength, bool bHandleIsButtonHandle)
{
   if (m_vbp.size() >= MAX_BTN)
      throw Exception("Maximum of " + IntToStr(MAX_BTN) + " Buttons already set!");
   if (!hWindowHandle)
      throw Exception("invalid window handle passed to button marking");
   if (nStart < 0 || nLength <= 0)
      throw Exception("start and length for button marking must be >= 0");

   EnterCriticalSection(&m_cs);
   try
      {
      m_vbp.push_back(ButtonProperties(hWindowHandle, rc, nStart, nStart + nLength, bHandleIsButtonHandle));
      }
   __finally
      {
      LeaveCriticalSection(&m_cs);
      }
}
//------------------------------------------------------------------------------

//------------------------------------------------------------------------------
/// Determines button handle by position info and captiopn and adds a it
/// to internal button vector
/// NOTE: rc contains 'MATLAB'-like values, i.e.
/// - left:     'real' left position
/// - top:      counted from bottom
/// - right:    contains width
/// - bottom:   contains height
//------------------------------------------------------------------------------
void SDPMarkButtons::AddButton(RECT &rc, AnsiString strCaption, int64_t nStart, int64_t nLength)
{
   // setup static struct for EnumWindows call below!
   sm_fs.rc              = rc;
   sm_fs.iWindowCount    = 0;
   sm_fs.handle          = 0;
   sm_fs.sWindowCaption  = strCaption;
   sm_fs.bIsButtonHandle = false;
   // search it
   EnumWindows((WNDENUMPROC)EnumWindowsProc, 0);

   // give an error if we have not found the button
   if (!sm_fs.handle)
      throw Exception("button cannot be located");

   //write handle, position...
   AddButton(sm_fs.handle, sm_fs.rc, nStart, nLength, sm_fs.bIsButtonHandle);
}
//------------------------------------------------------------------------------

//------------------------------------------------------------------------------
/// iterates to stored buttons and marks/unmarks them depending on stored properties
/// and passed value
//------------------------------------------------------------------------------
void SDPMarkButtons::DoButtonMarking(int64_t nPosition)
{
   for (unsigned n = 0; n < m_vbp.size(); n++)
      {
      if (!m_vbp[n].m_hWindowHandle)
         continue;
      if (nPosition >= m_vbp[n].m_nMarkStop)
         MarkButton(n, false);
      else if (nPosition >= m_vbp[n].m_nMarkStart)
         MarkButton(n, true);
      }
   RemoveDoneButtons();
}
//------------------------------------------------------------------------------

//------------------------------------------------------------------------------
/// resets all buttons (unmarks them) and clears button vector afterwards 
//------------------------------------------------------------------------------
void SDPMarkButtons::ResetButtons()
{
   for (unsigned n = 0; n < m_vbp.size(); n++)
      MarkButton(n, false);
   EnterCriticalSection(&m_cs);
   try
      {
      m_vbp.clear();
      }
   __finally
      {
      LeaveCriticalSection(&m_cs);
      }
}
//------------------------------------------------------------------------------

//------------------------------------------------------------------------------
/// removes all buttons with window handle NULL
//------------------------------------------------------------------------------
void SDPMarkButtons::RemoveDoneButtons(void)
{
   EnterCriticalSection(&m_cs);
   try
      {
      unsigned nSize = (unsigned int)m_vbp.size();

      for (unsigned n = nSize; n > 0; n--)
         {
         if (m_vbp[n-1].m_bDone)
            m_vbp.erase(m_vbp.begin()+((int)n-1));
         }
}
   __finally
      {
      LeaveCriticalSection(&m_cs);
      }
}
//------------------------------------------------------------------------------


//------------------------------------------------------------------------------
/// marks/unmarks a button by calling InvertRect. If unmarked, handle is cleared
/// afterwards
//------------------------------------------------------------------------------
void SDPMarkButtons::MarkButton(unsigned int nIndex, bool bEnable)
{
   if (nIndex > m_vbp.size()-1)
      return;
   if (m_vbp[nIndex].m_bDone || bEnable == m_vbp[nIndex].m_bIsMarked)
      return;

   HDC hdc = GetDC(m_vbp[nIndex].m_hWindowHandle);
   if (!hdc)
      return;
   // if handle is a real window handle, then retrieve current position
   if (m_vbp[nIndex].m_bHandleIsButtonHandle)
      {
      RECT rc;
      ::GetClientRect(m_vbp[nIndex].m_hWindowHandle, &rc);
      InvertRect(hdc, &rc);
      }
   else
      {
      // otherwise use stored position
      InvertRect(hdc, &m_vbp[nIndex].m_rc);
      }
   ReleaseDC(m_vbp[nIndex].m_hWindowHandle, hdc);
   m_vbp[nIndex].m_bIsMarked = bEnable;
   if (!bEnable)
      m_vbp[nIndex].m_bDone = true;
}
//------------------------------------------------------------------------------

//---------------------------------------------------------------------------
/// Callbackfunction used in AddButton
//---------------------------------------------------------------------------
#pragma argsused
BOOL CALLBACK EnumWindowsProc(HWND hwnd,LPARAM lParam)
{
   char WndTitle[255];

   ZeroMemory(&WndTitle, sizeof(WndTitle));
   GetWindowText( hwnd, WndTitle, sizeof(WndTitle) -1);

   AnsiString as =  WndTitle;

   if ( !!strstr(as.c_str(), SDPMarkButtons::sm_fs.sWindowCaption.c_str()))
      {
      SDPMarkButtons::sm_fs.iWindowCount++;
      if (SDPMarkButtons::sm_fs.iWindowCount> 1)
         throw Exception("More than one matching window with caption of buttons parent found!");

      // get parent rect
      RECT rClient;
      GetClientRect(hwnd, &rClient);

      // now convert MATLAB values to real boundsrect values
      /// - left:     'real' left position
      /// - top:      counted from bottom
      /// - right:    contains width
      /// - bottom:   contains height
      SDPMarkButtons::sm_fs.rc.right += SDPMarkButtons::sm_fs.rc.left;
      SDPMarkButtons::sm_fs.rc.top = rClient.bottom - rClient.top - SDPMarkButtons::sm_fs.rc.top - SDPMarkButtons::sm_fs.rc.bottom;
      SDPMarkButtons::sm_fs.rc.bottom += SDPMarkButtons::sm_fs.rc.top;


      // now move to a point WITHIN the button
      POINT p;
      p.x = SDPMarkButtons::sm_fs.rc.left + 1;
      p.y = SDPMarkButtons::sm_fs.rc.top  + 1;

      // this will return me a 'magic' matlab window which is a frame with size
      // of the client area...
      HWND hFrame = ChildWindowFromPointEx(  hwnd,
                                             p,
                                             CWP_SKIPINVISIBLE | CWP_SKIPTRANSPARENT
                                             );

      if (hFrame)
         {
         // the button itself is a child of this frame!
         HWND hButton = ChildWindowFromPointEx( hFrame,
                                                p,
                                                CWP_SKIPINVISIBLE | CWP_SKIPTRANSPARENT
                                                );
         if (hButton)
            {
            SDPMarkButtons::sm_fs.handle = hButton;
            // check if handles are identical: from MATLAB 2013, the buttons are no real
            // Windows with an own handle any more (*sigh*)
            SDPMarkButtons::sm_fs.bIsButtonHandle = (hFrame != hButton);
            }
         }
      }
   return TRUE;
}
//------------------------------------------------------------------------------


