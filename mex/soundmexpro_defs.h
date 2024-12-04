//------------------------------------------------------------------------------
/// \file SoundDllPro.h
/// \author Berg
/// Contains global definitions for SoundMexPro and SoundDllPro
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
#ifndef SoundMexProH
#define SoundMexProH
//------------------------------------------------------------------------------

#include <limits.h>

//------------------------------------------------------------------------------
/// main DLL-Funktion: string parser
//------------------------------------------------------------------------------
typedef int    (cdecl *LPFNSOUNDDLLPROCOMMAND)(const char*, char*, int);
//------------------------------------------------------------------------------

//------------------------------------------------------------------------------
/// name of exported function
//------------------------------------------------------------------------------
#define SOUNDDLL_COMMANDNAME           "SoundDllProCommand"
//------------------------------------------------------------------------------

//------------------------------------------------------------------------------
/// enum for SoundMexPro return values. Most values not really used yet...
//------------------------------------------------------------------------------
enum  {
      SOUNDDLL_RETURN_WARNING = -3,
      SOUNDDLL_RETURN_ERROR,
      SOUNDDLL_RETURN_MEXERROR,
      SOUNDDLL_RETURN_BUSY,
      SOUNDDLL_RETURN_OK
      };
//------------------------------------------------------------------------------


//------------------------------------------------------------------------------
/// string definitions
//------------------------------------------------------------------------------
#define SOUNDDLLPRO_STR_COMMAND        "command"
#define SOUNDDLLPRO_STR_Type           "Type"
#define SOUNDDLLPRO_STR_Mser           "MSer"
#define SOUNDDLLPRO_STR_Version        "Version"
#define SOUNDDLLPRO_STR_AUD            "AUD"
#define SOUNDDLLPRO_STR_SPW            "SPW"
#define SOUNDDLLPRO_STR_ACALES         "ACALES"
#define SOUNDDLLPRO_STR_Name           "Name"
#define SOUNDDLLPRO_STR_Update         "Update"
#define SOUNDDLLPRO_STR_Settings       "Settings"
#define SOUNDDLLPRO_STR_Program        "Program"
#define SOUNDDLLPRO_STR_Parameter      "Parameter"
#define SOUNDDLLPRO_STR_Licfile        "Licfile"
#define SOUNDDLLPRO_STR_Update         "Update"
//------------------------------------------------------------------------------

//------------------------------------------------------------------------------
/// command definitions
//------------------------------------------------------------------------------

