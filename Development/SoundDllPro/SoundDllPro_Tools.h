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
#ifndef SoundDllPro_ToolsH
#define SoundDllPro_ToolsH
#include <vcl.h>
#include <vector>
#include "soundmexpro_defs.h"
#include "SoundDllPro_ToolsParse.h"
#include "SoundDllPro_Style.h"

/// define name of 'self'
#define SOUNDDLLPRO_DLL          "sounddllpro.dll"

//------------------------------------------------------------------------------
/// enumeration of value types
//------------------------------------------------------------------------------
enum TValType {
   VAL_ALL = 0,         /// all values allowed
   VAL_POS,             /// positive values allowed
   VAL_NEG,             /// negative values allowed
   VAL_POS_OR_ZERO,     /// positive values or 0 allowed
   VAL_NEG_OR_ZERO,     /// negative values or 0 allowed
   VAL_NON_ZERO         /// all non-zero values allowed
} ;
//------------------------------------------------------------------------------


//------------------------------------------------------------------------------
/// prototypes of tool functions
//------------------------------------------------------------------------------
void        CheckForUpdate(TForm* pfrm);
AnsiString  GetLatestVersion(bool &rbUpdate);
bool        IsDouble(AnsiString s);
AnsiString  DoubleToStr(double val, const char* lpcszFormat = NULL);
double      StrToDouble(AnsiString s);
bool        FileContains(AnsiString asName, char* lpcsz, int nSize);
float       GetHanningRamp(unsigned int uWindowPos, unsigned int uWindowLen, bool bUp);
void        CheckVal(float fValue, TValType nType, LPCSTR lpcszName);
void        CheckVal(int64_t nValue, TValType nType, LPCSTR lpcszName);

//------------------------------------------------------------------------------

//------------------------------------------------------------------------------
/// error functions
//------------------------------------------------------------------------------
AnsiString GetLastWindowsError();
bool       IsAudioSpike();
//------------------------------------------------------------------------------
/// INI functions
//------------------------------------------------------------------------------
int GetProfileStringDefault(AnsiString strField, int nDefault, AnsiString strFileName = "");
AnsiString GetProfileStringDefault(AnsiString strField, AnsiString strDefault, AnsiString strFileName = "");
void SetProfileString(AnsiString strField, int nValue, AnsiString strFileName = "");
void SetProfileString(AnsiString strField, AnsiString strValue, AnsiString strFileName = "");

// gloabl variables (externals here)
extern AnsiString g_strLogFile;
extern AnsiString g_strBinPath;
//------------------------------------------------------------------------------
/// writes debug string to g_strLogFile
//------------------------------------------------------------------------------
void WriteToLogFile(AnsiString str, AnsiString strFile = "");
//------------------------------------------------------------------------------
/// helper functions for debugging
//------------------------------------------------------------------------------
#ifdef DEBUG_LOGFILE
/// global TStringList
extern TStringList *g_pslDebug;
//------------------------------------------------------------------------------
/// init debugging TStringList
//------------------------------------------------------------------------------
void  InitDebug();
//------------------------------------------------------------------------------
/// save (c:\\soundmexpro.log) and exit debugging TStringList
//------------------------------------------------------------------------------
void  ExitDebug();
//------------------------------------------------------------------------------
/// add debug string to string list
//------------------------------------------------------------------------------
void  AddDebugString(AnsiString s);
//------------------------------------------------------------------------------
#endif
//------------------------------------------------------------------------------
/// writes debug string to soundmexpro.log on cdrive root or passed file
//------------------------------------------------------------------------------
void WriteDebugString(AnsiString s, AnsiString strFile = "");


/// bitmask for ConvertChannelArgument
#define CCA_ARGS_NULL            0x00
#define CCA_ARGS_EMPTYONEMPTY    0x01
#define CCA_ARGS_NEGALLOWED      0x02
#define CCA_ARGS_DUPALLOWED      0x04
#define CCA_ARGS_NEGDUPALLOWED   0x08

//------------------------------------------------------------------------------
/// \brief returns elapsed time in milliseconds with respect to DWORD overflow
/// (if overflow is detected, function takes this into account by adding UINT_MAX
/// to current GetTckCount value before calculating the eplapsed time difference)
/// \param[in] dw time in milliseconds
/// \return int64_t
/// \retval time elapsed between dw and GetTickCount()
/// NOTE: we use a signed int64_t to allow testing of overflow!
//------------------------------------------------------------------------------
int64_t ElapsedSince(DWORD dw);
//------------------------------------------------------------------------------
//------------------------------------------------------------------------------
/// helper function removing a trailing character form a string
//------------------------------------------------------------------------------
void  RemoveTrailingChar(AnsiString &str, char c = ',');
//------------------------------------------------------------------------------
//------------------------------------------------------------------------------
/// converts a comma deparated list of integer channel values to an int vector
/// with range and duplicate check. 
//------------------------------------------------------------------------------
std::vector<int>  ConvertChannelArgument(AnsiString strChannels, int nNumChannels, int nFlags = 0);
//------------------------------------------------------------------------------
//------------------------------------------------------------------------------
/// converts an integer vector to a comma deparated string containing the ints
/// as string 
//------------------------------------------------------------------------------
AnsiString  ConvertChannelVector(const std::vector<int>& vi);
//------------------------------------------------------------------------------
//------------------------------------------------------------------------------
/// copies ini-like values from one source TStrings object to target object, if
/// they do not exits in target
//------------------------------------------------------------------------------
void              CopyNonExistingEntries(TStrings *psSource, TStrings *psTarget);
//------------------------------------------------------------------------------

//------------------------------------------------------------------------------
/// retrieves a named int value from an INI-like string list with type checking
/// and default value
//------------------------------------------------------------------------------
int64_t           GetInt(  TStringList *psl,
                           LPCSTR lpcszName,
                           int64_t nDefault,
                           TValType nType = VAL_ALL,
                           bool bMustExist = false
                           );
//------------------------------------------------------------------------------
//------------------------------------------------------------------------------
/// retrieves a named int value from an INI-like string list with type checking
/// and default value
//------------------------------------------------------------------------------
int64_t           GetInt(TStringList *psl, LPCSTR lpcszName, TValType nType = VAL_ALL);
//------------------------------------------------------------------------------
//------------------------------------------------------------------------------
/// retrieves a named float value from an INI-like string list with type
/// checking and default value
//------------------------------------------------------------------------------
float             GetFloat(TStringList *psl,
                           LPCSTR lpcszName,
                           float fDefault,
                           TValType nType = VAL_ALL,
                           bool bMustExist = false
                           );
//------------------------------------------------------------------------------
//------------------------------------------------------------------------------
/// retrieves a named float value from an INI-like string list with type
/// checking and default value
//------------------------------------------------------------------------------
float             GetFloat(TStringList *psl, LPCSTR lpcszName, TValType nType = VAL_ALL);
//------------------------------------------------------------------------------
//------------------------------------------------------------------------------
/// returns version string as MAJOR.MINOR.REVISION
//------------------------------------------------------------------------------
AnsiString        VString();
AnsiString        GetFileVersion(HINSTANCE hLib);
//---------------------------------------------------------------------------
//---------------------------------------------------------------------------
/// Converts a linear factor to dB and vice versa
//---------------------------------------------------------------------------
float        FactorTodB(float f);
float        dBToFactor(float f);
//---------------------------------------------------------------------------
#endif
