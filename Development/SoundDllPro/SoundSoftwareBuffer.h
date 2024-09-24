//------------------------------------------------------------------------------
/// \file SoundSoftwareBuffer.h
///
/// \author Berg
/// \brief Interface of class TSoundSoftwareBuffer
///
/// Project SoundMexPro
/// Module  SoundDllPro.dll
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
#ifndef SoundSoftwareBufferH
#define SoundSoftwareBufferH
//---------------------------------------------------------------------------
#include <Classes.hpp>
#include "SoundDllPro_SoundClassBase.h"
//---------------------------------------------------------------------------
class  SoundClassMMDevice;
//---------------------------------------------------------------------------
/// class for software buffering in sound class
//---------------------------------------------------------------------------
class TSoundSoftwareBuffer : public TThread
{
   friend class UNIT_TEST_CLASS;
   private:
      SoundClassMMDevice*	m_pHtSound;       ///< instance of owning SoundClassMMDevice instance
      unsigned int      m_nNumBuffers;       ///< number of buffers
      unsigned int      m_nChannels;         ///< number of channels per buffer
      unsigned int      m_nBufsize;          ///< number of samples per channel
      volatile long     m_nReadIndex;        ///< current reading buffer index
      volatile long     m_nWriteIndex;       ///< current writing buffer index
      bool              m_bProcessingError;  ///< flag if a processing error occurred
      std::vector<vvf > m_vvvfSoftwareBuffer;///< internal sound data buffer
      HANDLE            m_hWriteHandle;      ///< event handle for waking up thread that fills the buffers
   protected:
      void __fastcall Execute();
   public:
      __fastcall  TSoundSoftwareBuffer(SoundClassMMDevice*   pHtSound,
                                       int         nNumBuffers,
                                       int         nChannels,
                                       int         nBufsize);
      __fastcall  ~TSoundSoftwareBuffer();
      bool        GetBuffer(vvf& vvfBuffer);
};
//---------------------------------------------------------------------------
#endif  // SoundSoftwareBufferH
