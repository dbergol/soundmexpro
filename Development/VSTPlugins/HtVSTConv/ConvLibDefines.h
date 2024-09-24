//------------------------------------------------------------------------------
/// \file ConvLibDefines.h
/// \author Berg
///
/// \brief Interface definitions for a DLL loaded by VST-Plugin HtVSTConv. 
///
/// Project SoundMexPro
/// Module  HtVSTConv.dll
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
#ifndef ConvLibDefinesH
#define ConvLibDefinesH

typedef void*  (cdecl *LPFNCONVINIT)(unsigned, unsigned, unsigned, const float**);
typedef void   (cdecl *LPFNCONVEXIT)(void*);
typedef int    (cdecl *LPFNCONVPROCESS)(void*, unsigned, unsigned, unsigned, float* const*, float* const*);

#endif // ConvLibDefinesH
