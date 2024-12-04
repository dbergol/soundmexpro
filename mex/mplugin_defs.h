//------------------------------------------------------------------------------
/// \file MPlugin_defs.h
/// \author Berg
/// \brief Shared definitions for IPC with TMPlugin class
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
#ifndef MPluginDefsH
#define MPluginDefsH
//------------------------------------------------------------------------------

/// definition of user data length per channel
#define USERDATA_LENGTH          100

//------------------------------------------------------------------------------
/// enum as list of events used for interprocess synchronisation.
/// NOTE: never change order of 'InterProcessCommunication'-events here!!!
//------------------------------------------------------------------------------
enum
   {
   IPC_EVENT_EXIT   = 0,
   IPC_EVENT_PROCESS,
   IPC_EVENT_ERROR,
   IPC_EVENT_DONE,
   IPC_EVENT_INIT,
   IPC_EVENT_TERMINATED,
   IPC_EVENT_LAST
   };
//------------------------------------------------------------------------------
#endif
