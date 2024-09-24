//------------------------------------------------------------------------------
/// \file AHtVSTConv.h
/// \author Berg
/// \brief Implementation of class CWaveFileReader for reading normalized 32bit PCM wave files.
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
#ifndef WaveFileReaderH
#define WaveFileReaderH
//------------------------------------------------------------------------------

#include <windows.h>
//------------------------------------------------------------------------------

//------------------------------------------------------------------------------
/// CWaveFileReader class for reading  PCM wavefile
//------------------------------------------------------------------------------
class CWaveFileReader
{
   public:
      CWaveFileReader();
      CWaveFileReader(LPSTR lpszFileName);
      ~CWaveFileReader();
      void Load(LPSTR lpszFileName);

      float* m_lpData[2];
      unsigned int m_nSize;
      bool         m_bLoaded;
   private:
      void Cleanup();
};
//------------------------------------------------------------------------------
#endif
