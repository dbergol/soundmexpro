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
#include "SoundDllPro_Tools.h"
#include "formAbout.h"
#include <stdio.h>
#include <math.h>
#include <math.hpp>
#include <string>
#include <Registry.hpp>
#include <Inifiles.hpp>
#include "formAbout.h"
#include "VersionCheck.h"
#include "frmVersionCheck.h"

#pragma warn -use


//------------------------------------------------------------------------------
/// Checks for updates with GUI interaction
//------------------------------------------------------------------------------
void  CheckForUpdate(TForm* pfrmOwner)
{   
   HWND hwnd = pfrmOwner ? pfrmOwner->Handle : NULL;
   
   TVersionChecker   vch;
   if (!vch.ReadVersionHistoryURL("https://www.soundmexpro.de/downloads/history.txt"))
      {
      MessageBox(hwnd, "Unkown error checking for updates", "Error", MB_ICONERROR);
      return;      
      }

   UnicodeString usVersion = VString() + ".0";
   // uncomment to set older version for testing ....  
   // usVersion = "2.8.0.0";
      
   if (vch.VersionIsLatest(usVersion))
      MessageBoxW(hwnd, L"You are running the latest version of SoundMexPro", L"Information", MB_ICONINFORMATION);
   else
      {
      TformVersionCheck* pfrm = new TformVersionCheck(pfrmOwner);
      pfrm->DoShowModal(vch, "SoundMexPro", usVersion, "https://www.soundmexpro.de/download");
      TRYDELETENULL(pfrm);
      }
}
//------------------------------------------------------------------------------

//------------------------------------------------------------------------------
/// Returns latest available version
//------------------------------------------------------------------------------
AnsiString  GetLatestVersion(bool &rbUpdate)
{
   AnsiString asReturn;
   TVersionChecker   vch;
   if (vch.ReadVersionHistoryURL("https://www.soundmexpro.de/downloads/history.txt"))
      {
      asReturn = vch.GetVersionLatest();
      rbUpdate = !vch.VersionIsLatest(VString()+".0");
      // uncomment to set older version for testing ....  
      // rbUpdate = !vch.VersionIsLatest("2.9.0.0");
      }
   return asReturn;
}
//------------------------------------------------------------------------------

//------------------------------------------------------------------------------
/// returns value of a hanning ramp
//------------------------------------------------------------------------------
float GetHanningRamp(unsigned int uWindowPos, unsigned int uWindowLen, bool bUp)
{
   if (!uWindowLen)
      throw Exception("invalid window lenght passed to " + UnicodeString(__FUNC__));
   if (uWindowPos > uWindowLen)
      {
      OutputDebugStringW((L"error: " + IntToStr((int64_t)uWindowPos) + L", " + IntToStr((int64_t)uWindowLen)).w_str());
      throw Exception("invalid window pos passed to "  + UnicodeString(__FUNC__));
      }
   if (!bUp)
      uWindowPos = uWindowLen - uWindowPos;
   return (float)(0.5 - 0.5*cos(M_PI*(double)(uWindowPos)/(double)uWindowLen));
}
//------------------------------------------------------------------------------

//------------------------------------------------------------------------------
/// returns last windows error ans an AnsiString
//------------------------------------------------------------------------------
AnsiString GetLastWindowsError()
{
   DWORD dw = ::GetLastError();
   LPVOID lpMsgBuf;
   FormatMessage(
       FORMAT_MESSAGE_ALLOCATE_BUFFER |
       FORMAT_MESSAGE_FROM_SYSTEM |
       FORMAT_MESSAGE_IGNORE_INSERTS,
       NULL,
       dw,
       MAKELANGID(LANG_NEUTRAL, SUBLANG_DEFAULT), // Default language
       (LPTSTR) &lpMsgBuf,
       0,
       NULL
   );
   AnsiString strError = (LPCSTR)lpMsgBuf;
   // Free the buffer.
   LocalFree( lpMsgBuf );
   return strError;
}
//------------------------------------------------------------------------------


//------------------------------------------------------------------------------
/// comments see SoundDllPro_Tools.h
//------------------------------------------------------------------------------
void WriteToLogFile(AnsiString str, AnsiString strFile)
{
   if (strFile.IsEmpty() || strFile.Length() == 0)
      strFile = g_strLogFile;
   if (strFile.IsEmpty() || strFile.Length() == 0)
      return;
   AnsiString strError;
   FILE *file = fopen(strFile.c_str(), "a");
   try
      {
      try
         {
         if (file)
            fprintf(file, "%s\n", str.c_str());
         }
      __finally
         {
         if (file)
            fclose(file);
         }
      }
   catch (Exception &e)
      {
      strError = e.Message;
      }
   catch (...)
      {
      strError = "unknown error";
      }
   if (!strError.IsEmpty())
      throw Exception("error writing to logfile '" + ExpandFileName(strFile) + "': " + strError);
}
//------------------------------------------------------------------------------