#define SOUNDDLLPRO_CMD_HELP           "help"
#define SOUNDDLLPRO_CMD_HELPA          "helpa"
#define SOUNDDLLPRO_CMD_HELPCMD        "helpcommand"
#define SOUNDDLLPRO_CMD_ABOUT          "about"
#define SOUNDDLLPRO_CMD_SETDRIVERMODEL "setdrivermodel"
#define SOUNDDLLPRO_CMD_GETDRIVERMODEL "getdrivermodel"
#define SOUNDDLLPRO_CMD_INIT           "init"
#define SOUNDDLLPRO_CMD_INITIALIZED    "initialized"
#define SOUNDDLLPRO_CMD_EXIT           "exit"
#define SOUNDDLLPRO_CMD_TRACKMAP       "trackmap"
#define SOUNDDLLPRO_CMD_TRACKMODE      "trackmode"
#define SOUNDDLLPRO_CMD_TRACKLEN       "tracklen"
#define SOUNDDLLPRO_CMD_GETDRV         "getdrivers"
#define SOUNDDLLPRO_CMD_GETDRVSTATUS   "getdriverstatus"
#define SOUNDDLLPRO_CMD_GETDRV_ACTIVE  "getactivedriver"
#define SOUNDDLLPRO_CMD_GETCH          "getchannels"
#define SOUNDDLLPRO_CMD_GETCH_ACTIVE   "getactivechannels"
#define SOUNDDLLPRO_CMD_GETPROP        "getproperties"
#define SOUNDDLLPRO_CMD_VERSION        "version"
#define SOUNDDLLPRO_CMD_LICENSE        "license"
#define SOUNDDLLPRO_CMD_START          "start"
#define SOUNDDLLPRO_CMD_STARTTHRSHLD   "startthreshold"
#define SOUNDDLLPRO_CMD_STOP           "stop"
#define SOUNDDLLPRO_CMD_STARTED        "started"
#define SOUNDDLLPRO_CMD_SHOW           "show"
#define SOUNDDLLPRO_CMD_HIDE           "hide"
#define SOUNDDLLPRO_CMD_SHOWTRACKS     "showtracks"
#define SOUNDDLLPRO_CMD_HIDETRACKS     "hidetracks"
#define SOUNDDLLPRO_CMD_UPDATETRACKS   "updatetracks"
#define SOUNDDLLPRO_CMD_SHOWMIXER      "showmixer"
#define SOUNDDLLPRO_CMD_HIDEMIXER      "hidemixer"
#define SOUNDDLLPRO_CMD_CONTROLPANEL   "controlpanel"
#define SOUNDDLLPRO_CMD_LOADMEM        "loadmem"
#define SOUNDDLLPRO_CMD_LOADFILE       "loadfile"
#define SOUNDDLLPRO_CMD_SHOWERROR      "showerror"
#define SOUNDDLLPRO_CMD_ERROR          "error"
#define SOUNDDLLPRO_CMD_SETBUTTON      "setbutton"
#define SOUNDDLLPRO_CMD_VOLUME         "volume"
#define SOUNDDLLPRO_CMD_TRACKVOLUME    "trackvolume"
#define SOUNDDLLPRO_CMD_CLEARDATA      "cleardata"
#define SOUNDDLLPRO_CMD_CLEARTRACK     "cleartrack"
#define SOUNDDLLPRO_CMD_TRACKLOAD      "trackload"
#define SOUNDDLLPRO_CMD_UNDERRUN       "underrun"
#define SOUNDDLLPRO_CMD_MUTE           "mute"
#define SOUNDDLLPRO_CMD_TRACKMUTE      "trackmute"
#define SOUNDDLLPRO_CMD_CHANNELMUTE    "channelmute"
#define SOUNDDLLPRO_CMD_RECMUTE        "recmute"
#define SOUNDDLLPRO_CMD_TRACKSOLO      "tracksolo"
#define SOUNDDLLPRO_CMD_CHANNELSOLO    "channelsolo"
#define SOUNDDLLPRO_CMD_RECSOLO        "recsolo"
#define SOUNDDLLPRO_CMD_TRACKNAME      "trackname"
#define SOUNDDLLPRO_CMD_CHANNELNAME    "channelname"
#define SOUNDDLLPRO_CMD_RECNAME        "recname"
#define SOUNDDLLPRO_CMD_PAUSE          "pause"
#define SOUNDDLLPRO_CMD_WAIT           "wait"
#define SOUNDDLLPRO_CMD_PLAYING        "playing"
#define SOUNDDLLPRO_CMD_XRUN           "xrun"
#define SOUNDDLLPRO_CMD_CLIPTHRS       "clipthreshold"
#define SOUNDDLLPRO_CMD_CLIPCOUNT      "clipcount"
#define SOUNDDLLPRO_CMD_RESETCLIPCOUNT "resetclipcount"
#define SOUNDDLLPRO_CMD_PLAYPOSITION   "playposition"
#define SOUNDDLLPRO_CMD_LOADPOSITION   "loadposition"
#define SOUNDDLLPRO_CMD_RECPOSITION    "recposition"
#define SOUNDDLLPRO_CMD_RECBUFSIZE     "recbufsize"
#define SOUNDDLLPRO_CMD_RECTHRSHLD     "recthreshold"
#define SOUNDDLLPRO_CMD_RECSTARTED     "recstarted"
#define SOUNDDLLPRO_CMD_RECLEN         "reclength"
#define SOUNDDLLPRO_CMD_RECFILENAME    "recfilename"
#define SOUNDDLLPRO_CMD_RECPAUSE       "recpause"
#define SOUNDDLLPRO_CMD_RECVOLUME      "recvolume"
#define SOUNDDLLPRO_CMD_RECORDING      "recording"
#define SOUNDDLLPRO_CMD_RECGETDATA     "recgetdata"
#define SOUNDDLLPRO_CMD_DEBUGSAVE      "debugsave"
#define SOUNDDLLPRO_CMD_DEBUGFILENAME  "debugfilename"
#define SOUNDDLLPRO_CMD_F2FFILENAME    "f2ffilename"
#define SOUNDDLLPRO_CMD_PLUGINSETDATA  "pluginsetdata"
#define SOUNDDLLPRO_CMD_PLUGINGETDATA  "plugingetdata"
#define SOUNDDLLPRO_CMD_IOSTATUS       "iostatus"
#define SOUNDDLLPRO_CMD_RESETERRORA    "resetasyncerror"
#define SOUNDDLLPRO_CMD_GETLASTERROR   "getlasterror"
#define SOUNDDLLPRO_CMD_ASYNCERROR     "asyncerror"
#define SOUNDDLLPRO_CMD_VSTQUERY       "vstquery"
#define SOUNDDLLPRO_CMD_VSTLOAD        "vstload"
#define SOUNDDLLPRO_CMD_VSTUNLOAD      "vstunload"
#define SOUNDDLLPRO_CMD_VSTPROGRAM     "vstprogram"
#define SOUNDDLLPRO_CMD_VSTPROGRAMNAME "vstprogramname"
#define SOUNDDLLPRO_CMD_VSTPARAM       "vstparam"
#define SOUNDDLLPRO_CMD_VSTSET         "vstset"
#define SOUNDDLLPRO_CMD_VSTSTORE       "vststore"
#define SOUNDDLLPRO_CMD_VSTEDIT        "vstedit"
#define SOUNDDLLPRO_CMD_DSPLOAD        "dspload"
#define SOUNDDLLPRO_CMD_DSPLOADRESET   "dsploadreset"
#define SOUNDDLLPRO_CMD_ADM            "adm"
#define SOUNDDLLPRO_CMD_MIDIINIT       "midiinit"
#define SOUNDDLLPRO_CMD_MIDIEXIT       "midiexit"
#define SOUNDDLLPRO_CMD_MIDIGETDRIVERS "midigetdrivers"
#define SOUNDDLLPRO_CMD_MIDIPLAYNOTE   "midiplaynote"
#define SOUNDDLLPRO_CMD_MIDISHORTMSG   "midishortmsg"
#define SOUNDDLLPRO_CMD_BETATEST       "betatest"
#define SOUNDDLLPRO_CMD_PREPARE        "prepare"
#define SOUNDDLLPRO_CMD_CHECKUPDATE    "checkupdate"
//------------------------------------------------------------------------------

