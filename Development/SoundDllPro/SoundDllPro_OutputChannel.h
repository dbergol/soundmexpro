//------------------------------------------------------------------------------
/// \file SoundDllPro_OutputChannel.h
/// \author Berg
/// \brief Implementation of class SDPOutput describing an output channel.
/// Includes storing of data to file (debugging). Used by SoundDllProMain class
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
#ifndef SoundDllPro_OutputChannelH
#define SoundDllPro_OutputChannelH
//------------------------------------------------------------------------------
#include <vector>
#include <valarray>
#include "soundmexpro_defs.h"
//------------------------------------------------------------------------------
class SDPRecFile;
//------------------------------------------------------------------------------

//------------------------------------------------------------------------------
/// \class SDPOutput describing an output channel. 
//------------------------------------------------------------------------------
class SDPOutput
{
   public:
      SDPOutput(unsigned int nChannelIndex, double dSampleRate);
      ~SDPOutput();
      void        Start();
      void        Stop();
      void        DebugSave(bool bEnable);
      bool        DebugSave();
      void        SaveBuffer(std::valarray<float> &vaf);
      void        DebugFileName(AnsiString strDebugSaveFileName);
      AnsiString  DebugFileName();
      AnsiString  Name(void);
      void        Name(AnsiString strName);
   private:
      bool           m_bDebugSave;
      unsigned int   m_nChannelIndex;
      AnsiString     m_strName;
      SDPRecFile*    m_prfDebugSave;
      AnsiString     m_strDebugSaveFileName;
};
//------------------------------------------------------------------------------

#endif
