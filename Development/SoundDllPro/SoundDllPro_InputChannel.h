//------------------------------------------------------------------------------
/// \file SoundDllPro_InputChannel.cpp
/// \author Berg
/// \brief Implementation of class SDPInput describing an input channel. Includes
/// a ringbuffer for cyclic storing of recorded data in memory and storing of
/// recorded data to file. Used by SoundDllProMain class
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
#ifndef SoundDllPro_InputChannelH
#define SoundDllPro_InputChannelH
//------------------------------------------------------------------------------
#include <vcl.h>
#include <vector>
#include <valarray>

//------------------------------------------------------------------------------
class SDPRecFile;
//------------------------------------------------------------------------------

//------------------------------------------------------------------------------
/// \class SDPInput. Input channel class.
//------------------------------------------------------------------------------
class SDPInput
{
   public:
      SDPInput(unsigned int nChannelIndex, bool bDisableRecFile, unsigned int nDownSampleFactor);
      ~SDPInput();
      void                    Start();
      void                    Stop();
      void                    Pause(bool bDoPause, bool bCloseFileOnPause = false);
      bool                    Pause();
      bool                    IsRecording();
      void                    SaveBuffer(std::valarray<float> &vaf, bool bSaveToFile = true);
      int64_t                 RecPosition();
      bool                    Started();
      void                    SaveToFile(bool bEnable);
      bool                    SaveToFile();
      std::valarray<float>&   GetBuffer();
      void                    BufferSize(unsigned int nBufSize);
      unsigned int            BufferSize(void);
      void                    RecordLength(unsigned int nRecordLength);
      unsigned int            RecordLength(void);
      void                    FileName(AnsiString sFileName);
      AnsiString              FileName(void);
      AnsiString              Name(void);
      void                    Name(AnsiString strName);
   private:
      bool                    m_bSaveToFile;    ///< flag if saving to file is enabled
      unsigned int            m_nChannelIndex;  ///< channel index
      AnsiString              m_strName;        ///< symbolic name
      SDPRecFile*             m_prfRecordFile;  ///< instance of record file
      unsigned int            m_nBufPos;        ///< internal buffer position
      bool                    m_bStarted;       ///< flag if recording is started
      std::vector<std::valarray<float> > m_vvafBuffer;  ///< internal buffer
};
//------------------------------------------------------------------------------

#endif