//------------------------------------------------------------------------------
/// command parameter definitions
//------------------------------------------------------------------------------
#define SOUNDDLLPRO_PAR_NOGUI          "nogui"
#define SOUNDDLLPRO_PAR_PRIORITY       "priority"
#define SOUNDDLLPRO_PAR_LICINFO        "licinfo"
#define SOUNDDLLPRO_PAR_LOGFILE        "logfile"
#define SOUNDDLLPRO_PAR_USEIPC         "useipc"
#define SOUNDDLLPRO_PAR_DRIVER         "driver"
#define SOUNDDLLPRO_PAR_FILE2FILE      "file2file"
#define SOUNDDLLPRO_PAR_RECCOMPLATENCY "reccompensatelatency"
#define SOUNDDLLPRO_PAR_FILEREADBUFSIZE "filereadbufsize"
#define SOUNDDLLPRO_PAR_F2FBUFSIZE     "f2fbufsize"
#define SOUNDDLLPRO_PAR_NUMBUFS        "numbufs"
#define SOUNDDLLPRO_PAR_FREEZESRATE    "freezesamplerate"
#define SOUNDDLLPRO_PAR_WDMNUMBUFS     "wdmnumbufs"
#define SOUNDDLLPRO_PAR_TRACK          "track"
#define SOUNDDLLPRO_PAR_AUTOCLEARDATA  "autocleardata"
#define SOUNDDLLPRO_PAR_RECPROCDATA    "recprocesseddata"
#define SOUNDDLLPRO_PAR_OUTPUT         "output"
#define SOUNDDLLPRO_PAR_INPUT          "input"
#define SOUNDDLLPRO_PAR_RECURSECHANNEL "recursechannel"
#define SOUNDDLLPRO_PAR_RECURSEPOS     "recursepos"
#define SOUNDDLLPRO_PAR_CHANNEL        "channel"
#define SOUNDDLLPRO_PAR_MODE           "mode"
#define SOUNDDLLPRO_PAR_HANDLE         "handle"
#define SOUNDDLLPRO_PAR_LEFT           "left"
#define SOUNDDLLPRO_PAR_TOP            "top"
#define SOUNDDLLPRO_PAR_WIDTH          "width"
#define SOUNDDLLPRO_PAR_HEIGHT         "height"
#define SOUNDDLLPRO_PAR_DATA           "data"
#define SOUNDDLLPRO_PAR_DATADEST       "datadest"
#define SOUNDDLLPRO_PAR_NAME           "name"
#define SOUNDDLLPRO_PAR_SAMPLERATE     "samplerate"
#define SOUNDDLLPRO_PAR_RECDOWNSAMPLEFACTOR  "recdownsamplefactor"
#define SOUNDDLLPRO_PAR_RECFILEDISABLE "recfiledisable"
#define SOUNDDLLPRO_PAR_COPYOUT2IN     "copyout2in"
#define SOUNDDLLPRO_PAR_STOP           "stop"
#define SOUNDDLLPRO_PAR_STARTTIMEOUT   "starttimeout"
#define SOUNDDLLPRO_PAR_STOPTIMEOUT    "stoptimeout"
#define SOUNDDLLPRO_PAR_BUFSIZE        "bufsize"
#define SOUNDDLLPRO_PAR_SRATES         "samplerates"
#define SOUNDDLLPRO_PAR_SOUNDFORMAT    "soundformat"
#define SOUNDDLLPRO_PAR_LATENCYIN      "LatencyIn"
#define SOUNDDLLPRO_PAR_LATENCYOUT	   "LatencyOut"
#define SOUNDDLLPRO_PAR_SAMPLES        "samples"
#define SOUNDDLLPRO_PAR_CHANNELS       "channels"
#define SOUNDDLLPRO_PAR_VSTMT          "vstmultithreading"
#define SOUNDDLLPRO_PAR_VSTTP          "vstthreadpriority"
#define SOUNDDLLPRO_PAR_LOOPCOUNT      "loopcount"
#define SOUNDDLLPRO_PAR_PLUGIN_EXE     "pluginexe"
#define SOUNDDLLPRO_PAR_PLUGIN_START   "pluginstart"
#define SOUNDDLLPRO_PAR_PLUGIN_PROC    "pluginproc"
#define SOUNDDLLPRO_PAR_PLUGIN_SHOW    "pluginshow"
#define SOUNDDLLPRO_PAR_PLUGIN_KILL    "pluginkill"
#define SOUNDDLLPRO_PAR_PLUGIN_TIMEOUT       "plugintimeout"
#define SOUNDDLLPRO_PAR_PLUGIN_USERDATASIZE  "pluginuserdatasize"
#define SOUNDDLLPRO_PAR_PLUGIN_FORCEJVM      "pluginforcejvm"
#define SOUNDDLLPRO_PAR_OUTPUTS        "outputs"
#define SOUNDDLLPRO_PAR_TRACKS         "tracks"
#define SOUNDDLLPRO_PAR_INPUTS         "inputs"
#define SOUNDDLLPRO_PAR_TOPMOST        "topmost"
#define SOUNDDLLPRO_PAR_FOREGROUND     "foreground"
#define SOUNDDLLPRO_PAR_WAVEDATA       "wavedata"
#define SOUNDDLLPRO_PAR_VALUE          "value"
#define SOUNDDLLPRO_PAR_CLOSERECFILE   "closerecfile"
#define SOUNDDLLPRO_PAR_XR_PROC        "xrunproc"
#define SOUNDDLLPRO_PAR_XR_DONE        "xrundone"
#define SOUNDDLLPRO_PAR_MAXVALUE       "maxvalue"
#define SOUNDDLLPRO_PAR_OFFSET         "offset"
#define SOUNDDLLPRO_PAR_STARTOFFSET    "startoffset"
#define SOUNDDLLPRO_PAR_BUSY           "busy"
#define SOUNDDLLPRO_PAR_TIMEOUT        "timeout"
#define SOUNDDLLPRO_PAR_FILENAME       "filename"
#define SOUNDDLLPRO_PAR_FILEOFFSET     "fileoffset"
#define SOUNDDLLPRO_PAR_POINTER        "pointer"
#define SOUNDDLLPRO_PAR_POSITION       "position"
#define SOUNDDLLPRO_PAR_STARTPOS       "startpos"
#define SOUNDDLLPRO_PAR_LENGTH         "length"
#define SOUNDDLLPRO_PAR_PAUSE          "pause"
#define SOUNDDLLPRO_PAR_RAMPLEN        "ramplen"
#define SOUNDDLLPRO_PAR_LOOPRAMPLEN    "loopramplen"
#define SOUNDDLLPRO_PAR_LOOPCROSSFADE  "loopcrossfade"
#define SOUNDDLLPRO_PAR_CROSSFADELEN   "crossfadelen"
#define SOUNDDLLPRO_PAR_ERRORTEXT      "errortext"
#define SOUNDDLLPRO_PAR_FORCE          "force"
#define SOUNDDLLPRO_PAR_FORCELIC       "forcelic"
#define SOUNDDLLPRO_PAR_EXTPREVSTPROC  "extprevstproc"
#define SOUNDDLLPRO_PAR_EXTPOSTVSTPROC "extpostvstproc"
#define SOUNDDLLPRO_PAR_EXTRECPREVSTPROC  "extrecprevstproc"
#define SOUNDDLLPRO_PAR_EXTRECPOSTVSTPROC "extrecpostvstproc"
#define SOUNDDLLPRO_PAR_EXTDONEPROC    "extdoneproc"
#define SOUNDDLLPRO_PAR_EXTDATANOTIFY  "extdatanotify"
#define SOUNDDLLPRO_PAR_EXTVSTRING     "extvstring"
#define SOUNDDLLPRO_PAR_DATANOTIFYTRACK  "datanotifytrack"
#define SOUNDDLLPRO_PAR_INFO           "info"
#define SOUNDDLLPRO_PAR_TYPE           "type"
#define SOUNDDLLPRO_PAR_PROGRAMS       "programs"
#define SOUNDDLLPRO_PAR_PROGRAM        "program"
#define SOUNDDLLPRO_PAR_USERCONFIG     "userconfig"
#define SOUNDDLLPRO_PAR_PROGRAMNAME    "programname"
#define SOUNDDLLPRO_PAR_PARAMETER      "parameter"
#define SOUNDDLLPRO_PAR_POSITION       "position"
#define SOUNDDLLPRO_PAR_CONFIGFILE     "configfile"
#define SOUNDDLLPRO_PAR_GAIN           "gain"
#define SOUNDDLLPRO_PAR_PAN            "pan"
#define SOUNDDLLPRO_PAR_SOUNDMEXCALL   "soundmexcall"
#define SOUNDDLLPRO_PAR_MATLABEXE      "matlabexe"
#define SOUNDDLLPRO_PAR_PLUGINPATH     "pluginpath"
#define SOUNDDLLPRO_PAR_MAPFILE        "mapfile"
#define SOUNDDLLPRO_PAR_QUIET          "quiet"
#define SOUNDDLLPRO_PAR_NOTE           "note"
#define SOUNDDLLPRO_PAR_VOLUME         "volume"
#define SOUNDDLLPRO_PAR_STATUS         "status"
#define SOUNDDLLPRO_PAR_MIDI1          "midi1"
#define SOUNDDLLPRO_PAR_MIDI2          "midi2"
//------------------------------------------------------------------------------

//------------------------------------------------------------------------------
/// parameter value definitions
//------------------------------------------------------------------------------
#define SOUNDDLLPRO_VAL_INPUT          "input"
#define SOUNDDLLPRO_VAL_OUTPUT         "output"
#define SOUNDDLLPRO_VAL_MASTER         "master"
#define SOUNDDLLPRO_VAL_TRACK          "track"
#define SOUNDDLLPRO_VAL_FINAL          "final"
#define SOUNDDLLPRO_PAR_ALL            "all"
#define SOUNDDLLPRO_PAR_ASIO           "asio"
#define SOUNDDLLPRO_PAR_WDM            "wdm"
//------------------------------------------------------------------------------
#endif
