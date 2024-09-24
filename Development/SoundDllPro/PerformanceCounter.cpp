//------------------------------------------------------------------------------
/// \file PerformanceCounter.cpp
/// \author Berg
/// \brief Implementation of class CPerformanceCounter (encapsulating calls
/// to QueryPerformance??? commands)
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

#include "PerformanceCounter.h"
//------------------------------------------------------------------------------

//------------------------------------------------------------------------------
/// Constructor. Retrieves QueryPerformanceFrequency and sets class valid on success.
//------------------------------------------------------------------------------
CPerformanceCounter::CPerformanceCounter()
{
   LARGE_INTEGER  liFreq;
   m_bValid = QueryPerformanceFrequency(&liFreq);
   if (m_bValid)
      m_dFrequency = (double)liFreq.QuadPart;
   Reset();
}
//------------------------------------------------------------------------------

//------------------------------------------------------------------------------
/// Starts counter
//------------------------------------------------------------------------------
void CPerformanceCounter::Start()
{
   if (m_bValid)
      m_bValid = QueryPerformanceCounter(&m_liLast);
}
//------------------------------------------------------------------------------

//------------------------------------------------------------------------------
/// Stops counter, returns elapsed seconds and stores them
//------------------------------------------------------------------------------
double CPerformanceCounter::Stop()
{
   if (m_bValid)
      {
      m_bValid = QueryPerformanceCounter(&m_liThis);
      // calculate time in seconds that is elapsed
      double d = ((double)m_liThis.QuadPart - (double)m_liLast.QuadPart)/ m_dFrequency;
      if (d > m_dValueMax)
         m_dValueMax = d;
      m_dValue = d;
      // store it with decay
      m_dDecayValue = m_dDecayValue * 0.9 + (0.1)*d;
      }
   return m_dValue;
}
//------------------------------------------------------------------------------

//------------------------------------------------------------------------------
/// resets internal performance value
//------------------------------------------------------------------------------
void CPerformanceCounter::Reset()
{
   m_dValue    = 0.0;
   m_dValueMax = 0.0;
}
//------------------------------------------------------------------------------

//------------------------------------------------------------------------------
/// returns internal performance value
//------------------------------------------------------------------------------
double CPerformanceCounter::Value()
{
   return m_dValue;
}
//------------------------------------------------------------------------------

//------------------------------------------------------------------------------
/// returns internal performance value with decay
//------------------------------------------------------------------------------
double CPerformanceCounter::DecayValue()
{
   return m_dDecayValue;
}
//------------------------------------------------------------------------------

//------------------------------------------------------------------------------
/// returns internal max performance value
//------------------------------------------------------------------------------
double CPerformanceCounter::MaxValue()
{
   return m_dValueMax;
}
//------------------------------------------------------------------------------
