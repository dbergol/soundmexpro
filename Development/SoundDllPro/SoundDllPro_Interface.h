//------------------------------------------------------------------------------
/// \file SoundDllPro_Interface.cpp
/// \author Berg
/// Main interface file for SoundDllPro implementing exported function StrCommand
/// and all functions called by StrCommand. It includes sounddllpro_cmd.h that
/// defines the command buffer containing all commands, help, syntax... 
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
#ifndef SoundDllPro_InterfaceH
#define SoundDllPro_InterfaceH
//---------------------------------------------------------------------------
#pragma warn -use
#include "SimpleMidi.h"

extern bool g_bUseRamps;
extern TSimpleMidi* g_pMidi;

//------------------------------------------------------------------------------
/// definition of command buffer function type
//------------------------------------------------------------------------------
typedef void (*LPFNSNDDLLFUNC)(TStringList* sl);
//------------------------------------------------------------------------------
//------------------------------------------------------------------------------
/// Exported string parser function to be called by SOUNDMEXPRO.DLL
//------------------------------------------------------------------------------
extern "C" {
   __declspec(dllexport) int cdecl SoundDllProCommand(const char* lpszCommand,
                                                      char* lpszReturnValue,
                                                      int nLength
                                                      );
}
//------------------------------------------------------------------------------

//------------------------------------------------------------------------------
/// prototypes of all (external) SoundMexPro commands 
//------------------------------------------------------------------------------
void   About(TStringList *psl);
void   SetDriverModel(TStringList *psl);
void   GetDriverModel(TStringList *psl);
void   Init(TStringList *psl);
void   Initialized(TStringList *psl);
void   Exit(TStringList *psl);
void   TrackMap(TStringList *psl);
void   TrackMode(TStringList *psl);
void   TrackLen(TStringList *psl);
void   Version(TStringList *psl);
void   CheckUpdate(TStringList *psl);
void   License(TStringList *psl);
void   Show(TStringList *psl);
void   Hide(TStringList *psl);
void   ShowTracks(TStringList *psl);
void   HideTracks(TStringList *psl);
void   UpdateTracks(TStringList *psl);
void   ShowMixer(TStringList *psl);
void   HideMixer(TStringList *psl);
void   ShowControlPanel(TStringList *psl);
void   GetDrivers(TStringList *psl);
void   GetDriverStatus(TStringList *psl);
void   GetChannels(TStringList *psl);
void   GetActiveDriver(TStringList *psl);
void   GetActiveChannels(TStringList *psl);
void   GetProperties(TStringList *psl);
void   Start(TStringList *psl);
void   Started(TStringList *psl);
void   StartThreshold(TStringList *psl);
void   Stop(TStringList *psl);
void   Pause(TStringList *psl);
void   Wait(TStringList *psl);
void   Mute(TStringList *psl);
void   ChannelMuteSolo(TStringList *psl);
void   ChannelName(TStringList *psl);
void   PlayPosition(TStringList *psl);
void   LoadPosition(TStringList *psl);
void   RecVolume(TStringList *psl);
void   RecPosition(TStringList *psl);
void   RecGetData(TStringList *psl);
void   RecBufferSize(TStringList *psl);
void   RecThreshold(TStringList *psl);
void   RecStarted(TStringList *psl);
void   RecLength(TStringList *psl);
void   RecFileName(TStringList *psl);
void   RecPause(TStringList *psl);
void   Volume(TStringList *psl);
void   TrackVolume(TStringList *psl);
void   LoadMem(TStringList *psl);
void   LoadFile(TStringList *psl);
void   ClearData(TStringList *psl);
void   ClearTrack(TStringList *psl);
void   Playing(TStringList *psl);
void   Recording(TStringList *psl);
void   NumXRuns(TStringList *psl);
void   ClipThreshold(TStringList *psl);
void   ClipCount(TStringList *psl);
void   ResetClipCount(TStringList *psl);
void   Underrun(TStringList *psl);
void   SetButton(TStringList *psl);
void   TrackLoad(TStringList *psl);
void   DebugSave(TStringList *psl);
void   DebugFileName(TStringList *psl);
void   PluginSetData(TStringList *psl);
void   PluginGetData(TStringList *psl);
void   IOStatus(TStringList *psl);
void   ResetError(TStringList *psl);
void   AsyncError(TStringList *psl);
void   VSTQuery(TStringList *psl);
void   VSTLoad(TStringList *psl);
void   VSTUnload(TStringList *psl);
void   VSTProgram(TStringList *psl);
void   VSTProgramName(TStringList *psl);
void   VSTParameter(TStringList *psl);
void   VSTSet(TStringList *psl);
void   VSTStore(TStringList *psl);
void   VSTEdit(TStringList *psl);
void   DspLoad(TStringList *psl);
void   DspLoadReset(TStringList *psl);
void   AsioDirectMonitoring(TStringList *psl);
void   MIDIInit(TStringList *psl);
void   MIDIExit(TStringList *psl);
void   MIDIGetDrivers(TStringList *psl);
void   MIDIPlayNote(TStringList *psl);
void   MIDIShortMsg(TStringList *psl);
void   BetaTest(TStringList *psl);
//------------------------------------------------------------------------------
#endif

