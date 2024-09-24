//------------------------------------------------------------------------------
/// \file SimpleMidi.h
/// \author Berg
/// \brief Implementation of class TSimpleMidi. support for simple MIDI
/// command support.
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
#ifndef SimpleMidiH
#define SimpleMidiH

#include <vcl.h>
#include <mmsystem.h>

//------------------------------------------------------------------------------
/// class for handling simple MIDI commands
//------------------------------------------------------------------------------
class TSimpleMidi
{
   public:
      TSimpleMidi(UINT uDeviceId);
      ~TSimpleMidi();
      void        PlayNote(BYTE bNote, BYTE bVolume, BYTE bChannel = 0);
      void        OutShortMsg(BYTE bStatusByte, BYTE bMIDIByte1, BYTE bMIDIByte2);
      static UINT GetNumDevs();
      static      AnsiString GetDevName(UINT uDeviceId);
      static void GetDevCaps(UINT uDeviceId, MIDIOUTCAPS &moc);
   private:
	  HMIDIOUT    m_hmoDevice;      ///< handle to midi out device
	  MIDIOUTCAPS m_mocDevCaps;     ///< midi out device capabilities
	  UINT        m_uDeviceId;      ///< midi out device id
};

//---------------------------------------------------------------------------
#endif
