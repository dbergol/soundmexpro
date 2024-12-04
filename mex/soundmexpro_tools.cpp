//------------------------------------------------------------------------------
/// \file soundmexprotools.cpp
/// \author Berg
/// \brief helper function and error class SOUNDMEX_Error used by soundmexpro.cpp
///
/// Project SoundMexPro
/// Module  soundmexpro
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
#include <string>
#include <sstream>
#include <vector>
#include <windows.h>
#include <math.h>
#include <ole2.h>
#pragma hdrstop
#include "soundmexpro.h"
#include "soundmexpro_defs.h"


// local prototypes
bool  IsDllCommand(std::string strCommand, std::string strHelp);

//-------------------------------------------------------------------------
// Error class (only holds error string and type)
//-------------------------------------------------------------------------
SOUNDMEX_Error::SOUNDMEX_Error(std::string sErrorMessage)
{
   Message     = sErrorMessage;
   iErrorType  = SOUNDDLL_RETURN_MEXERROR;
}
SOUNDMEX_Error::SOUNDMEX_Error(std::string sErrorMessage, int iType)
{
   Message     = sErrorMessage;
   iErrorType  = iType;
}
//------------------------------------------------------------------------------

//------------------------------------------------------------------------------
/// tool function trimming an std::string in place
//-----------------------------------------------------------------------------
void trim(std::string& str, char cTrim)
{
  std::string::size_type pos1 = str.find_first_not_of(cTrim);
  std::string::size_type pos2 = str.find_last_not_of(cTrim);
  str = str.substr(pos1 == std::string::npos ? 0 : pos1,
         pos2 == std::string::npos ? str.length() - 1 : pos2 - pos1 + 1);
}
//------------------------------------------------------------------------------

//------------------------------------------------------------------------------
/// checks if a command is implemented internally and _not_ in DLL
//------------------------------------------------------------------------------
#pragma argsused
bool  IsDllCommand(std::string strCommand, std::string strHelp)
{
   if (!strCommand.length())
      return true;
   if (  0==_strcmpi(strCommand.c_str(), SOUNDDLLPRO_CMD_SHOWERROR)
      || 0==_strcmpi(strCommand.c_str(), SOUNDDLLPRO_CMD_GETLASTERROR)
      )
      return false;
   return true;
}
//------------------------------------------------------------------------------

//------------------------------------------------------------------------------
/// checks if passes string is a double (with dot as separator)
//------------------------------------------------------------------------------
bool IsDouble(std::string s)
{
   char *endptr = NULL;
   trim(s);
   double value = strtod(s.c_str(), &endptr);
   // switch off clang warning: float comparison by purpose
   #pragma clang diagnostic push
   #pragma clang diagnostic ignored "-Wfloat-equal"
   if (*endptr != '\0' || value == HUGE_VAL || s.length()==0 )
      return false;
   #pragma clang diagnostic pop
   return true;
}
//------------------------------------------------------------------------------

//------------------------------------------------------------------------------
/// converts string (with dot as separator) to double
//------------------------------------------------------------------------------
std::string DoubleToStr(double val)
{
   std::ostringstream os;
   os << val;
   std::string s(os.str());
   return s;
}
//------------------------------------------------------------------------------

//------------------------------------------------------------------------------
/// converts double to string (with dot as separator)
//------------------------------------------------------------------------------
double StrToDouble(std::string s)
{
   char *endptr = NULL;
   trim(s);
   double value = strtod(s.c_str(), &endptr);
   // switch off clang warning: float comparison by purpose
   #pragma clang diagnostic push
   #pragma clang diagnostic ignored "-Wfloat-equal"
   if (*endptr != '\0' || value == HUGE_VAL || s.length()==0 )
      throw SOUNDMEX_Error("Not a double");
   #pragma clang diagnostic pop
   return value;
}
//------------------------------------------------------------------------------

//------------------------------------------------------------------------------
/// converts string to int
//------------------------------------------------------------------------------
std::string IntegerToStr(int64_t val)
{
   std::ostringstream os;
   os << val;
   std::string s(os.str());
   return s;
}
//------------------------------------------------------------------------------

