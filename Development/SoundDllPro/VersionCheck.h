//------------------------------------------------------------------------------
/// \file VersionCheck.h
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
#ifndef VersionCheckH
#define VersionCheckH

#include <vcl.h>
//------------------------------------------------------------------------------
/// class for downloading and parsing version history file from AudioSpike homepage
//------------------------------------------------------------------------------
class TVersionChecker
{
   public:
      TVersionChecker();
      ~TVersionChecker();
      bool           ReadVersionHistoryFile(UnicodeString usFileName);
      bool           ReadVersionHistoryURL(UnicodeString usURL);
      int            VersionIsLatest(UnicodeString usVersion);
      UnicodeString  GetVersionLatest(void);
      UnicodeString  GetVersionHistoryToVersion(UnicodeString usVersion);
   private:
      UnicodeString  m_usVersionToken;    ///< internal variable holding version token
      TStringList*   m_pslHistory;        ///< internal stringlist holding complete version history
      UnicodeString  m_usVersionLatest;   ///< string holding latest version
      UnicodeString  GetVersionFromLine(UnicodeString us);
      UnicodeString  GetVersionFromLine(int nLine);
      int            CompareVersions(UnicodeString us1, UnicodeString us2);      
};
//------------------------------------------------------------------------------
#endif
