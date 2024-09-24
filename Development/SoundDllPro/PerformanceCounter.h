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
#ifndef PerformanceCounterH
#define PerformanceCounterH
//------------------------------------------------------------------------------
#include <windows.h>
//------------------------------------------------------------------------------

//------------------------------------------------------------------------------
/// \class CPerformanceCounter. Encapsulates calls to QueryPerformance??? commands
//------------------------------------------------------------------------------
class CPerformanceCounter
{
   public:
      CPerformanceCounter();
      void     Start();
      double   Stop();
      void     Reset();
      double   DecayValue();
      double   Value();
      double   MaxValue();
   private:
	  bool           m_bValid;
	  double         m_dFrequency;
	  LARGE_INTEGER  m_liLast;
	  LARGE_INTEGER  m_liThis;
      double         m_dDecayValue;
      double         m_dValue;
      double         m_dValueMax;
};
//------------------------------------------------------------------------------
#endif
