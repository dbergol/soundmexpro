//------------------------------------------------------------------------------
/// \file SoundDllPro_Tools.cpp
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
#pragma hdrstop

#include "SoundDllPro_ToolsParse.h"
#include <stdio.h>
#include <math.h>
#include <math.hpp>
#include <string>

#pragma warn -use
//------------------------------------------------------------------------------

//------------------------------------------------------------------------------
/// tool function trimming an std::string in place
//------------------------------------------------------------------------------
void trim(std::string& str, char cTrim)
{
  std::string::size_type pos1 = str.find_first_not_of(cTrim);
  std::string::size_type pos2 = str.find_last_not_of(cTrim);
  str = str.substr(pos1 == std::string::npos ? 0 : pos1,
         pos2 == std::string::npos ? str.length() - 1 : pos2 - pos1 + 1);
}
//------------------------------------------------------------------------------


//------------------------------------------------------------------------------
/// converts a cDelimiter-delimited string list into passed vector of strings
//------------------------------------------------------------------------------
void ParseValues(TStringList *psl, const char* lpcsz, char cDelimter, bool bRemoveQuotes)
{
   psl->Clear();

   std::string str = lpcsz;
   std::string strTmp;
   std::string::size_type pos;
   while (1)
      {
      pos = str.find_first_of(cDelimter);
      if (pos == str.npos)
         strTmp = str;
      else
         strTmp = str.substr(0, pos);
      trim(strTmp, ' ');
      if (bRemoveQuotes)
         trim(strTmp, '"');
      if (!!strTmp.length())
         psl->Add(strTmp.c_str());
      if (pos == str.npos)
         break;
      str = str.erase(0, pos+1);
      }
}
//------------------------------------------------------------------------------

//------------------------------------------------------------------------------
/// Sets a value in a stringlist. If strValue is empty, the string 'strField='
/// is added to stringlist
//------------------------------------------------------------------------------
void SetValue(TStringList *psl, AnsiString strField, AnsiString strValue)
{
   strValue = Trim(strValue);
   if (strValue.IsEmpty())
      psl->Add(strField + "=");
   else
      psl->Values[strField] = strValue;
}
//------------------------------------------------------------------------------