//------------------------------------------------------------------------------
/// debug logfile functions
#ifdef DEBUG_LOGFILE
/// initialize global variable
TStringList *g_pslDebug = NULL;
//------------------------------------------------------------------------------
/// comments see SoundDllPro_Tools.h
//------------------------------------------------------------------------------
void  InitDebug()
{
   if (!g_pslDebug)
      g_pslDebug = new TStringList();
}
//------------------------------------------------------------------------------

//------------------------------------------------------------------------------
/// comments see SoundDllPro_Tools.h
//------------------------------------------------------------------------------
void  ExitDebug()
{
   if (g_pslDebug)
	  {
	  g_pslDebug->SaveToFile("c:\\soundmexpro.log");
	  TRYDELETENULL(g_pslDebug);
	  }
}
//------------------------------------------------------------------------------

//------------------------------------------------------------------------------
/// comments see SoundDllPro_Tools.h
//------------------------------------------------------------------------------
void AddDebugString(AnsiString s)
{
   if (g_pslDebug)
	  g_pslDebug->Add(s);
}
//------------------------------------------------------------------------------
#endif

//------------------------------------------------------------------------------
/// comments see SoundDllPro_Tools.h
//------------------------------------------------------------------------------
void WriteDebugString(AnsiString s, AnsiString strFile)
{
   OutputDebugString(s.c_str());
   if (strFile == "")
      strFile = "c:\\soundmexpro.log";
   FILE *file = fopen(strFile.c_str(), "a");
   try
      {
      if (file)
         fprintf(file, "%s\n", s.c_str());
      }
   __finally
      {
      if (file)
         fclose(file);
      }
}
//------------------------------------------------------------------------------

//------------------------------------------------------------------------------
/// comments see SoundDllPro_Tools.h
//------------------------------------------------------------------------------
bool IsAudioSpike()
{
   char c[2*MAX_PATH];
   ZeroMemory(c, 2*MAX_PATH);
   if (GetModuleFileName(GetModuleHandle(NULL), c, 2*MAX_PATH-1))
      return UpperCase(c).Pos("AUDIOSPIKE") > 0;
   return false;
}      
//------------------------------------------------------------------------------


//------------------------------------------------------------------------------
/// comments see SoundDllPro_Tools.h
//------------------------------------------------------------------------------
bool IsDouble(AnsiString s)
{
   char *endptr = NULL;
   s = Trim(s);
   strtod(s.c_str(), &endptr);
   if (*endptr != NULL || s.Length()==0)
      return false;
   return true;
}
//------------------------------------------------------------------------------

//------------------------------------------------------------------------------
/// comments see SoundDllPro_Tools.h
//------------------------------------------------------------------------------
AnsiString DoubleToStr(double val, const char* lpcszFormat)
{
   AnsiString s;
   if (!lpcszFormat)
	  lpcszFormat = "%lf";
   s.sprintf(lpcszFormat, val);
   return s;
}
//------------------------------------------------------------------------------

//------------------------------------------------------------------------------
/// comments see SoundDllPro_Tools.h
//------------------------------------------------------------------------------
double StrToDouble(AnsiString s)
{
   char *endptr = NULL;
   double value;
   s = Trim(s);
   value = strtod(s.c_str(), &endptr);
   if (*endptr != NULL || s.Length()==0)
      throw Exception("Not a double");
   return value;
}
//------------------------------------------------------------------------------

//------------------------------------------------------------------------------
/// comments see SoundDllPro_Tools.h
//------------------------------------------------------------------------------
void  RemoveTrailingChar(AnsiString &str, char c)
{
   int n = str.Length();
   if (n)
      {
      if (str[n] == c)
         str.Delete(n, 1);
      }
}
//------------------------------------------------------------------------------

//------------------------------------------------------------------------------
/// comments see SoundDllPro_Tools.h
//------------------------------------------------------------------------------
 int64_t ElapsedSince(DWORD dw)
{
   int64_t i64Then = GetTickCount();
   // overflow of GetTickCount?
   if (i64Then < dw)
	  i64Then += UINT_MAX;
   return (i64Then - dw);
}
//------------------------------------------------------------------------------

