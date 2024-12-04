//------------------------------------------------------------------------------
/// \file soundmexpro.h
/// \author Berg
/// \brief main interface for soundmexpro.dll. Contains definitions and interface
/// of helper classes/functions for soundmexpro
///
/// Project SoundMexPro
/// Module  soundmexpro.dll
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
#ifndef soundmexproH
#define soundmexproH

#include <windows.h>
#include <string>
#include <vector>


/// microsoft specials 
#ifdef _MSC_VER 
   #define snprintf sprintf_s
	#define int64_t __int64
#else
	#include <stdint.h>
#endif

#ifndef NOMEX
#include "mex.h"
#endif

//------------------------------------------------------------------------------

#define SOUNDMEX_HELP \
"     SoundMexPro © Copyright 2023 Dr. Daniel Berg, Carl von Ossietzky University Oldenburg\n\n\
                http://www.soundmexpro.de   \n\n\
     SoundMexPro is a powerful tool for ASIO sound applications in MATLAB, GNU Octave and Python.\n\
     Read the following help carefully and/or print it. Take a look at the example scripts.\n\
     Call soundmexpro('help') to see a list of all available commands\n\
     Call soundmexpro('help', COMMANDNAME) to see help on command COMMANDNAME\n\
     For MATLAB/Octave all commands return '1' if the function succeeded and '0' on errors. \n\
     Functions, that have additional return values (e.g. volume, isplaying, xrun ...) \n\
     return these values in additional output arguments.\n\
     ATTENTION! This means that e.g. 'xrun' will return '1' if the command itself\n\
     succeeds. That does not indicate that the device had xruns! Evaluate the \n\
     next return value(s) (second, third...)!!! \n\
     For Python a dictionary is returned with return values. On any error soundmexpro.py\n\
     raises an AssertionError. You may change this behaviour in soundmexpro.py if needed\n\n\
     MATLAB by The Mathworks, Inc.\n\
     ASIO is a trademark and software of Steinberg Media Technologies GmbH.\n\
     VST Interface Technology by Steinberg Media Technologies GmbH.\n\
     VST is a trademark and software of Steinberg Media Technologies GmbH.\n"

//------------------------------------------------------------------------------
std::string NameFromString(std::string& str);
std::string ValueFromString(std::string& str);
std::string GetValue(std::vector<std::string>& vs, const char* lpcszName);
void        SetValue(std::vector<std::string>& vs, const char* lpcszName, const char* lpcszValue);
void        RemoveValue(std::vector<std::string>& vs, const char* lpcszName);
void        ParseValues(std::string str, std::vector<std::string>& vs, char cDelimiter = ';');
void        trim(std::string& str, char cTrim = ' ');
bool        IsDouble(std::string s);
std::string DoubleToStr(double val);
double      StrToDouble(std::string s);
std::string IntegerToStr(int64_t val);
int64_t     StrToInteger(std::string s);
bool        TryStrToInteger(std::string s, int64_t& n64);
#ifndef NOMEX
bool        IsScalarDouble(const mxArray *phs);
void        GetButtonWindowProperties(mxArray** mxa, RECT &rc, std::string &strCaption);
bool        IsDllCommand(std::string strCommand, std::string strHelp);
#endif

//------------------------------------------------------------------------------





//------------------------------------------------------------------------------
// Error class (only holds error string and type)
//------------------------------------------------------------------------------
class SOUNDMEX_Error {
   public:
      SOUNDMEX_Error(std::string sErrorMessage);
      SOUNDMEX_Error(std::string sErrorMessage, int iType);
      std::string Message;
      int iErrorType;
};
//------------------------------------------------------------------------------
#endif
