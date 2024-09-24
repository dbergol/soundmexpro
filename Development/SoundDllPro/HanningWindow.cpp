//------------------------------------------------------------------------------
/// \file HanningWindow.cpp
///
/// \author Berg
/// \brief Implementation hanning window class CHanningWindow
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
#pragma hdrstop
#include <math.h>
#include <tchar.h>
#include "HanningWindow.h"
//------------------------------------------------------------------------------


//------------------------------------------------------------------------------
/// returns hanning value for a window
//------------------------------------------------------------------------------
float GetHanningValue(unsigned int uWindowPos, unsigned int uWindowLen)
{
   if (!uWindowLen)
      throw Exception("invalid window lenght passed to "  + UnicodeString(__FUNC__));
   return (float)(0.5 - 0.5*cos(M_PI*(double)(uWindowPos)/(double)uWindowLen));
}
//------------------------------------------------------------------------------

//------------------------------------------------------------------------------
/// \class CHanningWindow
///
/// Small helper class for using Hanning windows.
/// The complete window is _not_ genarated/stored in this class. Only the length
/// of the window is stored, and an actual value can be retrieved depending on
/// the 'state' of the window. States are
///   WINDOWSTATE_UP          window is 'up' (factor 1)
///   WINDOWSTATE_DOWN        window is 'down' (factor 0)
///   WINDOWSTATE_RUNUP       window is running up (from factor 0 to factor 1)
///   WINDOWSTATE_RUNDOWN     window is running down (from factor 1 to factor 0)
///
/// The function GetValue() calculates the actual value and increases/decreases
/// the internal position if necessary, as well as the state, if a 'running' window
/// reaches it's border
/// \version Date: 2006-10-26, Author Berg, Origin: Review KM-20061025-1
/// Purpose: comments changed
//------------------------------------------------------------------------------
//------------------------------------------------------------------------------
/// \brief constructor. initializes members
//------------------------------------------------------------------------------
CHanningWindow::CHanningWindow()
   : m_ws(WINDOWSTATE_UP), m_nWindowPos(0), m_nWindowLen(0)
{
}
//------------------------------------------------------------------------------
//------------------------------------------------------------------------------
/// \brief returns actual state of window
/// \retval internal state m_ws
//------------------------------------------------------------------------------
WindowState   CHanningWindow::GetState(void)
{
   return m_ws;
}
//------------------------------------------------------------------------------
//------------------------------------------------------------------------------
/// \retval returns true if ramp is currently running
//------------------------------------------------------------------------------
bool        CHanningWindow::Running(void)
{
   return (m_ws == WINDOWSTATE_RUNUP || m_ws == WINDOWSTATE_RUNDOWN);
}
//------------------------------------------------------------------------------
//------------------------------------------------------------------------------
/// \brief sets actual state of window and adjusts internal position to
/// - 0, if rs is WINDOWSTATE_UP
/// - window length, if rs is WINDOWSTATE_UP
/// \param[in] rs WindowState to set
//------------------------------------------------------------------------------
void CHanningWindow::SetState(WindowState rs)
{
   m_ws = rs;
   if (m_ws == WINDOWSTATE_UP || m_ws == WINDOWSTATE_RUNDOWN)
      m_nWindowPos = 0;
   else if (m_ws == WINDOWSTATE_DOWN || m_ws == WINDOWSTATE_RUNUP)
      m_nWindowPos = m_nWindowLen;
}
//------------------------------------------------------------------------------
//------------------------------------------------------------------------------
/// \brief sets length of window in samples
/// NOTE: this is the half length of a full Hanning window!!
/// \param[in] n length to set
/// \exception Exception if ramp is active (in state WINDOWSTATE_RUNUP or WINDOWSTATE_RUNDOWN)
//------------------------------------------------------------------------------
void CHanningWindow::SetLength(unsigned int n)
{
   // changing length only allowed in non-running state
   if (m_ws == WINDOWSTATE_RUNUP || m_ws == WINDOWSTATE_RUNDOWN)
      throw Exception(_T("changing ramp length not allowed while ramp is active"));
   m_nWindowLen = n;
}
//------------------------------------------------------------------------------
//------------------------------------------------------------------------------
/// \retval returns length of hanning window in samples
//------------------------------------------------------------------------------
unsigned int CHanningWindow::GetLength(void)
{
   return m_nWindowLen;
}
//------------------------------------------------------------------------------
//------------------------------------------------------------------------------
/// \retval returns position of hanning window in samples
//------------------------------------------------------------------------------
unsigned int CHanningWindow::GetPosition(void)
{
   return m_nWindowPos;
}
//------------------------------------------------------------------------------
//------------------------------------------------------------------------------
/// \brief returns the actual value of window, increases/decreases position and
/// adjusts internal state if necessary
/// \retval 0 if ramp length is 0 or actual ramp state is WINDOWSTATE_DOWN
/// \retval 1 if actual ramp state is WINDOWSTATE_UP
/// \retval 0.5 - 0.5*cos(M_PI*(float)(m_nWindowLen-m_nWindowPos)/(float)m_nWindowLen) else
//------------------------------------------------------------------------------
float CHanningWindow::GetValue(void)
{
   // switch immediately to UP/DOWN if ramp is running AND Window-Length is 0
   if (m_nWindowLen == 0)
      {
      if (m_ws == WINDOWSTATE_RUNDOWN)
         m_ws = WINDOWSTATE_DOWN;
      else if (m_ws == WINDOWSTATE_RUNUP)
         m_ws = WINDOWSTATE_UP;
      }
   if (m_ws == WINDOWSTATE_DOWN)
      return 0.0f;
   else if (m_ws == WINDOWSTATE_UP)
      return 1.0f;
   else
      {
      float f = GetHanningValue(m_nWindowLen-m_nWindowPos, m_nWindowLen);
      if (m_ws == WINDOWSTATE_RUNDOWN)
         m_nWindowPos++;
      else if (m_ws == WINDOWSTATE_RUNUP)
         m_nWindowPos--;
      // check boundaries
      if (m_nWindowPos <= 0)
         {
         m_nWindowPos   = 0;
         m_ws           = WINDOWSTATE_UP;
         }
      else if (m_nWindowPos >= m_nWindowLen)
         {
         m_nWindowPos   = m_nWindowLen;
         m_ws           = WINDOWSTATE_DOWN;
         }
      return f;
      }
}
//------------------------------------------------------------------------------