//------------------------------------------------------------------------------
/// comments see SoundDllPro_Tools.h
//------------------------------------------------------------------------------
std::vector<int> ConvertChannelArgument(  AnsiString strChannels,
                                          int nNumChannels,
                                          int nFlags
                                          )
{
   std::vector<int> vi;
   bool bEmptyOnEmpty   = (nFlags&CCA_ARGS_EMPTYONEMPTY)!=0;
   bool bNegAllowed     = (nFlags&CCA_ARGS_NEGALLOWED)!=0;
   bool bDupAllowed     = (nFlags&CCA_ARGS_DUPALLOWED)!=0;
   bool bNegDupAllowed  = (nFlags&CCA_ARGS_NEGDUPALLOWED)!=0;
   if (strChannels.IsEmpty())
      {
      if (bEmptyOnEmpty)
         return vi;
      AnsiString str;
      for (int i = 0; i < nNumChannels; i++)
         vi.push_back(i);
      }
   else
      {
      int nChannelIndex;
      TStringList *pslTmp = new TStringList();
      try
         {
         pslTmp->Delimiter = ',';
         pslTmp->DelimitedText = strChannels;
         for (int i = 0; i < pslTmp->Count; i++)
            {
            if (!TryStrToInt(pslTmp->Strings[i], nChannelIndex))
               throw Exception("invalid channel: must be integer in "  + UnicodeString(__FUNC__));
            if (nChannelIndex >= nNumChannels)
               throw Exception("invalid channel: out of range in "  + UnicodeString(__FUNC__));
            if (!bNegAllowed && nChannelIndex < 0)
               throw Exception("invalid channel: out of range (negative) in "  + UnicodeString(__FUNC__));
            if (!bDupAllowed)
               {
               if (nChannelIndex > 0 || !bNegDupAllowed)
                  {
                  for (unsigned int j = 0; j < vi.size(); j++)
                     {
                     if (nChannelIndex == vi[j] && nChannelIndex >= 0)
                        throw Exception("duplicate channel passed");
                     }
                  }
               }
            vi.push_back(nChannelIndex);
            }
         }
      __finally
         {
         TRYDELETENULL(pslTmp);
         }
      }
   if (vi.size() == 0)
      throw Exception("fatal channel error (no channels in vector)");
   return vi;
}
//------------------------------------------------------------------------------

//------------------------------------------------------------------------------
/// comments see SoundDllPro_Tools.h
//------------------------------------------------------------------------------
AnsiString  ConvertChannelVector(const std::vector<int>& vi)
{
   AnsiString str;
   unsigned int i;
   for (i = 0; i < vi.size(); i++)
	  str += IntToStr(vi[i]) + ",";
   // remove last ','
   RemoveTrailingChar(str);
   return str;
}
//------------------------------------------------------------------------------

//------------------------------------------------------------------------------
/// checks if a value is of a particular type
//------------------------------------------------------------------------------
void CheckVal(float fValue, TValType nType, LPCSTR lpcszName)
{
   // switch off clang warning: float comparison by purpose
   #pragma clang diagnostic push
   #pragma clang diagnostic ignored "-Wfloat-equal"
   if (nType == VAL_POS && fValue <= 0)
      throw Exception("value for parameter '" + AnsiString(lpcszName) + "' invalid: must be > 0");
   else if (nType == VAL_NEG && fValue >= 0)
      throw Exception("value for parameter '" + AnsiString(lpcszName) + "' invalid: must be < 0");
   else if (nType == VAL_POS_OR_ZERO && fValue < 0)
      throw Exception("value for parameter '" + AnsiString(lpcszName) + "' invalid: must be >= 0");
   else if (nType == VAL_NEG_OR_ZERO && fValue > 0)
      throw Exception("value for parameter '" + AnsiString(lpcszName) + "' invalid: must be <= 0");
   else if (nType == VAL_NON_ZERO && fValue == 0)
      throw Exception("value for parameter '" + AnsiString(lpcszName) + "' invalid: must be != 0");
   #pragma clang diagnostic pop
}
//------------------------------------------------------------------------------