//------------------------------------------------------------------------------
/// converts int to string
//------------------------------------------------------------------------------
int64_t     StrToInteger(std::string s)
{
   int64_t value;
   trim(s);
   int nRet = sscanf(s.c_str(), "%lld", &value);
   if (nRet != 1)
      throw SOUNDMEX_Error("Not an int");
   return value;
}
//------------------------------------------------------------------------------

//------------------------------------------------------------------------------
/// tries to convert int to string
//------------------------------------------------------------------------------
bool TryStrToInteger(std::string s, int64_t& n64)
{
   n64 = 0;
   trim(s);
   int64_t value;
   if (sscanf(s.c_str(), "%lld", &value) != 1)
      return false;
   n64 = value;
   return true;
}
//------------------------------------------------------------------------------

//------------------------------------------------------------------------------
/// Returns name from a string: for "x=y" it returns "x"
//------------------------------------------------------------------------------
std::string NameFromString(std::string& str)
{
   std::string strRet;
   std::string::size_type pos = str.find_first_of('=');
   if (pos != str.npos)
      strRet = str.substr(0, pos);
   return strRet;
}
//------------------------------------------------------------------------------

//------------------------------------------------------------------------------
/// Returns value from a string: for "x=y" it returns "y"
//------------------------------------------------------------------------------
std::string ValueFromString(std::string& str)
{
   std::string strRet;
   std::string::size_type pos = str.find_first_of('=');
   if (pos != str.npos)
      strRet = str.substr(pos+1);
   return strRet;
}
//------------------------------------------------------------------------------

//------------------------------------------------------------------------------
/// converts a cDelimiter-delimited string list into passed vector of strings
//------------------------------------------------------------------------------
void ParseValues(std::string str, std::vector<std::string>& vs, char cDelimiter)
{
   vs.clear();
   std::string::size_type pos;
   std::string strTmp;
   while (1)
      {
      pos = str.find_first_of(cDelimiter);
      if (pos == str.npos)
         strTmp = str;
      else
         strTmp = str.substr(0, pos);
      trim(strTmp, '"');
      if (!!strTmp.length())
         vs.push_back(strTmp);
      if (pos == str.npos)
         break;
      str = str.erase(0, pos+1);
      }
}
//------------------------------------------------------------------------------

//------------------------------------------------------------------------------
/// returns a 'value' from a vector of strings. Strings in vector are of type:
///   a=b
///   c=d
///   e=f
/// then GetValue(VECTOR, "c") returns "d"
//------------------------------------------------------------------------------
std::string GetValue(std::vector<std::string>& vs, const char* lpcszName)
{
   std::string strRet;
   std::string str = std::string(lpcszName) + "=";
   std::string::size_type pos;
   for (unsigned int i = 0; i < vs.size(); i++)
      {
      pos = vs[i].find(str);
      if (pos == 0)
         {
         strRet = vs[i].substr(pos+str.length());
         break;
         }
      }
   trim(strRet, '"');
   trim(strRet, '\n');
   return strRet;
}
//------------------------------------------------------------------------------

//------------------------------------------------------------------------------
/// sets a 'value' in a vector of strings. Strings in vector are of type:
///   a=b
///   c=d
///   e=f
/// then SetValue(VECTOR, "c", "x") results in
///   a=b
///   c=x
///   e=f
/// non-existing values are appended, so SetValue(VECTOR, "g", "h") results in
///   a=b
///   c=x
///   e=f
///   g=h
//------------------------------------------------------------------------------
void SetValue(std::vector<std::string>& vs, const char* lpcszName, const char* lpcszValue)
{
   std::string str = std::string(lpcszName) + "=";
   std::string::size_type pos;
   for (unsigned int i = 0; i < vs.size(); i++)
      {
      pos = vs[i].find(str);
      //if (pos != vs[i].npos)
      if (pos == 0)
         {
         vs[i] = str + lpcszValue;
         return;
         }
      }
   vs.push_back(std::string(str + lpcszValue));
}
//------------------------------------------------------------------------------

//------------------------------------------------------------------------------
/// Removes a 'value' from a vector of strings. Strings in vector are of type:
///   a=b
///   c=d
///   e=f
/// then RemoveValue(VECTOR, "c") results in
///   a=b
///   e=f
//------------------------------------------------------------------------------
void RemoveValue(std::vector<std::string>& vs, const char* lpcszName)
{
   std::string str = std::string(lpcszName) + "=";
   std::string::size_type pos;
   for (unsigned int i = 0; i < vs.size(); i++)
      {
      pos = vs[i].find(str);
      //if (pos != vs[i].npos)
      if (pos == 0)
         {
         vs.erase(vs.begin() + (int)i);
         return;
         }
      }
}
//------------------------------------------------------------------------------

