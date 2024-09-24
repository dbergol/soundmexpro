//------------------------------------------------------------------------------
/// \file SoundDllPro_Tools.h
/// \author Berg
/// \brief Implementation of helper functions for module
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
#ifndef SoundDllPro_ToolsParseH
#define SoundDllPro_ToolsParseH

#include <vcl.h>
#include <vector>
#include "soundmexpro_defs.h"


//------------------------------------------------------------------------------
/// helper define for deleting classes safely
//------------------------------------------------------------------------------
#ifndef TRYDELETENULL
   #define TRYDELETENULL(p) {if (p!=NULL) { try {delete p;} catch (...){;} p = NULL;}}
#endif
//------------------------------------------------------------------------------

//---------------------------------------------------------------------------
/// converts a cDelimiter-delimited string list into passed vector of strings
//---------------------------------------------------------------------------
void ParseValues(TStringList *psl, const char* lpcsz, char cDelimter = ';', bool bRemoveQuotes = false);

//---------------------------------------------------------------------------

//---------------------------------------------------------------------------
/// Sets a value in a stringlist. If strValue is empty, the string 'strField='
/// is added to stringlist
//---------------------------------------------------------------------------
void SetValue(TStringList *psl, AnsiString strField, AnsiString strValue);
//------------------------------------------------------------------------------


//------------------------------------------------------------------------------
/// tool function trimming an std::string in place
//------------------------------------------------------------------------------
void trim(std::string& str, char cTrim);

#endif