//------------------------------------------------------------------------------
/// checks if a value is of a particular type
//------------------------------------------------------------------------------
void CheckVal(int64_t nValue, TValType nType, LPCSTR lpcszName)
{
   if (nType == VAL_POS && nValue <= 0)
      throw Exception("value for parameter '" + AnsiString(lpcszName) + "' invalid: must be > 0");
   else if (nType == VAL_NEG && nValue >= 0)
      throw Exception("value for parameter '" + AnsiString(lpcszName) + "' invalid: must be < 0");
   else if (nType == VAL_POS_OR_ZERO && nValue < 0)
      throw Exception("value for parameter '" + AnsiString(lpcszName) + "' invalid: must be >= 0");
   else if (nType == VAL_NEG_OR_ZERO && nValue > 0)
      throw Exception("value for parameter '" + AnsiString(lpcszName) + "' invalid: must be <= 0");
   else if (nType == VAL_NON_ZERO && nValue == 0)
      throw Exception("value for parameter '" + AnsiString(lpcszName) + "' invalid: must be != 0");
}
//------------------------------------------------------------------------------

//------------------------------------------------------------------------------
/// comments see SoundDllPro_Tools.h
//------------------------------------------------------------------------------
void CopyNonExistingEntries(TStrings *psSource, TStrings *psTarget)
{
   if (!psSource || !psTarget)
      throw Exception("invalid strings object passed to "  + UnicodeString(__FUNC__));
   for (int i = 0; i < psSource->Count; i++)
      {
      if (psTarget->Values[psSource->Names[i]].IsEmpty())
         psTarget->Values[psSource->Names[i]] = psSource->Values[psSource->Names[i]];
      }
}
//------------------------------------------------------------------------------

//------------------------------------------------------------------------------
/// comments see SoundDllPro_Tools.h
//------------------------------------------------------------------------------
int64_t  GetInt(TStringList *psl, LPCSTR lpcszName, TValType nType)
{
   return GetInt(psl, lpcszName, 0, nType, true);
}
//------------------------------------------------------------------------------

//------------------------------------------------------------------------------
/// comments see SoundDllPro_Tools.h
//------------------------------------------------------------------------------
int64_t GetInt(TStringList *psl, LPCSTR lpcszName, int64_t nDefault, TValType nType, bool bMustExist)
{
   if (!psl)
      throw Exception("psl invalid in "  + UnicodeString(__FUNC__));
   int64_t nValue;
   AnsiString str = psl->Values[lpcszName];
   if (str.IsEmpty())
      {
      if (bMustExist)
         throw Exception("value for parameter '" + AnsiString(lpcszName) + "' missing");
      nValue = nDefault;
      }
   else
      {
      try
         {
         nValue = StrToInt64(str);
         }
      catch (...)
         {
         throw Exception("value for parameter '" + AnsiString(lpcszName) + "' invalid (must be integer)");
         }
      }
   // check validity
   CheckVal(nValue, nType, lpcszName);
   return nValue;
}
//------------------------------------------------------------------------------

//------------------------------------------------------------------------------
/// comments see SoundDllPro_Tools.h
//------------------------------------------------------------------------------
float  GetFloat(TStringList *psl, LPCSTR lpcszName, TValType nType)
{
   return GetFloat(psl, lpcszName, 0, nType, true);
}
//------------------------------------------------------------------------------

//------------------------------------------------------------------------------
/// comments see SoundDllPro_Tools.h
//------------------------------------------------------------------------------
float GetFloat(TStringList *psl, LPCSTR lpcszName, float fDefault, TValType nType, bool bMustExist)
{
   if (!psl)
      throw Exception("psl invalid in "  + UnicodeString(__FUNC__));
   float fValue;
   AnsiString str = psl->Values[lpcszName];
   if (str.IsEmpty())
      {
      if (bMustExist)
         throw Exception("value for parameter '" + AnsiString(lpcszName) + "' missing");
      fValue = fDefault;
      }
   else
      {
      if (!IsDouble(str))
         throw Exception("value for parameter '" + AnsiString(lpcszName) + "' invalid (must be float)");
      fValue = (float)StrToDouble(str);
      }
   // check validity
   CheckVal(fValue, nType, lpcszName);
   return fValue;
}
//------------------------------------------------------------------------------


//------------------------------------------------------------------------------
/// reads int value from inifile
//------------------------------------------------------------------------------
int GetProfileStringDefault(AnsiString strField, int nDefault, AnsiString strFileName)
{
   AnsiString as = GetProfileStringDefault(strField, IntToStr(nDefault), strFileName);
   int n;
   if (TryStrToInt(as, n))
      return n;
   return nDefault;
}
//------------------------------------------------------------------------------

