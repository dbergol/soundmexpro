//------------------------------------------------------------------------------
/// \file SoundDllPro_Debug.cpp
/// \author Berg
/// \brief Implementation of class SDPDebug for debugging purposes.
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

#include "SoundDllPro_Debug.h"
//------------------------------------------------------------------------------
#pragma package(smart_init)

//------------------------------------------------------------------------------
/// constructor, initializes members
//------------------------------------------------------------------------------
SDPDebug::SDPDebug()
   :m_psl(NULL)
{
   m_psl = new TStringList();
   m_dwLast = GetTickCount();
}
//------------------------------------------------------------------------------

//------------------------------------------------------------------------------
/// destructor, does cleanup
//------------------------------------------------------------------------------
SDPDebug::~SDPDebug()
{
   if (m_psl)
      delete m_psl;
}
//------------------------------------------------------------------------------

//------------------------------------------------------------------------------
/// Adds one string + timing info to internal string list
//------------------------------------------------------------------------------
void SDPDebug::Add(UnicodeString us)
{
   DWORD dw = GetTickCount() - m_dwLast;
   m_dwLast = GetTickCount();

   us = IntToStr((int)dw) + ": " + us;
   m_psl->Add(us);
}
//------------------------------------------------------------------------------

