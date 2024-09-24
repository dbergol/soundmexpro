//------------------------------------------------------------------------------
/// \file HanningWindow.h
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
#ifndef HanningWindowH
#define HanningWindowH
//------------------------------------------------------------------------------
#include <vcl.h>

//------------------------------------------------------------------------------
/// \enum WindowState describes different states of a (fading) window. Prefix rs
//------------------------------------------------------------------------------
enum WindowState
{
   WINDOWSTATE_UP = 0,    /// < \var window is 'up' (factor 1)
   WINDOWSTATE_DOWN,      /// < \var window is 'down' (factor 0)
   WINDOWSTATE_RUNUP,     /// < \var window is running up (from factor 0 to factor 1)
   WINDOWSTATE_RUNDOWN    /// < \var window is running down (from factor 1 to factor 0)
};
//------------------------------------------------------------------------------

//------------------------------------------------------------------------------
/// returns hanning value for a window
//------------------------------------------------------------------------------
float GetHanningValue(unsigned int uWindowPos, unsigned int uWindowLen);

   //------------------------------------------------------------------------------
   /// \class CHanningWindow, prefix hw
   /// NOTE: intended for stack and class member use. No new or delete operator
   //------------------------------------------------------------------------------
   class CHanningWindow
   {
      friend class UNIT_TEST_CLASS;
      public:
         CHanningWindow();
         WindowState GetState(void);
         bool        Running(void);
         void        SetState(WindowState ws);
         void        SetLength(unsigned int n);
         unsigned int GetLength(void);
         float       GetValue(void);
         unsigned int GetPosition(void);
      private:
         WindowState    m_ws;       ///< actual 'window state' of window
         unsigned int   m_nWindowPos; ///< actual position of window
         unsigned int   m_nWindowLen; ///< length of window
   };

//------------------------------------------------------------------------------
#endif