//------------------------------------------------------------------------------
/// reads string value from inifile
//------------------------------------------------------------------------------
AnsiString GetProfileStringDefault(AnsiString strField, AnsiString strDefault, AnsiString strFileName)
{
   if (strFileName == "")
      strFileName = "soundmexpro.ini";
   char c[MAX_PATH];
   ZeroMemory(c, MAX_PATH);
   int n;
   if (!!GetPrivateProfileString("Settings",
                                 strField.c_str(),
                                 strDefault.c_str(),
                                 c,
                                 MAX_PATH,
                                 AnsiString(g_strBinPath + strFileName).c_str()
                                 )
      )
      {
      AnsiString as = c;
      if (as.Length() > 0)
         return as;
      }
   return strDefault;
}
//------------------------------------------------------------------------------

//------------------------------------------------------------------------------
/// writes int value to inifile
//------------------------------------------------------------------------------
void SetProfileString(AnsiString strField, int nValue, AnsiString strFileName)
{
   SetProfileString(strField, IntToStr(nValue), strFileName);
}
//------------------------------------------------------------------------------

//------------------------------------------------------------------------------
/// writes string value to inifile
//------------------------------------------------------------------------------
void SetProfileString(AnsiString strField, AnsiString strValue, AnsiString strFileName)
{
   if (strFileName == "")
      strFileName = "soundmexpro.ini";
   WritePrivateProfileString( "Settings",
                              strField.c_str(),
                              strValue.c_str(),
                              AnsiString(g_strBinPath + strFileName).c_str()
                              );
}
//------------------------------------------------------------------------------

//------------------------------------------------------------------------------
/// Converts a linear factor to dB
//------------------------------------------------------------------------------
float        FactorTodB(float f)
{
   if (f <= 0.0f || IsInfinite(f) || IsNan(f))
	  return -180.0f;
   else
	  return (float)(20.0f*log10(f));
}
//------------------------------------------------------------------------------

//------------------------------------------------------------------------------
/// Converts dB to a linear factor
//------------------------------------------------------------------------------
float        dBToFactor(float f)
{
   return (float)(pow(10, f/20.0f));
}
//------------------------------------------------------------------------------

//------------------------------------------------------------------------------
// searches for particuluar memory content in a file
//------------------------------------------------------------------------------
bool      FileContains(AnsiString asName, char* lpcsz, int nSize)
{
   if (!FileExists(asName))
      return false;
   bool bReturn = false;
   TMemoryStream* pms = new TMemoryStream();
   try
      {
      pms->LoadFromFile(asName);
      char* pc = (char*)pms->Memory;
      for (int i = 0; i < (pms->Size - nSize); i++)
         {
         if (memcmp(pc, lpcsz, (unsigned int)nSize) == 0)
            {
            bReturn = true;
            break;
            }
         pc++;
         }
      }
   catch (...)
      {
      }
   TRYDELETENULL(pms);
   return bReturn;
}
//------------------------------------------------------------------------------

//------------------------------------------------------------------------------
/// comments see SoundDllPro_Tools.h
//------------------------------------------------------------------------------
AnsiString VString()
{
   return GetFileVersion(GetModuleHandle(SOUNDDLLPRO_DLL));
}
//------------------------------------------------------------------------------

//------------------------------------------------------------------------------
/// comments see SoundDllPro_Tools.h
//------------------------------------------------------------------------------
AnsiString GetFileVersion(HINSTANCE hLib)
{
   AnsiString as;
   char cFilename[2*MAX_PATH];
   ZeroMemory(cFilename, 2*MAX_PATH);
   if (!GetModuleFileName(hLib, cFilename, 2*MAX_PATH-1))
      return as;

   DWORD dwHandle;
   DWORD sz = GetFileVersionInfoSizeA( cFilename, & dwHandle );
   if ( 0 == sz )
      return as;

   char *buf = new char[sz];
   if ( !GetFileVersionInfoA( cFilename, dwHandle, sz, & buf[ 0 ] ) )
      {
      delete[] buf;
      return as;
      }
      
   VS_FIXEDFILEINFO * pvi;
   sz = sizeof( VS_FIXEDFILEINFO );
   if ( !VerQueryValueA( & buf[ 0 ], "\\", (LPVOID*)&pvi, (unsigned int*)&sz ) )
      {
      delete[] buf;
      return as;
      }

   as.sprintf( "%d.%d.%d",
               pvi->dwProductVersionMS >> 16,
               pvi->dwFileVersionMS & 0xFFFF,
               pvi->dwFileVersionLS >> 16
             );

   delete[] buf;
   return as;
}
//------------------------------------------------------------------------------
