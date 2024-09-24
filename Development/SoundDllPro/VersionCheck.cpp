//------------------------------------------------------------------------------
/// \file VersionCheck.cpp
/// \author Berg
/// \brief Implementation of a downloader and parser of version history for
/// AudioSpike 
///
/// Project AudioSpike
/// Module  AudioSpike.exe
///
///
/// ****************************************************************************
/// Copyright 2023 Daniel Berg, Oldenburg, Germany
/// ****************************************************************************
///
/// This file is part of AudioSpike.
///
///    AudioSpike is free software: you can redistribute it and/or modify
///    it under the terms of the GNU General Public License as published by
///    the Free Software Foundation, either version 3 of the License, or
///    (at your option) any later version.
///
///    AudioSpike is distributed in the hope that it will be useful,
///    but WITHOUT ANY WARRANTY; without even the implied warranty of
///    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
///    GNU General Public License for more details.
///
///    You should have received a copy of the GNU General Public License
///    along with AudioSpike.  If not, see <http:///www.gnu.org/licenses/>.
///
//------------------------------------------------------------------------------
#pragma hdrstop

#include "VersionCheck.h"
#include <System.SysUtils.hpp>
#include <IdBaseComponent.hpp>
#include <IdComponent.hpp>
#include <IdHTTP.hpp>
#include <IdTCPClient.hpp>
#include <IdTCPConnection.hpp>
#include <IdIOHandler.hpp>
#include <IdIOHandlerSocket.hpp>
#include <IdIOHandlerStack.hpp>
#include <IdSSL.hpp>
#include <IdSSLOpenSSL.hpp>
//------------------------------------------------------------------------------
#pragma package(smart_init)
//------------------------------------------------------------------------------

//------------------------------------------------------------------------------
/// constructor. Creates members
//------------------------------------------------------------------------------
TVersionChecker::TVersionChecker()
{
   // fix version token: a line from version history starting with this token 
   /// starts one version history entry
   m_usVersionToken  = "Version ";
   m_pslHistory      = new TStringList();
}
//------------------------------------------------------------------------------

//------------------------------------------------------------------------------
/// destructor does cleanup
//------------------------------------------------------------------------------
TVersionChecker::~TVersionChecker()
{
   delete m_pslHistory;
}
//------------------------------------------------------------------------------

//------------------------------------------------------------------------------
/// reads version history from a file (for devolpment only)
//------------------------------------------------------------------------------
bool TVersionChecker::ReadVersionHistoryFile(UnicodeString usFileName)
{
   m_usVersionLatest = "";
   try
      {
      m_pslHistory->LoadFromFile(usFileName);
      // valid, if at least 'latest' version can be read
      m_usVersionLatest = GetVersionFromLine(0);
      }
   catch(...)
      {
      }
   return m_usVersionLatest.Length(); 
}
//------------------------------------------------------------------------------

//------------------------------------------------------------------------------
/// reads version history from URL using SSL
//------------------------------------------------------------------------------
bool TVersionChecker::ReadVersionHistoryURL(UnicodeString usURL)
{
   m_usVersionLatest = "";
   try
      {
      TIdHTTP *pHTTP = NULL;
      try
         {
         pHTTP = new TIdHTTP(NULL);   
         pHTTP->IOHandler = new TIdSSLIOHandlerSocketOpenSSL(pHTTP);
         ((TIdSSLIOHandlerSocketOpenSSL*)pHTTP->IOHandler)->SSLOptions->Method = sslvSSLv23;
         ((TIdSSLIOHandlerSocketOpenSSL*)pHTTP->IOHandler)->SSLOptions->Mode = sslmUnassigned;
         pHTTP->HandleRedirects = true;
         m_pslHistory->Text = pHTTP->Get(usURL);
         // valid, if at least 'latest' version can be read
         m_usVersionLatest = GetVersionFromLine(0);
         }
      __finally
         {
         if (pHTTP)
            delete pHTTP;
         }
      }
   catch(...)
      {
      }
   return m_usVersionLatest.Length(); 
}
//------------------------------------------------------------------------------


