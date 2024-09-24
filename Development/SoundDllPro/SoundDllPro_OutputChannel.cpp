//------------------------------------------------------------------------------
/// \file SoundDllPro_OutputChannel.cpp
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
#include <vcl.h>
#pragma hdrstop

#include "SoundDllPro_OutputChannel.h"
#include "SoundDllPro_RecFile.h"
#include "SoundDllPro_Tools.h"
//------------------------------------------------------------------------------

//------------------------------------------------------------------------------
/// constructor. Intialises debug save file
//------------------------------------------------------------------------------
SDPOutput::SDPOutput(unsigned int nChannelIndex, double dSampleRate)
   :  m_bDebugSave(false),
      m_nChannelIndex(nChannelIndex),
      m_prfDebugSave(NULL)
{
   m_strName = "Output " + IntToStr((int)nChannelIndex);
   m_strDebugSaveFileName = "out_" + IntToStr((int)nChannelIndex) + ".wav";
   
   m_prfDebugSave = new SDPRecFile((unsigned int)dSampleRate, (int)m_nChannelIndex, 0, false);
   DebugSave(m_bDebugSave);
}
//------------------------------------------------------------------------------

//------------------------------------------------------------------------------
/// calls stop and does cleanup
//------------------------------------------------------------------------------
SDPOutput::~SDPOutput()
{
   Stop();
   TRYDELETENULL(m_prfDebugSave);
}
//------------------------------------------------------------------------------

//------------------------------------------------------------------------------
/// Starts debug-saving
//------------------------------------------------------------------------------
void  SDPOutput::Start()
{
   DebugSave(m_bDebugSave);
   if (DebugSave())
      m_prfDebugSave->OpenFile(m_strDebugSaveFileName);
}
//------------------------------------------------------------------------------

//------------------------------------------------------------------------------
/// Stops debug-saving
//------------------------------------------------------------------------------
void  SDPOutput::Stop()
{
   DebugSave(false);
   m_prfDebugSave->CloseFile();
}
//------------------------------------------------------------------------------

//------------------------------------------------------------------------------
/// sets debug-saving status
//------------------------------------------------------------------------------
void  SDPOutput::DebugSave(bool bEnable)
{
   m_bDebugSave = bEnable;
   m_prfDebugSave->Enabled(bEnable);
}
//------------------------------------------------------------------------------

//------------------------------------------------------------------------------
/// returns current debug-saving status
//------------------------------------------------------------------------------
bool SDPOutput::DebugSave()
{
   return m_bDebugSave;
}
//------------------------------------------------------------------------------

//------------------------------------------------------------------------------
/// Saves a buffer to file if debug-saving enabled
//------------------------------------------------------------------------------
void SDPOutput::SaveBuffer(std::valarray<float> &vaf)
{
   if (!DebugSave())
      return;
   m_prfDebugSave->WriteBuffer(vaf);
}
//------------------------------------------------------------------------------

//------------------------------------------------------------------------------
/// sets filename for debug/file2file saving
//------------------------------------------------------------------------------
void  SDPOutput::DebugFileName(AnsiString strDebugSaveFileName)
{
   if (m_prfDebugSave->IsOpen())
      throw Exception("cannot set filename while file is open");
   m_strDebugSaveFileName = strDebugSaveFileName;
}
//------------------------------------------------------------------------------

//------------------------------------------------------------------------------
/// returns debug filename
//------------------------------------------------------------------------------
AnsiString SDPOutput::DebugFileName()
{
   return m_strDebugSaveFileName;
}
//------------------------------------------------------------------------------

//------------------------------------------------------------------------------
/// returns output name
//------------------------------------------------------------------------------
AnsiString SDPOutput::Name(void)
{
   return m_strName;
}
//------------------------------------------------------------------------------

//------------------------------------------------------------------------------
/// sets output name
//------------------------------------------------------------------------------
void SDPOutput::Name(AnsiString strName)
{
   m_strName = strName;
}
//------------------------------------------------------------------------------


