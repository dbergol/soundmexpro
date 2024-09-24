//------------------------------------------------------------------------------
/// \file SimpleMidi.cpp
/// \author Berg
/// \brief Implementation of class TSimpleMidi. Simple MIDI
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
#pragma hdrstop

#include "SimpleMidi.h"


#pragma package(smart_init)
//------------------------------------------------------------------------------

//------------------------------------------------------------------------------
/// Constructor. Retrieves device capabilitiess and opens it
/// \param[in] uDeviceId MIDI device id
/// \exception if device id out of range, device query or opening fails
//------------------------------------------------------------------------------
TSimpleMidi::TSimpleMidi(UINT uDeviceId)
   :  m_hmoDevice(NULL),
      m_uDeviceId(0)
{
   GetDevCaps(uDeviceId, m_mocDevCaps);

   if (MMSYSERR_NOERROR != midiOutOpen(&m_hmoDevice, uDeviceId, 0, 0, CALLBACK_NULL))
      throw Exception("Error opening MIDI device");
   m_uDeviceId = uDeviceId;
}

//------------------------------------------------------------------------------

//------------------------------------------------------------------------------
/// Destructor. Closes device
//------------------------------------------------------------------------------
TSimpleMidi::~TSimpleMidi()
{
   if (!!m_hmoDevice)
      midiOutClose(m_hmoDevice);
   m_hmoDevice = NULL;
}
//------------------------------------------------------------------------------

//------------------------------------------------------------------------------
/// Plays a note on device.
/// \param[in] bNote note to play
/// \param[in] bVolume volume
/// \param[in] bChannel MIDI channel
/// \exception Exception on any error
//------------------------------------------------------------------------------
void TSimpleMidi::PlayNote(BYTE bNote, BYTE bVolume, BYTE bChannel)
{
   if (bNote > 127)
      throw Exception("note out of range");
   if (bVolume > 127)
      throw Exception("volume out of range");
   if (bChannel > 15)
      throw Exception("volume out of range");

   // 0x90 is note on, add the channel
   OutShortMsg((BYTE)(0x90 | bChannel), bNote, bVolume);
   // 0x80 is note off, add the channel
   OutShortMsg((BYTE)(0x80 | bChannel), bNote, 0);
}
//------------------------------------------------------------------------------

//------------------------------------------------------------------------------
/// Sends a short MIDI message to device
/// \param[in] bStatusByte status byte to send (first byte)
/// \param[in] bMIDIByte1  first midi byte to send (second byte)
/// \param[in] bMIDIByte2  first midi byte to send (third byte)
//------------------------------------------------------------------------------
void TSimpleMidi::OutShortMsg(BYTE bStatusByte, BYTE bMIDIByte1, BYTE bMIDIByte2)
{
   if (MMSYSERR_NOERROR != midiOutShortMsg(m_hmoDevice, (DWORD)((bMIDIByte2 << 16) | (bMIDIByte1 << 8) | bStatusByte)))
      throw Exception("Error sending short message to MIDI device");
}
//------------------------------------------------------------------------------

//------------------------------------------------------------------------------
/// Returns number of installed MIDI devices
/// \retval number of installed MIDI devices
//------------------------------------------------------------------------------
UINT TSimpleMidi::GetNumDevs()
{
   return midiOutGetNumDevs();
}
//------------------------------------------------------------------------------

//------------------------------------------------------------------------------
/// Returns name of a MIDI device
/// \param[in] uDeviceId MIDI device id
/// \exception if device id out of range, device query fails
//------------------------------------------------------------------------------
AnsiString TSimpleMidi::GetDevName(UINT uDeviceId)
{
   MIDIOUTCAPS moc;
   GetDevCaps(uDeviceId, moc);
   return AnsiString(moc.szPname);
}
//------------------------------------------------------------------------------

//------------------------------------------------------------------------------
/// Retrievs capabilities of a MIDI-out device 
/// \param[in] uDeviceId MIDI device id
/// \param[in] rmoc reference to MIDIOUTCAPS structure to be filled
/// \exception if device id out of range, device query fails
//------------------------------------------------------------------------------
void TSimpleMidi::GetDevCaps(UINT uDeviceId, MIDIOUTCAPS &rmoc)
{
   if (uDeviceId >= midiOutGetNumDevs())
      throw Exception("MIDI DeviceId out of range");
   ZeroMemory(&rmoc, sizeof(MIDIOUTCAPS));
   if (MMSYSERR_NOERROR != midiOutGetDevCaps(uDeviceId, &rmoc, sizeof(MIDIOUTCAPS)))
      throw Exception("Error retrieving MIDI caps");
}
//------------------------------------------------------------------------------