//------------------------------------------------------------------------------
/// \retval 1 if passed version is identical or newer than latest
/// \retval 0 if passed version is lower than latest 
/// \retval -1 if version hsitory is not valid 
//------------------------------------------------------------------------------
int TVersionChecker::VersionIsLatest(UnicodeString usVersion)
{
   if (!m_usVersionLatest.Length())
      return -1; 
   return CompareVersions(GetVersionFromLine(0), usVersion) <= 0;
}
//------------------------------------------------------------------------------

//------------------------------------------------------------------------------
/// returns version string from passed string or mpty string, if passe string 
/// does not contain version info
//------------------------------------------------------------------------------
UnicodeString TVersionChecker::GetVersionFromLine(UnicodeString us)
{  
   int nTokenLen = m_usVersionToken.Length();
   UnicodeString usReturn;
   int nPosV = us.Pos(m_usVersionToken);
   int nPosC = us.Pos(",");
   if (nPosV == 1 && nPosC > nTokenLen)
      {
      usReturn = us.SubString(nTokenLen+1, nPosC-nTokenLen-1);
      
      }

   return usReturn;
}
//------------------------------------------------------------------------------

//------------------------------------------------------------------------------
/// returns version string from passed line number or empty string, if no version 
/// contained in corresponding string from version history
//------------------------------------------------------------------------------
UnicodeString TVersionChecker::GetVersionFromLine(int nLine)
{
   if (nLine >=  m_pslHistory->Count)
      return "";
   return  GetVersionFromLine(m_pslHistory->Strings[nLine]);
}
//------------------------------------------------------------------------------

//------------------------------------------------------------------------------
///
//------------------------------------------------------------------------------
UnicodeString TVersionChecker::GetVersionLatest(void)
{
   return m_usVersionLatest;
}
//------------------------------------------------------------------------------

//------------------------------------------------------------------------------
/// returns version history back to passed version number as string
//------------------------------------------------------------------------------
UnicodeString TVersionChecker::GetVersionHistoryToVersion(UnicodeString usVersion)
{
   UnicodeString usReturn;
   TStringList *psl = new TStringList();
   try
      {
      int n;
      UnicodeString us, usVer;
      for(n = 0; n < m_pslHistory->Count; n++)
         {
         us = m_pslHistory->Strings[n];
         usVer = GetVersionFromLine(us);
         if (usVer.Length())
            {
            if (CompareVersions(usVer, usVersion) <= 0)
               break;
            if (psl->Count)
               psl->Add(" ");
            }
         if (us.Pos("<ul>") == 0 && us.Pos("</ul>") == 0)
            {

            us = StringReplace(us, "<li>", " --", TReplaceFlags() << rfReplaceAll);
            us = StringReplace(us, "</li>", "", TReplaceFlags() << rfReplaceAll);
            psl->Add(us);
            }
         
         }
      usReturn = psl->Text;
      }
   __finally
      {
      delete psl;
      }
   return usReturn;
}
//------------------------------------------------------------------------------

//------------------------------------------------------------------------------
/// compares two version numbers
/// \retval 1 if passed version 1 newer than passed version 2 
/// \retval 0 if passed versions are identical
/// \retval -1 if passed version 2 newer than passed version 1
//------------------------------------------------------------------------------
int TVersionChecker::CompareVersions(UnicodeString us1, UnicodeString us2)
{
   int nMaj1, nMin1, nRev1, nBuild1, nMaj2, nMin2, nRev2, nBuild2;

   if (4 != swscanf(us1.w_str(), L"%d.%d.%d.%d", &nMaj1, &nMin1, &nRev1, &nBuild1))
      throw Exception("invalid version string 1 passed");
   if (4 != swscanf(us2.w_str(), L"%d.%d.%d.%d", &nMaj2, &nMin2, &nRev2, &nBuild2))
      throw Exception("invalid version string 2 passed");

   if (nMaj1 > nMaj2)
      return 1;
   else if (nMaj1 < nMaj2)
      return -1;
   if (nMin1 > nMin2)
      return 1;
   else if (nMin1 < nMin2)
      return -1;
   if (nRev1 > nRev2)
      return 1;
   else if (nRev1 < nRev2)
      return -1;

   return 0;   
}
//------------------------------------------------------------------------------