//------------------------------------------------------------------------------
// below function only needed when compiling MEX files. When compiling it for
// SoundDllPro.dll then NOMEX must  be defined
//------------------------------------------------------------------------------
#ifndef NOMEX
//------------------------------------------------------------------------------
/// checks if a passed mxArray contains a scalar double value
//------------------------------------------------------------------------------
bool IsScalarDouble(const mxArray *phs)
{
   if (!mxIsDouble(phs))
      return false;
   if (mxGetM(phs) != 1 || mxGetN(phs) != 1)
      return false;
   return true;
}
//------------------------------------------------------------------------------

#define FreeNullArray(p) {if (p) mxDestroyArray(p);p=NULL;}
//------------------------------------------------------------------------------
/// searches for a MATLAB button and returns boundsrect
//------------------------------------------------------------------------------
void GetButtonWindowProperties(mxArray** mxa, RECT &rc, std::string &strCaption)
{
   //pointer to Result array
   mxArray *mxaRes = NULL;
   //array for command
   mxArray *rhs[3];
   rhs[0] = *mxa;
   rhs[1] = NULL;
   rhs[2] = NULL;
   try
      {
      //allocate strings for
      rhs[1] = mxCreateString("Units");
      rhs[2] = mxCreateString("pixels");
      //set Units property to pixels!
      int iReturn = mexCallMATLAB(0, NULL, 3, rhs, "set");
      if (iReturn!=0)
         throw SOUNDMEX_Error("Error retrieving button properties (2)");

      //'Units' and 'pixels' not needed any longer!
      FreeNullArray(rhs[1]);
      FreeNullArray(rhs[2]);
      //retrieve Position
      rhs[1] = mxCreateString("Position");
      iReturn = mexCallMATLAB(1, &mxaRes, 2, rhs, "get");
      if (iReturn!=0)
         throw SOUNDMEX_Error("Error retrieving button properties (3)");
      double *pr = mxGetPr(mxaRes);
      int num_elem = (int)mxGetNumberOfElements(mxaRes);
      if (num_elem != 4)
         throw SOUNDMEX_Error("field 'Position' does not have 4 dimensions!");
      // wirte values to rc. in MATLAB fashion:
      // - left:     'real' left position
      // - top:      counted from bottom
      // - right:    contains width
      // - bottom:   contains height
      rc.left     = (int)floor(pr[0]);
      rc.top      = (int)floor(pr[1]);
      rc.right    = (int)floor(pr[2]);
      rc.bottom   = (int)floor(pr[3]);
      FreeNullArray(rhs[1]);
      FreeNullArray(mxaRes);
      //retrieve Parent
      mxArray *mxaParent = NULL;
      rhs[1] = mxCreateString("Parent");
      iReturn = mexCallMATLAB(1, &mxaParent, 2, rhs, "get");
      if (iReturn!=0)
         throw SOUNDMEX_Error("Error retrieving button properties (4)");
      FreeNullArray(rhs[1]);
      // retrieve 'Name' property of parent as windowcaption
      rhs[0] = mxaParent;
      rhs[1] = mxCreateString("Name");
      iReturn = mexCallMATLAB(1, &mxaRes, 2, rhs, "get");
      if (iReturn!=0)
         throw SOUNDMEX_Error("Error retrieving button properties (5)");
      if (!mxIsChar(mxaRes))
         throw SOUNDMEX_Error("field 'Name' does not contain a string!");
      // remember old units
      char *c = mxArrayToString(mxaRes);
      strCaption  = c;
      mxFree(c);
      FreeNullArray(mxaRes);
      FreeNullArray(mxaParent);
      FreeNullArray(rhs[1]);
      }
   catch (...)
      {
      FreeNullArray(rhs[1]);
      FreeNullArray(rhs[2]);
      FreeNullArray(mxaRes);
      throw;
      }
    FreeNullArray(rhs[1]);
    FreeNullArray(rhs[2]);
    FreeNullArray(mxaRes);
}
//------------------------------------------------------------------------------
#endif
