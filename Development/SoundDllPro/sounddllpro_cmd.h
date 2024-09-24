//------------------------------------------------------------------------------
/// \file SoundDllPro_cmd.h
/// \author Berg
/// Definition of a 'command buffer' containing all commands called by exported
/// function SoundDllProCommand (see SoundDllPro_Interface.cpp). It contains
/// a struct with help, syntax and some helper flags.
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
#ifndef sounddll_cmdH
#define sounddll_cmdH
#include "SoundDllPro_Interface.h"
//------------------------------------------------------------------------
//------------------------------------------------------------------------
// Structure array of all commands und respective functions.
//------------------------------------------------------------------------
static struct CMD_ARG
{
   const char *lpszName;   // Command
   const char *lpszHelp;   // Help
   const char *lpszArgs;   // known arguments
   LPFNSNDDLLFUNC lpfn;    // pointer to funciton
   int         nMustInit;  // flag if init must have been done
} cmd_arg[] =
{
{
   SOUNDDLLPRO_CMD_SETDRIVERMODEL,                                         // cmd
   "Name> " SOUNDDLLPRO_CMD_SETDRIVERMODEL "\n"                            // help
   "Help> sets driver model. Command 'init' must not called before!\n"
   "      Driver model 'wdm' is only available for Windows Vista or later.\n"
   "      NOTE: it is STRONGLY recommended NOT to use the 'wdm'-mode!!!\n"
   "      Try to use tools like ASIO4All or Jack instead!\n"
   "      NOTE: in 'wdm'-mode only 2 output channels and no inputs are\n"
   "      supported. 'xrun' and 'controlpanel' are not supported with 'wdm'.\n"
   "      NOTE: calling 'exit' clears the driver model, thus you have\n"
   "      to call 'setdrivermodel' again after calling 'exit'!\n"
   "Par.> value:        'asio' or 'wdm'\n"
   "Def.> value:        'asio'\n",
   SOUNDDLLPRO_PAR_VALUE ",",                                           // arguments
   SetDriverModel,                                                      // function pointer
   0                                                                    // must be initialized
},
{
   SOUNDDLLPRO_CMD_GETDRIVERMODEL,                                         // cmd
   "Name> " SOUNDDLLPRO_CMD_GETDRIVERMODEL "\n"                            // help
   "Help> returns current driver model\n"
   "Ret.> value:        current driver model",
   "",                                                                   // arguments
   GetDriverModel,                                                      // function pointer
   0                                                                    // must be initialized
},
{
   SOUNDDLLPRO_CMD_INIT,                                                // cmd
   "Name> " SOUNDDLLPRO_CMD_INIT "\n"                                   // help
   "Help> initializes module\n"
   "Par.> force:     if set to 1, 'exit' is called internally before init.\n"
   "      forcelic:  if set to 1, init forces a license to be used even, if it\n"
   "                 is currently locked by another computer.\n"
   "                 NOTE: if you pass this parameter, then you might force\n"
   "                 usage of a license that is currently used by another user!\n"
   "                 Without this parameter you are asked with a dialog if you\n"
   "                 want to take over the license and will have the choice...\n"
   "      driver:    name or index of ASIO driver to use\n"
   "                 NOTE: ignored for file2file-operation.\n"
   "      file2file: if set to '1' all final output channel data are written\n"
   "                 to files, no soundcard used at all. The default output file\n"
   "                 names are 'f2f_?.wav, where ? is the channel index To change\n"
   "                 filename see command 'f2fnames', to set buffersize to be used\n"
   "                 see 'f2fbufsize'.\n"
   "      f2fbufsize: buffersize to be used for file2file-operation.\n"
//   "      bufsize:      buffersize to use for ASIO driver\n"          // undocumented feature
   "      reccompensatelatency: if set to '1' then the latency retrieved from the\n"
   "                 driver in samples is cutted from record files.\n"
   "                 NOTE: value '1' is ignored if a 'recdownsamplefactor' other\n"
   "                 than '1' is speccified!\n"
   "                 NOTE: this option uses the latency retrieved from the driver\n"
   "                 itself. Depending in a particular driver this might lead to\n"
   "                 perfectly 'aligned' record files that contain exactly the played\n"
   "                 samples - or not! See also command 'getproperties'\n"
   "      filereadbufsize: buffer size used for wave file reading, If below 65536\n"
   "                 value is set to 65536.\n"
   "      samplerate: samplerate to use. NOTE: after intialization only this\n"
   "                 samplerate can be used, only files with this samplerate\n"
   "                 can be played!\n"
   "      output:    output channels to allocate (vector/array), or number of \n"
   "                 channels to use for file2file-operation (scalar value).\n"
   "                 NOTE: after initialization the allocated channels are\n"
   "                 enumerated starting with 0. If [1 2 4] is specified as\n"
   "                 output channels, you can access them in later commands\n"
   "                 only with indices 0, 1 or 2 respectively\n"
   "                 If -1 is specified, no output channels are used, if 'all'\n"
   "                 is specified all available output channels are used.\n"
   "      input:     input channels to allocate\n"
   "                 NOTE: after initialization the allocated channels are\n"
   "                 enumerated starting with 0. If [1 2 4] is specified as\n"
   "                 input channels, you can access them in later commands\n"
   "                 only with indices 0, 1 or 2 respectively\n"
   "                 If -1 is specified, no input channels are used, if 'all'\n"
   "                 is specified all available input channels are used\n"
   "                 NOTE: recorded data are always stored in normalized\n"
   "                 32-bit float PCM wave files.\n"
   "                 NOTE: never store record files directly on network drives\n"
   "                 or other slow drives! This may cause dropouts (xruns)!\n"
   "                 NOTE: ignored for file2file-operation.\n"
   "      track:     number of virtual output tracks to be used. Each output\n"
   "                 track is connected (mapped) to one output channels. This\n"
   "                 mapping can be changed with the command 'trackmap' (see\n"
   "                 also command 'trackmap' for a description how (multiple)\n"
   "                 track data are played on output channels). On 'init'\n"
   "                 the mapping is done 'circular', i.e. track 0 is mapped\n"
   "                 to channel 0, track 1 is mapped to channel 1 and so on.\n"
   "                 If more tracks than channels are specified, 'circular'\n"
   "                 means that mapping starts at channel 0 again. E.g. \n"
   "                 specifying [0, 1, 2] in output and 8 tracks leads to\n"
   "                 the following mapping:\n"
   "                        track0 -> channel0\n"
   "                        track1 -> channel1\n"
   "                        track2 -> channel2\n"
   "                        track3 -> channel0\n"
   "                        track4 -> channel1\n"
   "                        track5 -> channel2\n"
   "                        track6 -> channel0\n"
   "                        track7 -> channel1\n"
   "                 The current mapping can be retrieved with the command\n"
   "                 'trackmap'. On startup all tracks are in standard mode\n"
   "                 0 ('adding'), i.e. data samples are added up on the\n"
   "                 corresponding output channel .The mode of tracks can be\n"
   "                 changed with command 'trackmode'.\n"
   "      ramplen:   ramp length in samples applied when starting, stopping\n"
   "                 muting, unmuting, pausing, unpausing and setting master\n"
   "                 volumes (command 'volume').\n"
   "      numbufs:   number of buffers (each size of current ASIO buffer size)\n"
   "                 used for software buffering. Increases I/O delay and\n"
   "                 delay for commands like 'volume' or 'pause' to be\n"
   "                 applied, but lowers risk of xruns to occur.\n"
   "                 NOTE: read the special section 'Buffer configuration in\n"
   "                 manual if you need low latencies!\n"
   "                 NOTE: ignored for file2file-operation.\n"
//   "     wdmnumbufs: internal buffer number for WDM mode. Increase only, if\n"
//   "                 increasing 'numbufs' does NOT prevent dropouts. Values to\n"
//   "                 may result in an initialization error.\n"                        // undocumented feature
   " recdownsamplefactor: factor n for downsampling recording data. All data saved\n"
   "                 to disk or retrieved by 'recgetdata' are sampled down by\n"
   "                 avaraging n samples and writing this sample\n"
   " recfiledisable: disables recording to file completely. No files are created.\n"
   " recprocesseddata: if set to 1, then harddisk recording is done AFTER the VST\n"
   "                 and MATLAB script plugin. Otherwise the raw data from driver\n"
   "                 are written to disk.\n"
   "  autocleardata: flag, if audio data (vectors or files), that are already\n"
   "                 played completely should be cleared from memory auto-\n"
   "                 matically on next data loading command. Set this to 0, if\n"
   "                 you want to use the 'playposition' command to \"rewind\"\n"
   "                 to a certain playback position with parameter 'position':\n"
   "                 audio data segements are kept loaded until 'stop' command\n"
   "                 If parameter is set to 1 (default is 0), all audio data\n"
   "                 segments that were already played completely are freed\n"
   "                 from memory on every 'loadfile' or 'loadmem' command.\n"
   "                 This is especially useful for online stimulus generation,\n"
   "                 where hundreds of new data segments are loaded during\n"
   "                 runtime!\n"
   "   starttimeout: timeout in milliseconds that is allowed between command\n"
   "                 'start' and the start of the driver. This is a debugging\n"
   "                 option for sound cards that respond very slow.\n"
   "    stoptimeout: timeout in milliseconds that is allowed between command\n"
   "                 'stop' and real stop of the driver. This is a debugging\n"
   "                 option for sound cards that do not really stop\n"
   "                 immediately after they are told to do so.\n"
   " freezesamplerate: if this parameter s set to '1' then the real samplerate\n"
   "                 is not queried after start of the device any more and the\n"
   "                 stored samplerate BEFORE start is used. Intended for drivers\n"
   "                 that return an error when querying the samplerate on running\n"
   "                 devices\n"
   "    pluginexe:   executable for plugin. Leave this empty if running MATLAB\n"
   "                 or OCTAVE. Only to be set if using compiled plugins, see\n"
   "                 corresponding example\n"
   "                 NOTE: this parameter is not supported in Python\n"
   "    pluginstart: MATLAB script to be executed on startup of MATLAB script\n"
   "                 plugin\n"
   "                 NOTE: this parameter is not supported in Python\n"
   "    pluginproc:  MATLAB script to be executed for each audio buffer\n"
   "                 within MATLAB plugin. If this value is empty the MATLAB\n"
   "                 plugin interface stays disabled\n"
   "                 NOTE: this parameter is not supported in Python\n"
   "    pluginshow:  flag, if MATLAB process created for MATLAB plugin should\n"
   "                 be shown (0 or 1). NOTE: use only for debugging purposes!\n"
   "                 NOTE: for OCTAVE the workspace is only shown properly, if\n"
   "                 pluginkill is set to 0 as well!\n"
   "                 NOTE: this parameter is not supported in Python\n"
   "    pluginkill:  flag, if MATLAB process created for MATLAB plugin should\n"
   "                 be killed on 'exit' (0 or 1). While SoundMexPro is\n"
   "                 initialized you cannot access the MATLAB window that\n"
   "                 runs the plugin, so this parameter may be useful to keep\n"
   "                 the window alive after quitting SoundMexPro to check\n"
   "                 variables in plugin's workspace. NOTE: use only for\n"
   "                 debugging purposes! NOTE: if 'pluginshow' is set to 0 and\n"
   "                 'pluginkill' to 0, then you only can kill the processing\n"
   "                 MATLAB/OCTAVE instance with the task manager!\n"
   "                 NOTE: this parameter is not supported in Python\n"
   "  plugintimeout: timout in milliseconds for startup of the plugin. Set\n"
   "                 this value to higher values, if your startup script for\n"
   "                 the plugin takes some time.\n"
   "                 NOTE: this parameter is not supported in Python\n"
   "  pluginuserdatasize: size of user data per channel\n"
   "                 NOTE: this parameter is not supported in Python\n"
   " pluginforcejvm: flag if the MATLAB instance running the plugin should be\n"
   "                 started with Java Virtual Machine (JVM). It is highly\n"
   "                 recommended NOT to use this flag, since the JVM lowers\n"
   "                 the performance of plugins significantly. Additionally\n"
   "                 you may have to increase the value of the parameter\n"
   "                 'plugintimeout' because the MATLAB startup might be very\n"
   "                 slow. This parameter is ignored for Octave.\n"
   "                 NOTE: this parameter is not supported in Python\n"
   "      logfile:   name of a file for command and return value logging. If\n"
   "                 it is set non-empty all commands and return values are\n"
   "                 written to this file (not in MATLAB but SoundDllMaster\n"
   "                 syntax). NOTE: if write access to file fails (read only\n"
   "                 of invalid filename) 'init' command will fail!\n"
   " vstmultithreading: flag if each parallel VST plugin should run in a\n"
   "                 separate thread.\n"
   " vstthreadpriority: thread priority for VST threads. Must be between 0 and\n"
   "                 3 (0: normal, 1: higher, 2: highest, 3: time critical).\n"
   "                 Setting value to 3 (time critical) will give highest\n"
   "                 priority to processing, but may block other processes.\n"
   "                 This value is ignored, if 'vstmultithreading' is 0.\n"
   "      quiet:     if set to 1, then no version info is printed to workspace.\n"
   "Def.> force:     empty\n"
   "      forcelic:  0\n"
   "      driver:    0\n"
   "      file2file: 0\n"
   "      reccompensatelatency: 0\n"
   "      filereadbufsize: 655360\n"
   "     f2fbufsize: 1024\n"
//   "      bufsize: drivers preferred buffersize\n"
   "     samplerate: 44100\n"
   "      output:    [0 1] (first two channels) for regular operation,\n"
   "                 2 for file2file-operation.\n"
   "      input:     -1 (no recording at all!)\n"
   "      track:     one track for each allocated output channel\n"
   "      ramplen:   samplerate / 100\n"
   "      numbufs:   10 for ASIO driver model, 20 for WDM\n"
   " recdownsamplefactor: 1\n"
   " recfiledisable: 0\n"
   " recprocesseddata: 0\n"
   "  autocleardata: 0\n"
   "   starttimeout: 6000\n"
   "    stoptimeout: 1000\n"
   "    pluginstart: empty\n"
   "    pluginproc:  empty (no plugin started)\n"
   "    pluginshow:  0\n"
   "    pluginkill:  1\n"
   "  plugintimeout: 10000 (10 seconds)\n"
   " pluginuserdatasize: 100\n"
   " pluginforcejvm: 0\n"
   "    logfile:     empty (no logging)\n"
   " vstmultithreading: 1\n"
   " vstthreadpriority: 2\n"
   " quiet:             0\n"
   "Ret.> Type:      LicenceType",
   SOUNDDLLPRO_PAR_DRIVER ","                                           // arguments
   SOUNDDLLPRO_PAR_PRIORITY ","
   SOUNDDLLPRO_PAR_FILEREADBUFSIZE ","
   SOUNDDLLPRO_PAR_FILE2FILE ","
   SOUNDDLLPRO_PAR_RECCOMPLATENCY ","
   SOUNDDLLPRO_PAR_F2FBUFSIZE ","
   SOUNDDLLPRO_PAR_STARTTIMEOUT ","
   SOUNDDLLPRO_PAR_STOPTIMEOUT ","
   SOUNDDLLPRO_PAR_BUFSIZE ","
   SOUNDDLLPRO_PAR_PLUGIN_EXE ","
   SOUNDDLLPRO_PAR_PLUGIN_START ","
   SOUNDDLLPRO_PAR_PLUGIN_PROC ","
   SOUNDDLLPRO_PAR_PLUGIN_SHOW ","
   SOUNDDLLPRO_PAR_PLUGIN_TIMEOUT ","
   SOUNDDLLPRO_PAR_PLUGIN_USERDATASIZE ","
   SOUNDDLLPRO_PAR_PLUGIN_FORCEJVM ","
   SOUNDDLLPRO_PAR_AUTOCLEARDATA ","
   SOUNDDLLPRO_PAR_PLUGIN_KILL ","
   SOUNDDLLPRO_PAR_LOGFILE ","
   SOUNDDLLPRO_PAR_VSTMT ","
   SOUNDDLLPRO_PAR_VSTTP ","
   SOUNDDLLPRO_PAR_FREEZESRATE ","
   SOUNDDLLPRO_PAR_TRACK ","
   SOUNDDLLPRO_PAR_NUMBUFS ","
   SOUNDDLLPRO_PAR_WDMNUMBUFS ","
   SOUNDDLLPRO_PAR_FORCE ","
   SOUNDDLLPRO_PAR_FORCELIC ","
   SOUNDDLLPRO_PAR_OUTPUT ","
   SOUNDDLLPRO_PAR_INPUT ","
   SOUNDDLLPRO_PAR_RECDOWNSAMPLEFACTOR ","
   SOUNDDLLPRO_PAR_RECFILEDISABLE ","
   SOUNDDLLPRO_PAR_RECPROCDATA ","
   SOUNDDLLPRO_PAR_SAMPLERATE ","
   SOUNDDLLPRO_PAR_RAMPLEN ","
   SOUNDDLLPRO_PAR_MATLABEXE ","
   SOUNDDLLPRO_PAR_USEIPC ","
   SOUNDDLLPRO_PAR_PLUGINPATH ","
   SOUNDDLLPRO_PAR_QUIET ","
   SOUNDDLLPRO_PAR_COPYOUT2IN ","
   SOUNDDLLPRO_PAR_EXTPREVSTPROC ","
   SOUNDDLLPRO_PAR_EXTPOSTVSTPROC ","
   SOUNDDLLPRO_PAR_EXTRECPREVSTPROC ","
   SOUNDDLLPRO_PAR_EXTRECPOSTVSTPROC ","
   SOUNDDLLPRO_PAR_EXTDONEPROC ","
   SOUNDDLLPRO_PAR_EXTDATANOTIFY ","
   SOUNDDLLPRO_PAR_DATANOTIFYTRACK ","
   ,
   Init,                                                                // function pointer
   0                                                                    // must be initialized
},
{  SOUNDDLLPRO_CMD_EXIT,                                                // cmd
   "Name> " SOUNDDLLPRO_CMD_EXIT "\n"                                   // help
   "Help> de-initializes SOUNDDLLPRO",
   "",                                                                  // arguments
   Exit,                                                                // function pointer
   0                                                                    // must be initialized
},
{  SOUNDDLLPRO_CMD_ABOUT,                                               // cmd
   "Name> " SOUNDDLLPRO_CMD_ABOUT "\n"                                  // help
   "Help> Shows an about box with information about SoundMexPro",
   "",                                                                  // arguments
   About,                                                               // function pointer
   0                                                                    // must be initialized
},
{  SOUNDDLLPRO_CMD_CHECKUPDATE,                                         // cmd
   "Name> " SOUNDDLLPRO_CMD_CHECKUPDATE "\n"                            // help
   "Help> returns latest available SoundMexPro version from website\n"
   "Ret.> Update:   1 if an update is available or else\n"   
   "      Version:  version string of latest available version",   
   "",                                                                  // arguments
   CheckUpdate,                                                         // function pointer
   0                                                                    // must be initialized
},
{  SOUNDDLLPRO_CMD_INITIALIZED,                                         // cmd
   "Name> " SOUNDDLLPRO_CMD_INITIALIZED "\n"                            // help
   "Help> determines if module is initialized\n"
   "Ret.> initialized:  1 if initialized, 0 else",
   "",                                                                  // arguments
   Initialized,                                                         // function pointer
   0                                                                    // must be initialized
},
{  SOUNDDLLPRO_CMD_VERSION,                                             // cmd
   "Name> " SOUNDDLLPRO_CMD_VERSION "\n"                                // help
   "Help> returns version string\n"
   "Ret.> Version:      version string",
   "",                                                                  // arguments
   Version,                                                             // function pointer
   0                                                                    // must be initialized
},
{  SOUNDDLLPRO_CMD_LICENSE,                                             // cmd
   "Name> " SOUNDDLLPRO_CMD_LICENSE "\n"                                // help
   "Help> returns current license information\n"
   "Ret.> Version:      current major revision number of SoundMexPro\n"
   "      Ed:           license type (edition)",
   SOUNDDLLPRO_PAR_LICINFO  ",",                                             // arguments
   License,                                                             // function pointer
   0                                                                    // must be initialized
},
{  SOUNDDLLPRO_CMD_SHOW,                                                // cmd
   "Name> " SOUNDDLLPRO_CMD_SHOW "\n"                                   // help
   "Help> shows mixer (identical to 'showmixer').\n"
   "Par.> outputs:      if set to '0' output mixers are hidden on startup\n"
   "      tracks:       if set to '0' track mixers are hidden on startup\n"
   "      inputs:       if set to '0' input mixers are hidden on startup\n"
   "      topmost:      if set to '1' mixer window stays on top\n"
   "      foreground:   if set to 1 window is forced to the foreground\n"
   "Def.> outputs:      1\n"
   "      tracks:       1\n"
   "      inputs:       1\n"
   "      topmost:      0\n"
   "      foreground:   1\n",
   SOUNDDLLPRO_PAR_OUTPUTS  ","
   SOUNDDLLPRO_PAR_TRACKS  ","
   SOUNDDLLPRO_PAR_INPUTS  ","
   SOUNDDLLPRO_PAR_TOPMOST ","
   SOUNDDLLPRO_PAR_FOREGROUND ",",
   ShowMixer,                                                           // function pointer
   1                                                                    // must be initialized
},
{  SOUNDDLLPRO_CMD_HIDE,                                                // cmd
   "Name> " SOUNDDLLPRO_CMD_HIDE "\n"                                   // help
   "Help> hides visualization of allocated channels",
   "",                                                                  // arguments
   Hide,                                                                // function pointer
   1                                                                    // must be initialized
},
{  SOUNDDLLPRO_CMD_SHOWTRACKS,                                          // cmd
   "Name> " SOUNDDLLPRO_CMD_SHOWTRACKS "\n"                             // help
   "Help> shows visualization of files/vectors in tracks. This 'view' is\n"
   "      especially intended to check the setup of your experiment/pardigm,\n"
   "      i.e. if everything is loaded/located as expected. During playback\n"
   "      a cursor shows the current position.\n"
   "      NOTE: command does call 'updatetracks' internally, so there is no\n"
   "      need to call 'updatetracks' directly after 'showtracks'.\n"
   "Par.> topmost:      if set to '1' track window stays on top\n"
   "      foreground:   if set to 1 window is forced to the foreground\n"
   "      wavedata:     if set to '1' waveforms are painted as well.\n"
   "                    NOTE: this might take quite a while...\n"
   "Def.> topmost:      0\n"
   "      foreground:   1\n"
   "      wavedata:     1\n",
   SOUNDDLLPRO_PAR_WAVEDATA ","
   SOUNDDLLPRO_PAR_TOPMOST ","
   SOUNDDLLPRO_PAR_FOREGROUND ",",
   ShowTracks,                                                          // function pointer
   1                                                                    // must be initialized
},
{  SOUNDDLLPRO_CMD_HIDETRACKS,                                          // cmd
   "Name> " SOUNDDLLPRO_CMD_HIDETRACKS "\n"                             // help
   "Help> hides visualization of files/vectors in tracks.",
   "",                                                                  // arguments
   HideTracks,                                                          // function pointer
   1                                                                    // must be initialized
},
{  SOUNDDLLPRO_CMD_UPDATETRACKS,                                        // cmd
   "Name> " SOUNDDLLPRO_CMD_UPDATETRACKS "\n"                           // help
   "Help> updates visualization of files/vectors in tracks. NOTE: this \n"
   "      command must be called to have the loaded files and vectors'up\n"
   "      to date'. It is recommended to call it directly before 'play'.\n"
   "Par.> wavedata:     if set to '1' waveforms are painted as well.\n"
   "                    NOTE: this might take quite a while...\n"
   "Def.> wavedata:     1\n",
   SOUNDDLLPRO_PAR_WAVEDATA ",",
   UpdateTracks,                                                      // function pointer
   1                                                                    // must be initialized
},
{  SOUNDDLLPRO_CMD_SHOWMIXER,                                           // cmd
   "Name> " SOUNDDLLPRO_CMD_SHOWMIXER "\n"                              // help
   "Help> shows mixer (identical to 'show').\n"
   "Par.> outputs:      if set to '0' output mixers are hidden on startup\n"
   "      tracks:       if set to '0' track mixers are hidden on startup\n"
   "      inputs:       if set to '0' input mixers are hidden on startup\n"
   "      topmost:      if set to '1' mixer window stays on top\n"
   "      foreground:   if set to 1 window is forced to the foreground\n"
   "Def.> outputs:      1\n"
   "      tracks:       1\n"
   "      inputs:       1\n"
   "      topmost:      0\n"
   "      foreground:   1\n",
   SOUNDDLLPRO_PAR_OUTPUTS  ","
   SOUNDDLLPRO_PAR_TRACKS  ","
   SOUNDDLLPRO_PAR_INPUTS  ","
   SOUNDDLLPRO_PAR_TOPMOST ","
   SOUNDDLLPRO_PAR_FOREGROUND ",",
   ShowMixer,                                                           // function pointer
   1                                                                    // must be initialized
},
{  SOUNDDLLPRO_CMD_HIDEMIXER,                                           // cmd
   "Name> " SOUNDDLLPRO_CMD_HIDEMIXER "\n"                              // help
   "Help> hides mixer.",
   "",                                                                  // arguments
   HideMixer,                                                           // function pointer
   1                                                                    // must be initialized
},
{  SOUNDDLLPRO_CMD_GETDRV,                                              // cmd
   "Name> " SOUNDDLLPRO_CMD_GETDRV "\n"                                 // help
   "Help> returns names of all installed ASIO drivers\n"
   "Ret.> driver:    vector/array with ASIO driver names",
   "",                                                                  // arguments
   GetDrivers,                                                          // function pointer
   0                                                                    // must be initialized
},
{  SOUNDDLLPRO_CMD_GETDRVSTATUS,                                        // cmd
   "Name> " SOUNDDLLPRO_CMD_GETDRVSTATUS "\n"                           // help
   "Help> returns status of all installed ASIO drivers\n"
   "Ret.> value:    vector/array  with ASIO driver status (1: ok, 0: error)",
   "",                                                                  // arguments
   GetDriverStatus,                                                     // function pointer
   0                                                                    // must be initialized
},
{  SOUNDDLLPRO_CMD_GETCH,                                               // cmd
   "Name> " SOUNDDLLPRO_CMD_GETCH "\n"                                  // help
   "Help> returns names of all channels of an ASIO driver\n"
   "      NOTE: if SoundMexPro is already initialized, the parameters are\n"
   "      ignored and the current driver is queried!\n"
   "Par.> driver:    name or index of ASIO driver to query\n"
   "Ret.> output:    vector/array  with names of output channels\n"
   "      input:     vector/array  with names of input channels",
   SOUNDDLLPRO_PAR_DRIVER ",",                                          // arguments
   GetChannels,                                                         // function pointer
   0                                                                    // must be initialized
},
{  SOUNDDLLPRO_CMD_GETDRV_ACTIVE,                                       // cmd
   "Name> " SOUNDDLLPRO_CMD_GETDRV_ACTIVE "\n"                          // help
   "Help> returns the name of the active ASIO driver\n"
   "Ret.> driver:    name of the ASIO driver used in command 'init'",
   "",                                                                  // arguments
   GetActiveDriver,                                                     // function pointer
   1                                                                    // must be initialized
},
{  SOUNDDLLPRO_CMD_GETCH_ACTIVE,                                        // cmd
   "Name> " SOUNDDLLPRO_CMD_GETCH_ACTIVE "\n"                           // help
   "Help> returns the names of all channels of current driver that were\n"
   "      allocated in 'init'\n"
   "Ret.> output:    vector/array with names of allocated ouptut channels\n"
   "      input:     vector/array with names of allocated input channels",
   "",                                                                  // arguments
   GetActiveChannels,                                                   // function pointer
   1                                                                    // must be initialized
},
{  SOUNDDLLPRO_CMD_GETPROP,                                             // cmd
   "Name> " SOUNDDLLPRO_CMD_GETPROP "\n"                                // help
   "Help> returns current samplerate and buffer size (samples) of current\n"
   "      driver, a list of supported samplerates and used sound format.\n"
   "      NOTE: before the device is running (i.e. 'start' was called) the\n"
   "      samplerate may differ from the sample rate that was specified in\n"
   "      command 'init': some drivers switch it not before device start.\n"
   "      If switching to specified sample rate is not successful in command\n"
   "      'start', it will fail with a corresponding error message.\n"
   "      NOTE: the list of supported samplerates may not be complete, it is\n"
   "      generated by 'asking' the driver if a particular samplerate is\n"
   "      supported. Some drivers return 'true' even if starting that\n"
   "      samplerate will fail (e.g. due to a samplerate lock by driver\n"
   "      settings dialog or external hardware). Some drivers may return\n"
   "      only one samplerate (the current one) even if others are\n"
   "      supported. The following samplerates are checked: 8000, 11025,\n"
   "      16000, 22050, 32000, 44100, 48000, 88200, 96000, 176400, 192000,\n"
   "      352800, 384000.\n"
   "Ret.> samplerate:   current samplerate\n"
   "      bufsize:      current ASIO buffersize on samples\n"
   "      samplerates:  vector/array with supported samplerates\n"
   "      soundformat:  description of currently used sound format of device\n"
   "      LatencyIn:    input latency as reveived from driver.\n"
   "      LatencyOut:   output latency as reveived from driver.\n"
	"      NOTE: the latencies will not include software buffering latency\n"
	"      and hardware delays, these are internal driver values as reported\n"
	"      by the driver itself!",
   "",                                                                  // arguments
   GetProperties,                                                       // function pointer
   1                                                                    // must be initialized
},
{  SOUNDDLLPRO_CMD_CONTROLPANEL,                                        // cmd
   "Name> " SOUNDDLLPRO_CMD_CONTROLPANEL "\n"                           // help
   "Help> shows 'own' control panel of an ASIO driver.\n"
   "      NOTE: if SoundMexPro is already initialized, no driver must be\n"
   "      specified, the current driver is called!\n"
   "      NOTE: command may raise an error for some drivers, if SoundMexPro\n"
   "      is already initialized!\n"
   "      NOTE: for some drivers this command may not return before the\n"
   "      control is closed again!\n"
   "Par.> driver:    name or index of ASIO driver\n"
   "Def.> driver:    0\n",
   SOUNDDLLPRO_PAR_DRIVER ",",                                          // arguments
   ShowControlPanel,                                                    // function pointer
   0                                                                    // must be initialized
},
{  SOUNDDLLPRO_CMD_TRACKMAP,                                            // cmd
   "Name> " SOUNDDLLPRO_CMD_TRACKMAP "\n"                               // help
   "Help> sets track mapping and returns current mapping\n"
   "Par.> track:     vector/array with track mapping. The vector/array must have\n"
   "                 an entry for every initialized track (see parameter 'track'\n"
   "                 of command 'init') specifying the output channel (indices\n"
   "                 or array with names), on which to playback the track\n"
   "                 data. Data are loaded to tracks with commands 'loadmem'\n"
   "                 and 'loadfile'. The data of all tracks are 'applied' to\n"
   "                 the data of the corresponding output channel, i.e. if\n"
   "                 more than one track is mapped to the same output the\n"
   "                 signals are added or multiplied and you may have to take\n"
   "                 care for clipping! \n"
   "Ret.> track:     vector/array with current track mapping",
   SOUNDDLLPRO_PAR_TRACK ",",
   TrackMap,                                                              // function pointer
   1                                                                    // must be initialized
},
{  SOUNDDLLPRO_CMD_TRACKMODE,                                           // cmd
   "Name> " SOUNDDLLPRO_CMD_TRACKMODE "\n"                              // help
   "Help> sets mode and returns current mode of tracks\n"
   "Par.> mode:      vector/array with mode values. If no value is specified, no\n"
   "                 volume is changed, current modes are returned. Either one\n"
   "                 mode must be specified (applied to all tracks) or lengths\n"
   "                 of mode vector/array and track vector/array must be identical\n"
   "                 (modes applied in corresponding order to specified tracks).\n"
   "                 Valid modes are:\n"
   "                    0  sample values of tracks are added to the output\n"
   "                       channel where they are mapped to,\n"
   "                    1  output channel data are multiplied with the track\n"
   "                       data. NOTE: the multiplication is _not_ done with\n"
   "                       the final, total data, but with the data processed\n"
   "                       (added) from tracks with a lower index. So if you\n"
   "                       want to apply a multiplication on the final channel\n"
   "                       output be sure, that you set the last track mapped\n"
   "                       to the corresponding channel to mode '1'!\n"
   "      track:     vector/array with tracks (indices or array with names)\n"
   "                 to apply mode to (no duplicates allowed)\n"
   "Def.> mode:      current modes (no changes)\n"
   "      track:     vector/array with all tracks\n"
   "Ret.> mode:      vector/array with current modes for all tracks",
   SOUNDDLLPRO_PAR_MODE ","                                             // arguments
   SOUNDDLLPRO_PAR_TRACK ",",
   TrackMode,                                                           // function pointer
   1                                                                    // must be initialized
},
{  SOUNDDLLPRO_CMD_TRACKLEN,                                           // cmd
   "Name> " SOUNDDLLPRO_CMD_TRACKLEN "\n"                              // help
   "Help> returns vector/array with total length of all tracks, i.e. position of\n"
   "      last sample in each track. A value of -1 indicates, that an endless\n"
   "      loop is running on the particular track.\n"
   "Ret.> value:     vector/array with lengths of all tracks",
   "",
   TrackLen,                                                           // function pointer
   1                                                                    // must be initialized
},
{  SOUNDDLLPRO_CMD_START,                                               // cmd
   "Name> " SOUNDDLLPRO_CMD_START "\n"                                  // help
   "Help> starts input and output\n"
   "      IMPORTANT NOTE: record files are always overwritten! Use the \n"
   "      'recfilename' command to change the filenames if necessary.\n"
   "      NOTE: recorded data are always stored in normalized 32-bit float\n"
   "      PCM wave files.\n"
   "      NOTE: if you get an error 'setting samplerate of device to XY not\n"
   "      successful', check if the sample rate is locked by the driver (check\n"
   "      control panel and/or documentation of driver for more information).\n"
   "      IMPORTANT NOTE: if you are running in file2file-mode (see command\n"
   "      'init'), then the command 'start' will return after the file2file-\n"
   "      operation is complete. Depending on your 'loadfile' and 'loadmem'\n"
   "      calls this may take a while! Afterwards the command 'cleardata' is\n"
   "      called automatically.\n"
   "Par.> length:    'running' length. Values for 'length' may be:\n"
   "           < 0:  device is stopped (playback and record) after all tracks\n"
   "                 played their data. NOTE: After each ASIO buffer it is\n"
   "                 checked, if no track has more buffered data to play. If\n"
   "                 this is true the device is stopped.\n"
   "             0:  device is never stopped, zeros are played endlessly. In\n"
   "                 this case you may load new data to track(s) at any time.\n"
   "                 NOTE: if you record data to a file this runs forever as\n"
   "                 well and files may become quite huge!\n"
   "           > 0:  length in samples to play/record before device is stopped.\n"
   "                 NOTE: this length will not be sample accurate due to\n"
   "                 block processing.\n"
   "                 This parameter is ignored in file2file-mode.\n"
   "      pause:     if set to 1 device is paused rather than stopped\n"
   "                 This parameter is ignored in file2file-mode.\n"
   "Def.> length:    -1. If NO output channels are specified on init (i.e. -1)\n"
   "                 for 'output'), then default is 0.\n"
   "      pause:     0",
   SOUNDDLLPRO_PAR_LENGTH ","
   SOUNDDLLPRO_PAR_PAUSE ",",                                          // arguments
   Start,                                                               // function pointer
   1                                                                    // must be initialized
},
{  SOUNDDLLPRO_CMD_STARTTHRSHLD,                                        // cmd
   "Name> " SOUNDDLLPRO_CMD_STARTTHRSHLD "\n"                           // help
   "Help> starts input and output after threshold value is exceeded in one or\n"
   "      more input channels\n"
   "      IMPORTANT NOTE: the command returns immediately to MATLAB, real start\n"
   "      of input and output waits for the threshold to be exceeced. If you\n"
   "      want to check, if threshold was exceeded meanwhile after calling \n"
   "      'startthreshold', use the command 'started': while waiting for\n"
   "      threshold it will return 0 in second return value, afterwards it will\n"
   "      return 1. This way you implement a waiting loop with timeout. After\n"
   "      the threshold is exceeded the playback starts immediately, but it will\n"
   "      start with\n"
   "                    numbufs * ASIO buffersize\n"
   "      zero samples (see parameter 'numbufs' of command 'init').\n"
   "      IMPORTANT NOTE: record files are always overwritten! Use the \n"
   "      'recfilename' command to change the filenames if necessary.\n"
   "      NOTE: recorded data are always stored in normalized 32-bit float\n"
   "      PCM wave files.\n"
   "      NOTE: if you get an error 'setting samplerate of device to XY not\n"
   "      successful', check if the sample rate is locked by the driver (check\n"
   "      control panel and/or documentation of driver for more information).\n"
   "      NOTE: this command is not available in file2file-mode.\n"
   "Par.> value:     Threshold between 0 and 1, current value is returned.\n"
   "                 If no value is specified the current value is not changed.\n"
   "                 A value of 0 disables the threshold. Otherwise playback\n"
   "                 and recording starts with the next buffer after the\n"
   "                 threshold was exceeded (with respect to the specified\n"
   "                 value, mode and channels).\n"
   "                 NOTE: threshold is resetted after exceeding it (set to 0)!\n"
   "      mode:      Flag, if the threshold must be exceeded in one (1) or\n"
   "                 all (0) of the channels specified in 'channel'. Must\n"
   "                 be 0 or 1.\n"
   "      channel:   vector/array with input channels (indices or names)\n"
   "                 to check for the threshold (no duplicates allowed)\n"
   "      length:    'running' length. Values for 'length' may be:\n"
   "           < 0:  device is stopped (playback and record) after all tracks\n"
   "                 played their data. NOTE: After each ASIO buffer it is\n"
   "                 checked, if no track has more buffered data to play. If\n"
   "                 this is true the device is stopped.\n"
   "             0:  device is never stopped, zeros are played endlessly. In\n"
   "                 this case you may load new data to track(s) at any time.\n"
   "                 NOTE: if you record data to a file this runs forever as\n"
   "                 well and files may become quite huge!\n"
   "           > 0:  length in samples to play/record before device is stopped.\n"
   "                 NOTE: this length will not be sample accurate due to\n"
   "                 block processing.\n"
   "                 This parameter is ignored in file2file-mode.\n"
   "      pause:     if set to 1 device is paused rather than stopped.\n"
   "Def.> value:     current thresholds (no changes, 0 on startup)\n"
   "      mode:      1\n"
   "      channel:   vector/array with all allocated input channels\n"
   "      length:    -1. If NO output channels are specified on init (i.e. -1)\n"
   "                 for 'output'), then default is 0.\n"
   "      pause:     0\n"
   "Ret.> value:     current threshold value (between 0.0 and 1.0)\n"
   "      mode:      current threshold mode\n",
   SOUNDDLLPRO_PAR_LENGTH ","
   SOUNDDLLPRO_PAR_PAUSE ","                                          // arguments
   SOUNDDLLPRO_PAR_VALUE ","                                            // arguments
   SOUNDDLLPRO_PAR_MODE ","
   SOUNDDLLPRO_PAR_CHANNEL ",",
   StartThreshold,                                                      // function pointer
   1                                                                    // must be initialized
},
{  SOUNDDLLPRO_CMD_STARTED,                                             // cmd
   "Name> " SOUNDDLLPRO_CMD_STARTED "\n"                                // help
   "Help> checks, if device was started (is still running) NOTE: this command\n"
   "      only checks if the ASIO device runs, it does not check, if data are\n"
   "      playing on any channel (see 'playing')\n"
   "Ret.> value:     1 if device is started, 0 else",
   "",                                                                  // arguments
   Started,                                                             // must be initialized
   1
},
{  SOUNDDLLPRO_CMD_STOP,                                                // cmd
   "Name> " SOUNDDLLPRO_CMD_STOP "\n"                                   // help
   "Help> stops device and clears loaded data",
   "",                                                                  // arguments
   Stop,                                                                // must be initialized
   1
},
{  SOUNDDLLPRO_CMD_PAUSE,                                               // cmd
   "Name> " SOUNDDLLPRO_CMD_PAUSE "\n"                                  // help
   "Help> sets pause status of device (playback  and record) and returns\n"
   "      current status.\n"
   "Par.> value:     1 (pauses device) or 0 (unpauses device)\n"
   "Def.> value:     current value (no change)\n"
   "Ret.> value:     1 if device is paused, 0 else",
   SOUNDDLLPRO_PAR_VALUE ",",                                           // arguments
   Pause,                                                               // function pointer
   1                                                                    // must be initialized
},
{  SOUNDDLLPRO_CMD_MUTE,                                                // cmd
   "Name> " SOUNDDLLPRO_CMD_MUTE "\n"                                   // help
   "Help> sets mute status and returns current status. NOTE: this command\n"
   "      mutes all output channels globally using a ramp of length 'ramplen'\n"
   "      (argument of command 'init'). See also commands 'channelmute' and\n"
   "      'trackmute' and 'recmute'.\n"
   "Par.> value:     1 (mutes output) or 0 (unmutes output)\n"
   "Def.> value:     current value (no change)\n"
   "Ret.> value:     1 if device is muted, 0 else.",
   SOUNDDLLPRO_PAR_VALUE ",",                                           // arguments
   Mute,                                                                // function pointer
   1                                                                    // must be initialized
},
{  SOUNDDLLPRO_CMD_TRACKMUTE,                                           // cmd
   "Name> " SOUNDDLLPRO_CMD_TRACKMUTE "\n"                             // help
   "Help> sets mute status of one or more tracks and returns current status.\n"
   "      NOTE: muting/unmuting is not ramped!\n"
   "      NOTE: 'solo' status supersedes 'mute' status: if solo status of any\n"
   "      track is '1', then mute status of all tracks is ignored!\n"
   "Par.> value:     vector/array with mute values (0 for unmute or 1 for mute).\n"
   "                 If no value is specified, no mute values are changed,\n"
   "                 current values are returned. Either one value must be\n"
   "                 specified (applied to all tracks) or lengths of mute\n"
   "                 and track vector/array must be identical (values applied in\n"
   "                 corresponding order to specified tracks).\n"
   "      track:     vector/array with tracks (indices or array with names)\n"
   "                 to apply values (no duplicates allowed)\n"
   "Def.> value:     current mute values (no changes)\n"
   "      track:     vector/array with all tracks\n"
   "Ret.> value:     vector/array with current mute values for all tracks",
   SOUNDDLLPRO_PAR_VALUE ","                                            // arguments
   SOUNDDLLPRO_PAR_TRACK ",",
   ChannelMuteSolo,                                                     // function pointer
   1                                                                    // must be initialized
},
{  SOUNDDLLPRO_CMD_CHANNELMUTE,                                         // cmd
   "Name> " SOUNDDLLPRO_CMD_CHANNELMUTE "\n"                            // help
   "Help> sets mute status of one or more output channels and returns current\n"
   "      status. NOTE: muting/unmuting is not ramped!\n"
   "      NOTE: 'solo' status supersedes 'mute' status: if solo status of any\n"
   "      output is '1', then mute status of all outputs is ignored!\n"
   "Par.> value:     vector/array with mute values (0 for unmute or 1 for mute).\n"
   "                 If no value is specified, no mute values are changed,\n"
   "                 current values are returned. Either one value must be\n"
   "                 specified (applied to all channels) or lengths of mute\n"
   "                 and output vector/array must be identical (values applied\n"
   "                 in corresponding order to specified channels).\n"
   "     output:     vector/array with output channels (indices or array with\n"
   "                 names) to apply values (no duplicates allowed)\n"
   "Def.> value:     current mute values (no changes)\n"
   "      output:    vector/array with all output channels\n"
   "Ret.> value:     vector/array with current mute values for all output channels",
   SOUNDDLLPRO_PAR_VALUE ","                                            // arguments
   SOUNDDLLPRO_PAR_OUTPUT ",",
   ChannelMuteSolo,                                                     // function pointer
   1                                                                    // must be initialized
},
{  SOUNDDLLPRO_CMD_RECMUTE,                                             // cmd
   "Name> " SOUNDDLLPRO_CMD_RECMUTE "\n"                                // help
   "Help> sets mute status of one or more input channels and returns current\n"
   "      status. NOTE: mute/unmute is not ramped!\n"
   "      NOTE: 'solo' status supersedes 'mute' status: if solo status of any\n"
   "      input is '1', then mute status of all inputs is ignored!\n"
   "Par.> value:     vector/array with mute values (0 for unmute or 1 for mute).\n"
   "                 If no value is specified, no mute values are changed,\n"
   "                 current values are returned. Either one value must be\n"
   "                 specified (applied to all channels) or lengths of mute\n"
   "                 and input vector/array must be identical (values applied in\n"
   "                 corresponding order to specified channels).\n"
   "      input:     vector/array with input channels (indices or array with\n"
   "                 names) to apply values (no duplicates allowed)\n"
   "Def.> value:     current mute values (no changes)\n"
   "      input:     vector/array with all input channels\n"
   "Ret.> value:     vector/array with current mute values for all input channels",
   SOUNDDLLPRO_PAR_VALUE ","                                            // arguments
   SOUNDDLLPRO_PAR_INPUT ",",
   ChannelMuteSolo,                                                     // function pointer
   1                                                                    // must be initialized
},
{  SOUNDDLLPRO_CMD_TRACKSOLO,                                           // cmd
   "Name> " SOUNDDLLPRO_CMD_TRACKSOLO "\n"                             // help
   "Help> sets solo status of one or more tracks and returns current status.\n"
   "      NOTE: muting/unmuting is not ramped!\n"
   "      NOTE: 'solo' status supersedes 'mute' status: if solo status of any\n"
   "      track is '1', then mute status of all tracks is ignored!\n"
   "Par.> value:     vector/array with solo values (0 for unsolo or 1 for solo).\n"
   "                 If no value is specified, no solo values are changed,\n"
   "                 current values are returned. Either one value must be\n"
   "                 specified (applied to all tracks) or lengths of solo\n"
   "                 and track vector/array must be identical (values applied in\n"
   "                 corresponding order to specified tracks).\n"
   "      track:     vector/array with tracks (indices or array with names) to\n"
   "                 apply values (no duplicates allowed)\n"
   "Def.> value:     current solo values (no changes)\n"
   "      track:     vector/array with all tracks\n"
   "Ret.> value:     vector/array with current solo values for all tracks",
   SOUNDDLLPRO_PAR_VALUE ","                                            // arguments
   SOUNDDLLPRO_PAR_TRACK ",",
   ChannelMuteSolo,                                                     // function pointer
   1                                                                    // must be initialized
},
{  SOUNDDLLPRO_CMD_CHANNELSOLO,                                         // cmd
   "Name> " SOUNDDLLPRO_CMD_CHANNELSOLO "\n"                            // help
   "Help> sets solo status of one or more output channels and returns current\n"
   "      status. NOTE: muting/unmuting is not ramped!\n"
   "      NOTE: 'solo' status supersedes 'mute' status: if solo status of any\n"
   "      output is '1', then mute status of all outputs is ignored!\n"
   "Par.> value:     vector/array with solo values (0 for unsolo or 1 for solo).\n"
   "                 If no value is specified, no solo values are changed,\n"
   "                 current values are returned. Either one value must be\n"
   "                 specified (applied to all channels) or lengths of solo\n"
   "                 and output vector/array must be identical (values applied\n"
   "                 in corresponding order to specified channels).\n"
   "     output:     vector/array with output channels (indices or array with\n"
   "                 names) to apply values (no duplicates allowed)\n"
   "Def.> value:     current solo values (no changes)\n"
   "     output:     vector/array with all output channels\n"
   "Ret.> value:     vector/array with current solo values for all output channels",
   SOUNDDLLPRO_PAR_VALUE ","                                            // arguments
   SOUNDDLLPRO_PAR_OUTPUT ",",
   ChannelMuteSolo,                                                     // function pointer
   1                                                                    // must be initialized
},
{  SOUNDDLLPRO_CMD_RECSOLO,                                             // cmd
   "Name> " SOUNDDLLPRO_CMD_RECSOLO "\n"                                // help
   "Help> sets solo status of one or more input channels and returns current\n"
   "      status. NOTE: muting/unmuting is not ramped!\n"
   "      NOTE: 'solo' status supersedes 'mute' status: if solo status of any\n"
   "      input is '1', then mute status of all inputs is ignored!\n"
   "Par.> value:     vector/array with solo values (0 for unsolo or 1 for solo).\n"
   "                 If no value is specified, no solo values are changed,\n"
   "                 current values are returned. Either one value must be\n"
   "                 specified (applied to all channels) or lengths of solo\n"
   "                 and input vector/array must be identical (values applied in\n"
   "                 corresponding order to specified channels).\n"
   "      input:     vector/array with input channels (indices or array with \n"
   "                 names) to apply values (no duplicates allowed)\n"
   "Def.> value:     current solo values (no changes)\n"
   "      input:     vector/array with all input channels\n"
   "Ret.> value:     vector/array with current solo values for all input channels",
   SOUNDDLLPRO_PAR_VALUE ","                                            // arguments
   SOUNDDLLPRO_PAR_INPUT ",",
   ChannelMuteSolo,                                                     // function pointer
   1                                                                    // must be initialized
},
{  SOUNDDLLPRO_CMD_TRACKNAME,                                                // cmd
   "Name> " SOUNDDLLPRO_CMD_TRACKNAME "\n"                                   // help
   "Help> sets symbolic name of one or more output tracks and returns current\n"
   "      names. These names can be used in all commands using tracks instead\n"
   "      of their indices.\n"
   "Par.> track:     vector/array with output tracks (indices or array with\n"
   "                 names, no duplicates allowed)\n"
   "      name:      vector/array with names to be set. Number of names must be\n"
   "                 identical to number of tracks or must be empty.\n"
   "                 No duplicate names allowed. If empty, only current names\n"
   "                 are returned.\n"
   "Def.> track:     vector/array with all tracks\n"
   "      name:      empty\n"
   "Ret.> name:      vector/array with symbolic names of output tracks",
   SOUNDDLLPRO_PAR_NAME ","                                           // argument
   SOUNDDLLPRO_PAR_TRACK ",",
   ChannelName,                                                                // function pointer
   1                                                                    // must be initialized
},
{  SOUNDDLLPRO_CMD_CHANNELNAME,                                                // cmd
   "Name> " SOUNDDLLPRO_CMD_CHANNELNAME "\n"                                   // help
   "Help> sets symbolic name of one or more output channels and returns current\n"
   "      names. These names can be used in all commands using outputs instead\n"
   "      of their indices.\n"
   "Par.> output:    vector/array with output channels (indices or array with\n"
   "                 names, no duplicates allowed)\n"
   "      name:      vector/array with names to be set. Number of names must be\n"
   "                 identical to number of channels or must be empty.\n"
   "                 No duplicate names allowed. If empty, only current names\n"
   "                 are returned.\n"
   "Def.> output:    vector/array with all output channels\n"
   "      name:      empty\n"
   "Ret.> name:      vector/array with symbolic names of output channels",
   SOUNDDLLPRO_PAR_NAME ","                                           // argument
   SOUNDDLLPRO_PAR_OUTPUT ",",
   ChannelName,                                                                // function pointer
   1                                                                    // must be initialized
},
{  SOUNDDLLPRO_CMD_RECNAME,                                                // cmd
   "Name> " SOUNDDLLPRO_CMD_RECNAME "\n"                                   // help
   "Help> sets symbolic name of one or more input channels and returns current\n"
   "      names. These names can be used in all commands using inputs instead\n"
   "      of their indices.\n"
   "Par.> input:     vector/array with input channels (indices or array with\n"
   "                 names, no duplicates allowed)\n"
   "      name:      vector/array with names to be set. Number of names must be\n"
   "                 identical to number of channels or must be empty.\n"
   "                 No duplicate names allowed. If empty, only current names\n"
   "                 are returned.\n"
   "Def.> input:     vector/array with all input channels\n"
   "      name:      empty\n"
   "Ret.> name:      vector/array with symbolic names of input channels",
   SOUNDDLLPRO_PAR_NAME ","                                           // argument
   SOUNDDLLPRO_PAR_INPUT ",",
   ChannelName,                                                                // function pointer
   1                                                                    // must be initialized
},
{  SOUNDDLLPRO_CMD_LOADMEM,                                             // cmd
   "Name> " SOUNDDLLPRO_CMD_LOADMEM "\n"                                // help
   "Help> loads audio data to one or more channels\n"
   "      NOTE: while the device is running you may load more data to any track\n"
   "      at every time. However, if you load too many data segments (file or mem)\n"
   "      you may produce heavy memory load that may result in dropouts or even\n"
   "      crashes. Use the command 'trackload' to check, how many data segments\n"
   "      are currently pre-loaded to the tracks and adjust the 'speed' of loading\n"
   "      new data on-the-fly if necessary. On loading new data segments all data\n"
   "      that are played (not in use any more) are removed automatically.\n"
   "      Important not for Python: SoundMexPro expects the data for multi-channel\n"
   "      data arrays to be sorted non-interleaved in memory. Where MATLAB adjusts\n"
   "      the order in memory when transposing a matrix, this is NOT the case\n"
   "      with Python: transposing numpy ndarray only changes striping, the order\n"
   "      in memory stays unchanged. Especially when reading wave files to memory,\n"
   "      the channels will end up interleaved in memory! You may switch the order\n"
   "      in memory using the function 'numpy.asfortranarray' (see SoundMexPro\n"
   "      tutorials for examples). The same holds for arrays that are created\n"
   "      in code: be sure, that the memory order will be non-interleaved when\n"
   "      passing data to SoundMexPro!\n"
   "Par.> data:      matrix with one or more columns of data (mandatory).\n"
   "                 IMPORTANT NOTE: matrices with more than one channel are\n"
   "                 loaded 'aligned' to the specified tracks to keep the columns\n"
   "                 synchronous for playback, i.e. there may be zeros prepended\n"
   "                 to one or more columns if necessary!\n"
   "      track:     vector/array with tracks (indices or array with names),\n"
   "                 where data to be played (no duplicates allowed). The number\n"
   "                 of tracks must be a multiple of the number of data channels\n"
   "                 (columns of 'data'). If tracks devided by channels is > 1, \n"
   "                 the data are loaded circular to the specified tracks, e.g.\n"
   "                 loading a data matrix with two columns and specifying \n"
   "                 [0 1 4 7] in 'track', then the data columns are loaded as\n"
   "                 follows:\n"
   "                    column 0 -> track 0\n"
   "                    column 1 -> track 1\n"
   "                    column 0 -> track 4\n"
   "                    column 1 -> track 7,\n"
   "                 or loading a mono matrix with an empty 'track' argument\n"
   "                 will load that data to all tracks.\n"
   "      loopcount: number of times the data are to be played. NOTE: 0 is an\n"
   "                 endless loop!\n"
   "      offset:    number of zero samples to be played in the beginning\n"
   "    startoffset: number of samples to be skipped from data when playing the\n"
   "                 first loop. A value of -1 selects a random startoffset.\n"
   "      gain:      linear gain to be applied to each file sample.\n"
   "   crossfadelen: length in samples for a crossfade done with the object\n"
   "                 (vector or file) that was loaded BEFORE this object.\n"
   "                 If this object is set for the first vector in a track,\n"
   "                 it is ignored.\n"
   "      ramplen:   number of samples for fade in and fade out (hanning ramp)\n"
   "                 of 'complete' object, i.e. the first ramplen samples of\n"
   "                 playback are ramped up and the last ramplen samples\n"
   "                 (including all loops) are ramped down.\n"
   "                 NOTE: ramplen must not exceed half of total play length:\n"
   "                       ramplen <= loopcount*length - startoffset\n"
   "    loopramplen: number of samples for fade in and fade out (hanning ramp)\n"
   "                 looping of an object, i.e. the first ramplen samples of\n"
   "                 each loop are ramped up, and the last ramplen samples are\n"
   "                 ramped down.\n"
   "                 NOTE: the first samples of first loop are NOT ramped up\n"
   "                 and the last samples of last loop are NOT ramped down. Use\n"
   "                 parameter 'ramplen' additionally for an overall ramp!\n"
   "  loopcrossfade: if this value is set to 1 then a crossfade with a length of\n"
   "                 'loopramplen' samples is done on looping\n"
   "                 NOTE: this parameter is ignored if loopcount is 1 or\n"
   "                 loopramplen is 0\n"
   "                 IMPORTANT NOTE: this 'overlap' in crossfade mode changes the\n"
   "                 total playback length of your buffer to:\n"
   "                   (loopcount-1)*(length-loopramplen) + length\n"
   "      name:      optional name for the data object. Is used track view GUI\n"
   "                 to show names of used vectors.\n"
   "Def.> track:     vector/array with all tracks\n"
   "      loopcount: 1\n"
   "      offset:    0\n"
   "    startoffset: 0\n"
   "      gain:      1\n"
   "   crossfadelen: 0\n"
   "      ramplen:   0\n"
   "    loopramplen: 0\n"
   "  loopcrossfade: 0",
   SOUNDDLLPRO_PAR_DATA ","                                             // arguments
   SOUNDDLLPRO_PAR_TRACK ","
   SOUNDDLLPRO_PAR_SAMPLES ","
   SOUNDDLLPRO_PAR_CHANNELS ","
   SOUNDDLLPRO_PAR_LOOPCOUNT ","
   SOUNDDLLPRO_PAR_OFFSET ","
   SOUNDDLLPRO_PAR_STARTOFFSET ","
   SOUNDDLLPRO_PAR_GAIN ","
   SOUNDDLLPRO_PAR_NAME ","
   SOUNDDLLPRO_PAR_LOOPRAMPLEN ","
   SOUNDDLLPRO_PAR_RAMPLEN ","
   SOUNDDLLPRO_PAR_LOOPCROSSFADE ","
   SOUNDDLLPRO_PAR_CROSSFADELEN ",",
   LoadMem,                                                             // function pointer
   1                                                                    // must be initialized
},
{  SOUNDDLLPRO_CMD_LOADFILE,                                            // cmd
   "Name> " SOUNDDLLPRO_CMD_LOADFILE "\n"                               // help
   "Help> loads an audio file to one or more tracks, supported formats see PDF\n"
   "      documentation. NOTE: while the device is running you may load more\n"
   "      data to any track at every time. However, if you load too many data\n"
   "      segments (file or memory) you may produce heavy memory load that may\n"
   "      result in dropouts or even crashes. Use the command 'trackload' to\n"
   "      check, how many datasegments are currently pre-loaded to the tracks\n"
   "      and adjust the 'speed' of loading new data on-the-fly if necessary.\n"
   "      On loading new data segments all data that are played (not in use\n"
   "      any more) are removed automatically.\n"
   "Par.> filename:  filename of the audio file to load (mandatory).\n"
   "                 NOTE: this command does not load the complete file to\n"
   "                 memory, therefore it should be used rather than 'loadmem'\n"
   "                 on huge files. But if files are small and to be played in\n"
   "                 loop it is recommended to use 'wavread' and 'loadmem' to\n"
   "                 play data, because it is much more efficient to read from\n"
   "                 memory than to read from file on the fly.\n"
   "                 IMPORTANT NOTE: the channels of multichannel audio files\n"
   "                 are loaded 'aligned' to the specified tracks, i.e. there\n"
   "                 may be zeros prepended to one or more channels if\n"
   "                 necessary!\n"
   "      track:     vector/array with tracks, (indices or array with names)\n"
   "                 were data to be played (no duplicates >= 0 allowed). The\n"
   "                 number of tracks must be a multiple of the number of channels\n"
   "                 of the wave file. If tracks devided by channels is > 1, the\n"
   "                 data are loaded circular to the specified tracks, e.g.\n"
   "                 loading a wave file with two channels and specifying\n"
   "                 [0 1 4 7] in 'track', then the file channels are loaded as\n"
   "                 follows:\n"
   "                    channel 0 -> track 0\n"
   "                    channel 1 -> track 1\n"
   "                    channel 0 -> track 4\n"
   "                    channel 1 -> track 7,\n"
   "                 or loading a mono file with an empty 'track' argument\n"
   "                 will load that data to all tracks. For a wave file\n"
   "                 channel, that should _not_ be played on any track specify\n"
   "                 a negative value.\n"
   "                 This value is ignored if 'output' in command 'init' was -1.\n"
   "      loopcount: number of times the data are to be played. NOTE: 0 is an\n"
   "                  endless loop\n"
   "      offset:    number of zero samples to be played in the beginning\n"
   "    startoffset: number of samples to be skipped from data when playing the\n"
   "                 first loop. A value of -1 selects a random startoffset.\n"
   "                 NOTE: the samples of the file to be used are determined\n"
   "                 by the parameters 'fileoffset' and 'filelength':\n"
   "                 'fileoffset' sets the number of samples to be skipped from\n"
   "                 the start of the file. This applies for all played loops,\n"
   "                 i.e. the first 'fileoffset' samples of the file are never\n"
   "                 played.\n"
   "                 'length' sets the total length in samples to be used for\n"
   "                 each loop (see also description below).\n"
   "                 The 'range' of the file to be used is generated from these\n"
   "                 parameters and the file is looped if necessary (i.e. if\n"
   "                 'fileoffset' + 'length' > (filesize in samples)).\n"
   "                 You can set the start playback sample for the first\n"
   "                 loop within this range (!) with the parameter 'startoffset'.\n"
   "                 Must be between -1 (random) and 'length'.\n"
   "     fileoffset: number of samples to skip in the beginning. An offset of\n"
   "                 -1 starts at a random position within the file. NOTE: this\n"
   "                 applies for all loops, first 'fileoffset' samples of the\n"
   "                 file will never be used, see description of 'startoffset'!\n"
   "      length:    length in samples to play per loop. Must be between 0 and\n"
   "                 length of the used 'range' of the file in samples (see also\n"
   "                 description of 'startoffset'). 0 uses all samples starting\n"
   "                 at 'fileoffset' to the end of the file.\n"
   "      gain:      linear gain to be applied to each file sample.\n"
   "   crossfadelen: length in samples for a crossfade done with the object\n"
   "                 (vector or file) that was loaded BEFORE this object.\n"
   "                 If this object is set for the first vector in a track, it is\n"
   "                 ignored.\n"
   "      ramplen:   number of samples for fade in and fade out (hanning ramp)\n"
   "                 of 'complete' object, i.e. the first ramplen samples of\n"
   "                 playback are ramped up and the last ramplen samples\n"
   "                 (including all loops) are ramped down.\n"
   "                 NOTE: ramplen must not exceed half of total play length:\n"
   "                       ramplen <= loopcount*length - startoffset\n"
   "    loopramplen: number of samples for fade in and fade out (hanning ramp)\n"
   "                 for each loop of object, i.e. the first ramplen samples of\n"
   "                 each loop are ramped up, and the last ramplen samples are\n"
   "                 ramped down. If a startoffset > 0 is specified then the very\n"
   "                 first played ramplen samples are ramped up as well.\n"
   "                 NOTE: if fileoffset and length are specified, then the ramps\n"
   "                 apply for the 'snippet' defined by these parameters!\n"
   "  loopcrossfade: if this value is set to 1 then a crossfade with a length of\n"
   "                 'loopramplen' samples is done on looping\n"
   "                 NOTE: this parameter is ignored if loopcount is 1 or\n"
   "                 loopramplen is 0\n"
   "                 IMPORTANT NOTE: this means that the length of each loop will\n"
   "                 be shorter by 'loopramplen' samples than total sizes file.\n"
   "Def.> track:     vector/array with all tracks\n"
   "      loopcount: 1\n"
   "      offset:    0\n"
   "    startoffset: 0\n"
   "     fileoffset: 0\n"
   "      length:    0\n"
   "      gain:      1\n"
   "   crossfadelen: 0\n"
   "      ramplen:   0\n"
   "    loopramplen: 0\n"
   "  loopcrossfade: 0",
   SOUNDDLLPRO_PAR_FILENAME ","                                         // arguments
   SOUNDDLLPRO_PAR_TRACK ","
   SOUNDDLLPRO_PAR_LOOPCOUNT ","
   SOUNDDLLPRO_PAR_OFFSET ","
   SOUNDDLLPRO_PAR_STARTOFFSET ","
   SOUNDDLLPRO_PAR_FILEOFFSET ","
   SOUNDDLLPRO_PAR_LENGTH ","
   SOUNDDLLPRO_PAR_GAIN ","
   SOUNDDLLPRO_PAR_LOOPRAMPLEN ","
   SOUNDDLLPRO_PAR_RAMPLEN ","
   SOUNDDLLPRO_PAR_LOOPCROSSFADE ","
   SOUNDDLLPRO_PAR_CROSSFADELEN ",",
   LoadFile,                                                            // function pointer
   1                                                                    // must be initialized
},
{  SOUNDDLLPRO_CMD_CLEARDATA,                                           // cmd
   "Name> " SOUNDDLLPRO_CMD_CLEARDATA "\n"                              // help
   "Help> clears all loaded audio data and resets positions to zero.\n"
   "      If device is running, command is only allowed, if 'start' was called\n"
   "      with 'length' set to 0.\n",
   ",",                                                                 // arguments
   ClearData,                                                           // function pointer
   1                                                                    // must be initialized
},
{  SOUNDDLLPRO_CMD_CLEARTRACK,                                           // cmd
   "Name> " SOUNDDLLPRO_CMD_CLEARTRACK "\n"                              // help
   "Help> clears all loaded audio data on one or more tracks.\n"
   "      If device is running, command is only allowed, if 'start' was called\n"
   "      with 'length' set to 0.\n"
   "Par.> track:     vector/array with tracks (indices or array with names)\n"
   "                 to be cleared\n"
   "Def.> track:     empty\n",
   SOUNDDLLPRO_PAR_TRACK ",",
   ClearTrack,                                                           // function pointer
   1                                                                    // must be initialized
},
{  SOUNDDLLPRO_CMD_TRACKLOAD,                                           // cmd
   "Name> " SOUNDDLLPRO_CMD_TRACKLOAD "\n"                              // help
   "Help> retrieves the number of pending audio data 'buffers' (mem or file) for\n"
   "      all tracks. This command is intended for monitoring the current 'load'\n"
   "      of a track, especially if data are loaded 'on-the-fly' while the device\n"
   "      is running. If you load too many data segments (file or mem) you may\n"
   "      produce heavy memory load that may result in dropouts or even crashes.\n"
   "      So, this command can be used to adjust the 'speed' of loading new data\n"
   "      on-the-fly if necessary.\n"
   "Ret.> value:     vector/array with number of data segments that are currently\n"
   "                 pending for output on all tracks.",
   ",",                                                                 // arguments
   TrackLoad,                                                           // function pointer
   1                                                                    // must be initialized
},
{  SOUNDDLLPRO_CMD_WAIT,                                                // cmd
   "Name> " SOUNDDLLPRO_CMD_WAIT "\n"                                   // help
   "Help> waits for output on one or more track to be finished. NOTE: if a\n"
   "      track is specified, where an endless loop is running, an error is\n"
   "      returned!\n"
   "Par.> track:     vector/array with tracks (indices or array with names) to\n"
   "                 wait for (no duplicates allowed)\n"
   "      timeout:   timeout value. If a value > 0 is specified, the function\n"
   "                 returns with an error, if output was not finished within\n"
   "                 'value' milliseconds\n"
   "      mode:      'output' or 'stop'. If 'output' is set, the command waits\n"
   "                 until no more output data are pending for the corresponding\n"
   "                 track(s), i.e. it may return slightly before all data were\n"
   "                 really processed through soundcard. If 'stop' is set, then\n"
   "                 the command waits until the device is stopped automatically\n"
   "                 after playback is done (see parameters of command 'start')\n"
   "                 or if it is stopped by command 'stop' (e.g. from GUI). If\n"
   "                 'stop' is specified, then 'track' is neglected.\n"
   "Def.> track:     vector/array with all tracks\n"
   "      timeout:   0 (no timeout, i.e. endless waiting)\n"
   "      mode:      'output'",
   SOUNDDLLPRO_PAR_TIMEOUT ","
   SOUNDDLLPRO_PAR_TRACK ","                                            // arguments
   SOUNDDLLPRO_PAR_MODE ",",                                            // arguments
   Wait,                                                                // function pointer
   1                                                                    // must be initialized
},
{  SOUNDDLLPRO_CMD_PLAYING,                                             // cmd
   "Name> " SOUNDDLLPRO_CMD_PLAYING "\n"                                // help
   "Help> returns play status of all tracks\n"
   "Ret.> value:     vector/array with zeros and ones denoting corresponding play,\n"
   "                 status i.e. 1 if a track currently is playing data, or 0 if\n"
   "                 not. NOTE: the return value does _not_ show, if the device\n"
   "                 is running. If a channel is running but playing zeroes\n"
   "                 because no data were loaded the return value for the track\n"
   "                 will be '0'. To check if the device is (still) running use\n"
   "                  the command 'started'.\n"
   "      NOTE: this command will return '0' immediately after the data are\n"
   "      passed to the driver completely. If you want to wait for the playback\n"
   "      to be finished in 'autostop-mode' (see parameter 'length' for command\n"
   "      'start' when setting values other than 0), then you should used the\n"
   "      command 'started' to wait for device to be stopped.",
   "",                                                                  // arguments
   Playing,                                                             // function pointer
   1                                                                    // must be initialized
},
{  SOUNDDLLPRO_CMD_PLAYPOSITION,                                        // cmd
   "Name> " SOUNDDLLPRO_CMD_PLAYPOSITION "\n"                           // help
   "Help> Sets and returns current play position of device in samples (audible\n"
   "      samples). Setting the play position is only allowed, if parameter\n"
   "      'autocleardata' was set to 0 in command 'init', and if device is\n"
   "      paused with command 'pause'.\n"
   "      NOTE: since ASIO does blockwise audio processing, the returned\n"
   "      value always is a multiple of the current buffer size (not sample\n"
   "      accurate)!\n"
   "      IMPORTANT NOTE: if you use the playposition command to reset current\n"
   "      position in pause mode to position X, then the command will not (!)\n"
   "      return the position you have set, but a position Y that is smaller!!\n"
   "      When unpausing the device it will play (X-Y) samples of zeros before\n"
   "      the first requested sample at position X is audible. Use loadposition\n"
   "      to retrieve next audible sample in pause mode instead.\n"
   "      See also command 'loadposition'.\n"
   "Par.> position:  position in samples to set playback position to,\n"
   "Def.> position:  -1 (position not changed),\n"
   "Ret.> value:     current sample position of device",
   SOUNDDLLPRO_PAR_POSITION ",",                                        // arguments
   PlayPosition,                                                        // function pointer
   1                                                                    // must be initialized
},
{  SOUNDDLLPRO_CMD_LOADPOSITION,                                        // cmd
   "Name> " SOUNDDLLPRO_CMD_LOADPOSITION "\n"                           // help
   "Help> returns current loading position of device in samples. This is the\n"
   "      number of samples loaded up to now (therefore higher than the value\n"
   "      of 'playposition'). It's value is\n"
   "        'playposition' + ASIO buffersize * numbufs ('numbufs' see 'init').\n"
   "      NOTE: since ASIO does blockwise audio processing, the returned\n"
   "      value always is a multiple of the current buffer size (not sample\n"
   "      accurate)!\n"
   "Ret.> value:     current loading position of device",
   "",                                                                  // arguments
   LoadPosition,                                                        // function pointer
   1                                                                    // must be initialized
},
{  SOUNDDLLPRO_CMD_VOLUME,                                              // cmd
   "Name> " SOUNDDLLPRO_CMD_VOLUME "\n"                                 // help
   "Help> sets volume and returns current volume of output channels\n"
   "Par.> value:     vector/array with volumes (linear gains). If no value is\n"
   "                 specified, no volume is changed, current volumes are\n"
   "                 returned. Either one volume must be specified (applied\n"
   "                 to all channels) or lengths of volume and channel vector/array\n"
   "                 must be identical (gains applied in corresponding order\n"
   "                 to specified channels). NOTE: this volume is the 'master'\n"
   "                 volume, i.e. it is applied after all signal processing\n"
   "                 and mixing as a linear factor to each sample\n"
   "      channel:   vector/array with output channels (indices or array with\n"
   "                 names) to apply volume to (no duplicates allowed)\n"
   "Def.> value:     current volumes (no changes)\n"
   "      channel:   vector/array with all allocated channels\n"
   "Ret.> value:     vector/array with current volumes for all allocated channels",
   SOUNDDLLPRO_PAR_VALUE ","                                            // arguments
   SOUNDDLLPRO_PAR_CHANNEL ",",
   Volume,                                                              // function pointer
   1                                                                    // must be initialized
},
{  SOUNDDLLPRO_CMD_TRACKVOLUME,                                         // cmd
   "Name> " SOUNDDLLPRO_CMD_TRACKVOLUME "\n"                            // help
   "Help> sets volume and returns current volume of tracks\n"
   "Par.> value:     vector/array with volumes (linear gains). If no value is\n"
   "                 specified, no volume is changed, current volumes are\n"
   "                 returned. Either one volume  must be specified (applied\n"
   "                 to all tracks) or lengths of volume and track vector/array\n"
   "                 must be identical (gains applied in corresponding order\n"
   "                 to specified tracks).\n"
   "      track:     vector/array with tracks (indices or array with names) to\n"
   "                 apply volume (no duplicates allowed)\n"
   "      ramplen:   ramplength in samples to use for fading from current\n"
   "                 volume to new volume.\n"
   "Def.> value:     current track volumes (no changes)\n"
   "      track:     vector/array with all tracks\n"
   "      ramplen:   0\n"
   "Ret.> value:     vector/array with current volumes for all tracks",
   SOUNDDLLPRO_PAR_VALUE ","                                            // arguments
   SOUNDDLLPRO_PAR_TRACK ","
   SOUNDDLLPRO_PAR_RAMPLEN ",",
   TrackVolume,                                                         // function pointer
   1                                                                    // must be initialized
},
{  SOUNDDLLPRO_CMD_DEBUGSAVE,                                           // cmd
   "Name> " SOUNDDLLPRO_CMD_DEBUGSAVE "\n"                              // help
   "Help> sets debugsave mode and returns current mode\n"
   "Par.> value:     vector/array with ones and zeroes. If no value is specified,\n"
   "                 no mode is changed, current modes are returned. Either\n"
   "                 one mode must be specified (applied to all channels) or\n"
   "                 lengths of mode vector/array and channel vector/array must be\n"
   "                 identical (modes applied in corresponding order to\n"
   "                 specified channels). 1 sets enables debug saving: a file\n"
   "                 named 'out_?.wav' is created where '?' is the channel\n"
   "                 number. The output data are saved to that file before\n"
   "                 sending them to soundcard. NOTE: 'volume' and any ramps\n"
   "                 or applied after saving!. 0 disables debug saving.\n"
   "                 NOTE: if you switch debugsaving off, then NO files will\n"
   "                 be saved in file2file mode as well!\n"
   "      channel:   vector/array with channels (indices or array with names) to\n"
   "                 apply mode (no duplicates allowed)\n"
   "Def.> value:     current modes (no changes)\n"
   "      channel:   vector/array with all allocated channels\n"
   "Ret.> value:     vector/array with current debug modes for all allocated\n"
   "                 channels",
   SOUNDDLLPRO_PAR_VALUE ","                                            // arguments
   SOUNDDLLPRO_PAR_CHANNEL ",",
   DebugSave,                                                           // function pointer
   1                                                                    // must be initialized
},
{  SOUNDDLLPRO_CMD_DEBUGFILENAME,                                         // cmd
   "Name> " SOUNDDLLPRO_CMD_DEBUGFILENAME "\n"                            // help
   "Help> sets one or more debug filenames for one or more channels and\n"
   "      returns current name(s). NOTE: value can only be set if device is\n"
   "      stopped! If an invalid filename is passed, or a filename that is\n"
   "      already used by another channel you may get errors on 'start'!\n"
   "      Default names are 'out_?.wav' where ? is the channel number.\n"
   "      NOTE: debug files are always overwritten!\n"
   "      NOTE: debug files are always stored as normalized 32-bit float\n"
   "      PCM wave files.\n"
   "      NOTE: never write files directly to network drives or other slow\n"
   "      drives! This may cause dropouts (xruns)!\n"
   "Par.> filename:  array with one or more filenames for debug-data.\n"
   "                 The number of filenames must be identical to the number\n"
   "                 of channels specified in 'channel'\n"
   "      channel:   vector/array with channels (indices or array with names),\n"
   "                 were filenames to be set\n"
   "Def.> channel:   vector/array with all allocated output channels\n"
   "Ret.> value:     array with current record file names for all allocated\n"
   "                 output channels",
   SOUNDDLLPRO_PAR_FILENAME ","                                         // arguments
   SOUNDDLLPRO_PAR_CHANNEL ",",
   DebugFileName,                                                         // function pointer
   1                                                                    // must be initialized
},
{  SOUNDDLLPRO_CMD_F2FFILENAME,                                         // cmd
   "Name> " SOUNDDLLPRO_CMD_F2FFILENAME "\n"                            // help
   "Help> sets one or more file2file-filenames for one or more channels and\n"
   "      returns current name(s). NOTE: value can only be set if device is\n"
   "      stopped! If an invalid filename is passed, or a filename that is\n"
   "      already used by another channel you may get errors on 'start'!\n"
   "      Default names are 'f2f_?.wav' where ? is the channel number.\n"
   "      NOTE: file2file-files are always overwritten!\n"
   "      NOTE: file2file-files are always stored as normalized 32-bit float\n"
   "      PCM wave files.\n"
   "Par.> filename:  array with one or more filenames for file2file-data.\n"
   "                 The number of filenames must be identical to the number\n"
   "                 of channels specified in 'channel'\n"
   "      channel:   vector/array with channels (indices or array with names),\n"
   "                 were filenames to be set\n"
   "Def.> channel:   vector/array with all allocated output channels\n"
   "Ret.> value:     array with current record file names for all allocated\n"
   "                 output channels",
   SOUNDDLLPRO_PAR_FILENAME ","                                         // arguments
   SOUNDDLLPRO_PAR_CHANNEL ",",
   DebugFileName,                                                         // function pointer
   1                                                                    // must be initialized
},
{  SOUNDDLLPRO_CMD_RECORDING,                                           // cmd
   "Name> " SOUNDDLLPRO_CMD_RECORDING "\n"                              // help
   "Help> returns record status of all allocated input channels\n"
   "Ret.> value:     vector/array with zeros and ones denoting corresponding\n"
   "                 record status, i.e. 1 if channel is currently recording\n"
   "                 data, or 0 if not. NOTE: all allocated channels do always\n"
   "                 record data from the soundcard, but 'recording' determines\n"
   "                 if the data are currently saved to disk",
   "",                                                                  // arguments
   Recording,                                                           // function pointer
   1                                                                    // must be initialized
},
{  SOUNDDLLPRO_CMD_RECVOLUME,                                              // cmd
   "Name> " SOUNDDLLPRO_CMD_RECVOLUME "\n"                                 // help
   "Help> sets recording volume and returns current recording volume of input\n"
   "      channels.\n"
   "Par.> value:     vector/array with volumes (linear gains). If no value is\n"
   "                 specified, no volume is changed, current volumes are\n"
   "                 returned. Either one volume must be specified (applied\n"
   "                 to all channels) or lengths of volume and channel vector/array\n"
   "                 must be identical (gains applied in corresponding order\n"
   "                 to specified channels). NOTE: this volume is the 'master'\n"
   "                 volume, i.e. it is applied before all signal processing\n"
   "                 and mixing as a linear factor to each sample\n"
   "                 IMPORTANT NOTES: this volume is applied to each recorded\n"
   "                 sample BEFORE any signal processing, mixing aor threshold\n"
   "                 determination is done (see also command 'recthreshold').\n"
   "                 Clipping is checked BEFORE applying the volume. This gain\n"
   "                 is NOT ramped.\n"
   "      channel:   vector/array with input channels (indices or array with)\n"
   "                 names to apply volume to (no duplicates allowed)\n"
   "Def.> value:     current volumes (no changes)\n"
   "      channel:   vector/array with all allocated channels\n"
   "Ret.> value:     vector/array with current volumes for all allocated channels",
   SOUNDDLLPRO_PAR_VALUE ","                                            // arguments
   SOUNDDLLPRO_PAR_CHANNEL ",",
   RecVolume,                                                              // function pointer
   1                                                                    // must be initialized
},
{  SOUNDDLLPRO_CMD_RECFILENAME,                                         // cmd
   "Name> " SOUNDDLLPRO_CMD_RECFILENAME "\n"                            // help
   "Help> sets one or more record filenames for one or more channels and\n"
   "      returns current name(s). NOTE: value can only be set if device is\n"
   "      stopped!\n"
   "      NOTE: you cannot use the same filename for different channels, only\n"
   "      one mono file per channel can be written!\n"
   "      If an invalid filename is passed, or a filename that is\n"
   "      already used by another channel you may get errors on 'start'!\n"
   "      You can set/change the filename of a channel either if device is\n"
   "      stopped, or if the corresponding input channel(s) are paused with\n"
   "      command 'recpause'. If you set a record filename for a paused input\n"
   "      channel, the current record file (filename before setting the new\n"
   "      one) is closed and the new one created. If a record length was set\n"
   "      with command 'reclength' the new file again will only record the\n"
   "      specified number of samples.\n"
   "      NOTE: record files are always overwritten, i.e. if the same name\n"
   "      is specified 'again' in pause mode the file is overwritten directly!\n"
   "      NOTE: recorded data are always stored in normalized 32-bit float\n"
   "      PCM wave files.\n"
   "      NOTE: never store record files directly on network drives or other\n"
   "      slow drives! This may cause dropouts (xruns)!\n"
   "      To disable recording to file for one or more channels use the command\n"
   "      'recpause'.\n"
   "Par.> filename:  array with one or more filenames for recording data.\n"
   "                 The number of filenames must be identical to the number\n"
   "                 of channels specified in 'channel'\n"
   "      channel:   vector/array with channels (indices or array with names),\n"
   "                 were filenames to be set\n"
   "Def.> channel:   vector/array with all allocated input channels\n"
   "Ret.> value:     array with current record file names for all allocated\n"
   "                 input channels",
   SOUNDDLLPRO_PAR_FILENAME ","                                         // arguments
   SOUNDDLLPRO_PAR_CHANNEL ",",
   RecFileName,                                                         // function pointer
   1                                                                    // must be initialized
},
{  SOUNDDLLPRO_CMD_RECPAUSE,                                            // cmd
   "Name> " SOUNDDLLPRO_CMD_RECPAUSE "\n"                               // help
   "Help> sets recording pause status of one or more channels and returns\n"
   "      current recording pause status. NOTE: this only pauses recording to\n"
   "      file, recording to buffer (for retrieving record data with command\n"
   "      'recgetdata') is always enabled. So this command may be used also to\n"
   "       disable recording to file completely.\n"
   "      NOTE: setting recpause to 1 does not close the recording file, it only\n"
   "      disables writing to it (temporarily). If you want to read the file in\n"
   "      pause mode you have to change the current recfilename in recpause mode\n"
   "      with command 'recfilename': this will close the file(s)!\n" 
   "Par.> value:     vector/array with 1 (pauses record of channel) or 0 (resumes\n"
   "                 recording)\n"
   "      channel:   vector/array with channels (indices or array with names),\n"
   "                 were values to apply to\n"
   " closerecfile:   if set to 1 the recording file is closed if recording is\n"
   "                 paused. On resuming a new file is created (existing data\n"
   "                 are overwritten),\n"
   "Def.> value:     current values (no change)\n"
   "      channel:   vector/array with all allocated input channels\n"
   " closerecfile:   0\n"
   "Ret.> value:     vector/array with current recording pause status for all\n"
   "                 allocated input channels",
   SOUNDDLLPRO_PAR_VALUE ","                                           // arguments
   SOUNDDLLPRO_PAR_CHANNEL ","
   SOUNDDLLPRO_PAR_CLOSERECFILE ",",
   RecPause,                                                               // function pointer
   1                                                                    // must be initialized
},
{  SOUNDDLLPRO_CMD_RECPOSITION,                                         // cmd
   "Name> " SOUNDDLLPRO_CMD_RECPOSITION "\n"                            // help
   "Help> returns record position of all allocated output channels\n"
   "Ret.> value:     vector/array with number of samples recorded on each\n"
   "                 allocated input channel. NOTE: all allocated channels\n"
   "                 do always record data from the soundcard, so the absolute\n"
   "                 recording position is determined by 'playposition'. This\n"
   "                 command returns how many samples are saved to disk for each\n"
   "                 channel. During recording saving might be enabled and\n"
   "                 disabled multiple times, 'recposition' returns the total\n" 
   "                 number of saved samples",
   "",                                                                  // arguments
   RecPosition,                                                         // function pointer
   1                                                                    // must be initialized
},
{  SOUNDDLLPRO_CMD_RECTHRSHLD,                                          // cmd
   "Name> " SOUNDDLLPRO_CMD_RECTHRSHLD "\n"                             // help
   "Help> sets record threshold and returns current value\n"
   "Par.> value:     Thresholds  between 0 and 1, current value is returned.\n"
   "                 If no value is specified the current value is not changed.\n"
   "                 A value of 0 disables the threshold. Otherwise recording\n"
   "                 to file (!) starts with the first recorded buffer (not\n"
   "                 sample!) that contains data that exceed the threshold\n"
   "                 value with respect to the specified mode and channels.\n"
   "                 NOTE: threshold is resetted after exceeding it (set to 0)!\n"
   "      mode:      Flag, if the threshold must be exceeded in one (1) or\n"
   "                 all (0) of the channels specified in 'channel'. Must\n"
   "                 be 0 or 1.\n"
   "      channel:   vector/array with channels (indices or array with names) to\n"
   "                 check for the threshold (no duplicates allowed)\n"
   "Def.> value:     current thresholds (no changes, 0 on startup)\n"
   "      mode:      1\n"
   "      channel:   vector/array with all allocated channels\n"
   "Ret.> value:     current threshold value\n"
   "      mode:      current threshold mode",
   SOUNDDLLPRO_PAR_VALUE ","                                            // arguments
   SOUNDDLLPRO_PAR_MODE ","
   SOUNDDLLPRO_PAR_CHANNEL ",",
   RecThreshold,                                                        // function pointer
   1                                                                    // must be initialized
},
{  SOUNDDLLPRO_CMD_RECSTARTED,                                          // cmd
   "Name> " SOUNDDLLPRO_CMD_RECSTARTED "\n"                             // help
   "Help> returns, if recording to file (!) channels was ever started. This\n"
   "      command is especially useful when recording with threshold to check,\n"
   "      if the threshold value was ever exceeded (e.g. for implementing\n"
   "      timeouts).\n"
   "Ret.> value:     vector/array with ones (started) and zeros (not started)\n"
   "                 for all allocated input channels",
   "",                                                                  // arguments
   RecStarted,                                                          // function pointer
   1                                                                    // must be initialized
},
{  SOUNDDLLPRO_CMD_RECLEN,                                              // cmd
   "Name> " SOUNDDLLPRO_CMD_RECLEN "\n"                                 // help
   "Help> sets file record length and returns current values\n"
   "Par.> value:     vector/array with record lengths. If no value is specified, no\n"
   "                 value is changed, current values are returned. Either one\n"
   "                 value must be specified (applied to all channels) or\n"
   "                 lengths of value vector/array and channel vector/array must\n"
   "                 be identical (record length applied in corresponding order\n"
   "                 to specified channels). Recording to file on the \n"
   "                 corresponding channel is stopped after recording the\n"
   "                 specified number of samples (0 does endless recording).\n"
   "                 NOTE: this record length sets the length of the file (!)\n"
   "                 to be reorded only! If the recorded file exceeds this\n"
   "                 length only recording to file is deisabled, the device\n"
   "                 itself is _not_ stopped: playback and recording (to\n"
   "                 memory) is still ongoing!\n"
   "      channel:   vector/array with channels (indices or array with names) to\n"
   "                 apply record lengths to (no duplicates allowed)\n"
   "Def.> value:     current record lengths (no changes, 0 on startup)\n"
   "      channel:   vector/array with all allocated channels\n"
   "Ret.> value:     vector/array with current file record lengths for all\n"
   "                 allocated channels",
   SOUNDDLLPRO_PAR_VALUE ","                                            // arguments
   SOUNDDLLPRO_PAR_CHANNEL ",",
   RecLength,                                                           // function pointer
   1                                                                    // must be initialized
},
{  SOUNDDLLPRO_CMD_RECBUFSIZE,                                          // cmd
   "Name> " SOUNDDLLPRO_CMD_RECBUFSIZE "\n"                             // help
   "Help> sets record buffer sizes and returns current values\n"
   "Par.> value:     vector/array with buffer sizes. If no value is specified, no\n"
   "                 value is changed, current values are returned. This value\n"
   "                 specifies, how many recorded samples of a channel are\n"
   "                 buffered in memory and can be retrieved with command\n"
   "                 'recgetdata'. NOTE: values smaller than the current\n"
   "                 buffersize of the device are adjusted to that buffersize\n"
   "                 (except 0 which disables buffering). Either one value\n"
   "                 must be specified (applied to all channels) or lengths\n"
   "                 of value vector/array and channel vector/array must be\n"
   "                 identical (buffer size applied in corresponding order to\n"
   "                 specified channels).\n"
   "                 NOTE: the requested buffer size has to be allocated from\n"
   "                 memory for each recording channel. So take care that not\n"
   "                 more memory is requested than available. In general more\n"
   "                 than a few minutes should not be set as buffer size!\n"
   "      channel:   vector/array with channels (indices or array with names) to\n"
   "                 apply buffer sizes to (no duplicates allowed)\n"
   "Def.> value:     current record buffer sizes (no changes, 0 on startup)\n"
   "      channel:   vector/array with all allocated channels\n"
   "Ret.> value:     vector/array with record buffer size for all allocated\n"
   "                 input channels",
   SOUNDDLLPRO_PAR_VALUE ","                                            // arguments
   SOUNDDLLPRO_PAR_CHANNEL ",",
   RecBufferSize,                                                       // function pointer
   1                                                                    // must be initialized
},
{  SOUNDDLLPRO_CMD_RECGETDATA,                                          // cmd
   "Name> " SOUNDDLLPRO_CMD_RECGETDATA"\n"                              // help
   "Help> returns record data\n"
   "Par.> channel:   vector/array with channels (indices or array with names)\n"
   "                 to retrieve data from (no duplicates allowed) NOTE: data\n"
   "                 from multiple channels with different recbufsizes (set with\n"
   "                 'recbufsize') cannot be retrieved with a single command and\n"
   "                 must be retrieved subsequently\n"
   "Def.> channel:   vector/array with all allocated channels\n"
   "Ret.> data:      matrix/array with columns containing record data from\n"
   "                 channels (length is set by 'recbufsize' command),\n"
   "                 absolute record sample position of first sample in returned\n"
   "                 matrix.\n" 
   "                 IMPORTANT NOTE: this returned record sample position is the\n"
   "                 absolute position in time (i.e. recorded samples since the\n"
   "                 device is running). It will be identical to the position if\n"
   "                 recording to file. On each call the number of samples is\n"
   "                 returned that was specified in command 'recbufsize'.\n"
   "                 Therefore if 'recgetdata' is called before this number of\n"
   "                 samples is recorded at all, zeroes are prepended and the\n"
   "                 returned position will be negative. In subsequent calls to\n"
   "                 'recgetdata' you may retrieve overlapping data (if you are\n"
   "                 calling fast!), and thus the number of 'new' samples n\n"
   "                 (i.e. samples, that were not already retrieved in last call)\n"
   "                 can be calculated by the difference of the two retrieved\n"
   "                 positions p1 and p2:\n"
   "                    n = (p2 - p1).\n"
   "                 If this number is larger than your recbufsize, than you\n"
   "                 have missed data! Otherwise you can copy the new data with\n"
   "                 respect to the overlap: the last (recbufsize - n) samples\n"
   "                 in the first buffer are identical to the first (recbufsize - n)\n"
   "                 samples in the second buffer and you may skip them.",
   SOUNDDLLPRO_PAR_CHANNEL ","
   SOUNDDLLPRO_PAR_DATA ","
   SOUNDDLLPRO_PAR_DATADEST ","
   SOUNDDLLPRO_PAR_SAMPLES ","
   SOUNDDLLPRO_PAR_CHANNELS ",",                                         // arguments
   RecGetData,                                                          // function pointer
   1                                                                    // must be initialized
},
{  SOUNDDLLPRO_CMD_PLUGINSETDATA,                                       // cmd
   "Name> " SOUNDDLLPRO_CMD_PLUGINSETDATA "\n"                          // help
   "Help> sets current plugin user data\n"
   "      NOTE: this command is not supported in Python\n"
   "Par.> data:      matrix with user data (mandatory). A matrix with the n\n"
   "                 columns and 100 rows must be specified, where n is the\n"
   "                 number of allocated input or output channels respectively.\n"
   "      mode:      'input' or 'output' to set input or output user data\n"
   "Def.> mode:      'output'",
   SOUNDDLLPRO_PAR_DATA ","                                             // arguments
   SOUNDDLLPRO_PAR_MODE ","
   SOUNDDLLPRO_PAR_SAMPLES ","
   SOUNDDLLPRO_PAR_CHANNELS ",",
   PluginSetData,                                                       // function pointer
   1                                                                    // must be initialized
},
{  SOUNDDLLPRO_CMD_PLUGINGETDATA,                                       // cmd
   "Name> " SOUNDDLLPRO_CMD_PLUGINGETDATA "\n"                          // help
   "Help> retrieves current plugin user data\n"
   "      NOTE: this command is not supported in Python\n"
   "Par.> mode:      'input' or 'output' to retrieve input or output user data\n"
   "Def.> mode:      'output'\n"
   "Ret.> data:      matrix with current plugin user data for all allocated\n"
   "                 channels",
   SOUNDDLLPRO_PAR_MODE ","
   SOUNDDLLPRO_PAR_DATA ","
   SOUNDDLLPRO_PAR_SAMPLES ","
   SOUNDDLLPRO_PAR_CHANNELS ",",                                         // arguments
   PluginGetData,                                                       // function pointer
   1                                                                    // must be initialized
},
{  SOUNDDLLPRO_CMD_VSTQUERY,                                            // cmd
   "Name> " SOUNDDLLPRO_CMD_VSTQUERY "\n"                               // help
   "Help> returns information on a VST plugin by filenname or by type,\n"
   "      input and position\n"
   "Par.> filename:  filename of plugin to query. NOTE: if filename is\n"
   "                 specified, all other parameters are ignored!\n"
   "      type:      type of plugin to query. Allowed types are:\n"
   "                 master:  plugin loaded as master plugin,\n"
   "                 final:   plugin loaded as final plugin,\n"
   "                 track:   plugin loaded as track plugin,\n"
   "                 input:   plugin loaded as input/recording plugin,\n"
   "      input:     one of the plugins input channels specified when\n"
   "                 loading it with 'vstload',\n"
   "      position:  'vertical' position of plugin to unload.\n"
   "Def.> type:      'master'\n"
   "      position:  0\n"
   "Ret.> info:      array with effect name, product string and vendor as,\n"
   "                 returned from plugin\n" 
   "      input:     number of available inputs,\n"
   "      output:    number of available outputs,\n"
   "      programs:  array with available program names,\n"
   "      program:   name of current program,\n"
   "      parameter: array with available parameter names,\n"
   "      value:     vector/array with corresponding parameter values",
   SOUNDDLLPRO_PAR_FILENAME ","                                         // arguments
   SOUNDDLLPRO_PAR_TYPE ","
   SOUNDDLLPRO_PAR_INPUT ","
   SOUNDDLLPRO_PAR_POSITION ",",
   VSTQuery,                                                            // function pointer
   1                                                                    // must be initialized
},
{  SOUNDDLLPRO_CMD_VSTLOAD,                                             // cmd
   "Name> " SOUNDDLLPRO_CMD_VSTLOAD "\n"                                // help
   "Help> loads a VST plugin (optionally with config file).\n"
   "Par.> filename:  filename of plugin to load\n"
   "      type:      type of plugin to load. Allowed types are:\n"
   "                 master:  plugin is loaded as 'master plugin': it is\n"
   "                          plugged into 'hardware output channels', i.e.\n"
   "                          applied on track sums (after mixing) BEFORE the,\n"
   "                          script plugin\n"
   "                 final:   plugin is loaded as 'master plugin': it is\n"
   "                          plugged into 'hardware output channels', i.e.\n"
   "                          applied on track sums (after mixing) AFTER the,\n"
   "                          script plugin\n"
   "                 track:   plugin is loaded as 'track plugin': it is\n"
   "                          plugged into virtual tracks,\n"
   "                 input:   plugin is loaded as 'recording plugin': it is\n"
   "                          plugged into input channels,\n"
   "      input:     vector/array with input channels: hardware channels or tracks\n"
   "                 (plugin must support number of inputs). A value of -1\n"
   "                 configures a plugin input to be used as recurse input (see\n"
   "                 parameters 'recursechannel' and 'recursepos' below)\n"
   "      output:    vector/array with output channels: hardware channels or\n"
   "                 tracks (plugin must support number of outputs).\n"
   "                 If empty 'input' vector/array is used (input = output),\n"
   "      position:  'vertical' position. Each channel may contain five\n"
   "                 'vertical' plugins that are called subsequently.\n"
   "      recursechannel: An input of a plugin can be configured to receive so \n"
   "                 called 'recurse data' data rather than 'regular' input\n"
   "                 audio data from a channel or a track. This feature is\n"
   "                 intended for recursion e.g. for adaptive filter plugins\n"
   "                 that need 'a plugin output as an input'. To configure a\n"
   "                 plugins input for this purpose a value of '-1' must be\n"
   "                 specified for that plugin input in the parameter vector/array\n"
   "                 'input' (see above). The parameter vectors 'recursechannel'\n"
   "                 and 'recursepos' configure the 'source' for this plugin\n"
   "                 input by specifying the 'channel' (track or output channel,\n"
   "                 depending on plugin type) and the vertical position (see\n"
   "                 parameter 'position'), where the data are copied from. For\n"
   "                 a detailed example see the manual of SoundMexPro.\n"
   "                 NOTE: if the position of the source ('recursepos') is the\n"
   "                 same or higher than this plugin's position, then the source\n"
   "                 is 'behind' the input. Thus the data that will be received\n"
   "                 on this plugin's input will be the last block, i.e. the\n"
   "                 data are prom the past (one ASIO buffersize from the past)!\n"
   "                 NOTE: the number of values must be identical to the number\n"
   "                 of '-1' values passed to 'input'!\n"
   "     recursepos: vector/array specifying one or more 'positions' for recurse\n"
   "                 input configuration (see 'recursechannel'). If a value of '-1'\n"
   "                 is specified, this plugin's position is used (i.e. a direct\n"
   "                 recursion from the output of the plugin to one of it's\n"
   "                 inputs)\n"
   "                 NOTE: the number of values must be identical to the number\n"
   "                 of '-1' values passed to 'input'!\n"
   "      program:   program name to set.\n"
   "      programname: new name to set for current program. This name is set\n"
   "                 after 'program' was selected.\n"
   "     configfile: optional filename of config file to use (description of\n"
   "                 format see manual). NOTE: other parameters passed to\n"
   "                 command supersede corresponding entries in config file!\n"
   "Def.> type:      'master'\n"
   "      output:    input\n"
   "      position:  0\n"
   "      recursechannel: empty vector/array\n"
   "      recursepos: empty vector/array\n"
   "Ret.> type:      type of plugin (master, final, track or record),\n"
   "      input:     row vector/array with input channels,\n"
   "      output:    row vector/array with output channels,\n"
   "      position:  'vertical' position",
   SOUNDDLLPRO_PAR_FILENAME ","                                         // arguments
   SOUNDDLLPRO_PAR_TYPE ","
   SOUNDDLLPRO_PAR_INPUT ","
   SOUNDDLLPRO_PAR_OUTPUT ","
   SOUNDDLLPRO_PAR_RECURSECHANNEL ","
   SOUNDDLLPRO_PAR_RECURSEPOS ","
   SOUNDDLLPRO_PAR_POSITION ","
   SOUNDDLLPRO_PAR_PROGRAM ","
   SOUNDDLLPRO_PAR_PROGRAMNAME ","
   SOUNDDLLPRO_PAR_CONFIGFILE ",",
   VSTLoad,                                                             // function pointer
   1                                                                    // must be initialized
},
{  SOUNDDLLPRO_CMD_VSTUNLOAD,                                           // cmd
   "Name> " SOUNDDLLPRO_CMD_VSTUNLOAD "\n"                              // help
   "Help> unloads VST plugin.\n"
   "Par.> type:      type of plugin to unload. Allowed types are:\n"
   "                 master:  plugin is unloaded from master plugins,\n"
   "                 final:   plugin is unloaded from final plugins,\n"
   "                 track:   plugin is unloaded from track plugins,\n"
   "                 input:   plugin is unloaded from input plugins,\n"
   "      input:     one of the plugins input channels specified when\n"
   "                 loading it with 'vstload',\n"
   "                 NOTE: plugin is unloaded 'completely', it's removed\n"
   "                 from all channels, where it was loaded to!\n"
   "      position:  'vertical' position of plugin to unload.\n"
   "Def.> type:      'master'\n"
   "      position:  0",
   SOUNDDLLPRO_PAR_FILENAME ","                                         // arguments
   SOUNDDLLPRO_PAR_TYPE ","
   SOUNDDLLPRO_PAR_POSITION ","
   SOUNDDLLPRO_PAR_INPUT ",",
   VSTUnload,                                                           // function pointer
   1                                                                    // must be initialized
},
{  SOUNDDLLPRO_CMD_VSTPROGRAM,                                          // cmd
   "Name> " SOUNDDLLPRO_CMD_VSTPROGRAM "\n"                             // help
   "Help> sets and retrieves a program of a VST plugin.\n"
   "Par.> type:      type of plugin to query. Allowed types are:\n"
   "                 master:  master plugin is queried,\n"
   "                 final:   final plugin is queried,\n"
   "                 track:   track plugin is queried,\n"
   "                 input:   input plugin is queried,\n"
   "      input:     one of the plugins input channels specified when\n"
   "                 loading it with 'vstload'.\n"
   "      position:  'vertical' position of plugin to query.\n"
   "      program:   name of program to select. If empty, value is not changed.\n"
   "Def.> type:      'master'\n"
   "      position:  0\n"
   "Ret.> program:   current program name.",
   SOUNDDLLPRO_PAR_PROGRAM ","                                          // arguments
   SOUNDDLLPRO_PAR_TYPE ","
   SOUNDDLLPRO_PAR_POSITION ","
   SOUNDDLLPRO_PAR_INPUT ","
   SOUNDDLLPRO_PAR_USERCONFIG ",",
   VSTProgram,                                                          // function pointer
   1                                                                    // must be initialized
},
{  SOUNDDLLPRO_CMD_VSTPROGRAMNAME,                                      // cmd
   "Name> " SOUNDDLLPRO_CMD_VSTPROGRAMNAME "\n"                         // help
   "Help> sets and retrieves the name of the current program of a VST plugin.\n"
   "      NOTE: this command does not select a new program by name, it renames\n"
   "      the current program!\n"
   "      NOTE: the plugin itself has to support this renaming, otherwise the\n"
   "      command will fail.\n"
   "Par.> type:      type of plugin to query. Allowed types are:\n"
   "                 master:  master plugin is queried,\n"
   "                 final:   final plugin is queried,\n"
   "                 track:   track plugin is queried,\n"
   "                 input:   input plugin is queried,\n"
   "      input:     one of the plugins input channels specified when\n"
   "                 loading it with 'vstload'.\n"
   "      position:  'vertical' position of plugin to query.\n"
   "      programname: new name to set for current program. If empty, value is\n"
   "                 is not changed.\n"
   "Def.> type:      'master'\n"
   "      position:  0\n"
   "Ret.> program:   name of current program.",
   SOUNDDLLPRO_PAR_PROGRAMNAME ","                                      // arguments
   SOUNDDLLPRO_PAR_TYPE ","
   SOUNDDLLPRO_PAR_POSITION ","
   SOUNDDLLPRO_PAR_INPUT ",",
   VSTProgramName,                                                      // function pointer
   1                                                                    // must be initialized
},
{  SOUNDDLLPRO_CMD_VSTPARAM,                                            // cmd
   "Name> " SOUNDDLLPRO_CMD_VSTPARAM "\n"                               // help
   "Help> sets and retrieves one or more parameter values of a VST plugin\n"
   "Par.> type:      type of plugin to query. Allowed types are:\n"
   "                 master:  master plugin is queried,\n"
   "                 final:   final plugin is queried,\n"
   "                 track:   track plugin is queried,\n"
   "                 input:   input plugin is queried,\n"
   "      input:     one of the plugins input channels specified when\n"
   "                 loading it with 'vstload',\n"
   "      position:  'vertical' position of plugin to query.\n"
   "      parameter: parameter name(s) to set (array). If empty all\n"
   "                 parameters are queried/set.\n"
   "      value:     vector/array with values for parameters. All values must be\n"
   "                 between 0.0 and 1.0. If empty, values are not changed\n"
   "                 otherwise length must be identical to length of parameter\n"
   "                 array.\n"
   "Def.> type:      'master'\n"
   "      position:  0\n"
   "Ret.> parameter: array with queried parameter names,\n"
   "      value:     current values of queried parameters",
   SOUNDDLLPRO_PAR_PARAMETER ","                                        // arguments
   SOUNDDLLPRO_PAR_VALUE ","
   SOUNDDLLPRO_PAR_TYPE ","
   SOUNDDLLPRO_PAR_POSITION ","
   SOUNDDLLPRO_PAR_INPUT ",",
   VSTParameter,                                                        // function pointer
   1                                                                    // must be initialized
},
{  SOUNDDLLPRO_CMD_VSTSET,                                              // cmd
   "Name> " SOUNDDLLPRO_CMD_VSTSET "\n"                                 // help
   "Help> sets and retrieves program and/or parameters of VST plugin by\n"
   "      values in a config file\n"
   "Par.> type:      type of plugin to query. Allowed types are:\n"
   "                 master:  master plugin is queried,\n"
   "                 final:   final plugin is queried,\n"
   "                 track:   track plugin is queried,\n"
   "                 input:   input plugin is queried,\n"
   "      input:     one of the plugins input channels specified when\n"
   "                 loading it with 'vstload',\n"
   "      position:  'vertical' position of plugin to query.\n"
   "     configfile: filename of config file to use (description of format\n"
   "                 see manual). NOTE: other parameters passed to command\n"
   "                 supersede corresponding entries in config file!\n"
   "Def.> type:      'master'\n"
   "      position:  0\n"
   "Ret.> program:   current program\n"
   "      parameter: array with all (!) available parameter names\n"
   "      value:     vector/array with corresponding parameter values",
   SOUNDDLLPRO_PAR_CONFIGFILE ","
   SOUNDDLLPRO_PAR_TYPE ","
   SOUNDDLLPRO_PAR_POSITION ","
   SOUNDDLLPRO_PAR_INPUT ",",
   VSTSet,                                                              // function pointer
   1                                                                    // must be initialized
},
{  SOUNDDLLPRO_CMD_VSTSTORE,                                            // cmd
   "Name> " SOUNDDLLPRO_CMD_VSTSTORE "\n"                               // help
   "Help> stores current configuration of a VST plugin in a config file\n"
   "     (description of format see manual)\n"
   "Par.> type:      type of plugin to query. Allowed types are:\n"
   "                 master:  master plugin is queried,\n"
   "                 final:   final plugin is queried,\n"
   "                 track:   track plugin is queried,\n"
   "                 input:   input plugin is queried,\n"
   "      input:     one of the plugins input channels specified when\n"
   "                 loading it with 'vstload',\n"
   "      position:  'vertical' position of plugin to query.\n"
   "     configfile: filename of config file to write to. NOTE: and existing\n"
   "                 file will be overwritten!\n"
   "Def.> type:      'master'\n"
   "      position:  0",
   SOUNDDLLPRO_PAR_CONFIGFILE ","
   SOUNDDLLPRO_PAR_TYPE ","
   SOUNDDLLPRO_PAR_POSITION ","
   SOUNDDLLPRO_PAR_INPUT ",",
   VSTStore,                                                            // function pointer
   1                                                                    // must be initialized
},
{  SOUNDDLLPRO_CMD_VSTEDIT,                                             // cmd
   "Name> " SOUNDDLLPRO_CMD_VSTEDIT "\n"                                // help
   "Help> shows a GUI parameter editor of a VST plugin.\n"
   "Par.> type:      type of plugin to query. Allowed types are:\n"
   "                 master:  master plugin is queried,\n"
   "                 final:   final plugin is queried,\n"
   "                 track:   track plugin is queried,\n"
   "                 input:   input plugin is queried,\n"
   "      input:     one of the plugins input channels specified when\n"
   "                 loading it with 'vstload',\n"
   "      position:  'vertical' position of plugin to query.\n"
   "Def.> type:      'master'\n"
   "      position:  0",
   SOUNDDLLPRO_PAR_TYPE ","
   SOUNDDLLPRO_PAR_POSITION ","
   SOUNDDLLPRO_PAR_INPUT ",",
   VSTEdit,                                                             // function pointer
   1                                                                    // must be initialized
},
{  SOUNDDLLPRO_CMD_IOSTATUS,                                            // cmd
   "Name> " SOUNDDLLPRO_CMD_IOSTATUS "\n"                               // help
   "Help> sets I/O status of an input channel and returns current status\n"
   "Par.> input:     vector/array with input channels (indices or array with\n"
   "                 names) to map. This value is mandatory. If one channel is\n"
   "                 specified, then this input is mapped to all specified\n"
   "                 tracks. If more than one channel is specified then the\n"
   "                 number of tracks must be identical and the input channels\n"
   "                 are mapped to one track each in the specified order.\n"
   "      track:     vector/array with tracks (indices or array with names)\n"
   "                 for mapping (no duplicates allowed). The samples of the\n"
   "                 specified input channel are added to all specified tracks.\n"
   "                 Passing '-1' clears mapping for specified input channel.\n"
   "Def.> input:     if track is -1, then default is all available input channels\n"
   "Ret.> track:     vector/array containing tracks, where the samples from the\n"
   "                 input channel are added to. NOTE: this value is only\n"
   "                 returned, if one input channel is specified!",
   SOUNDDLLPRO_PAR_TRACK ","                                            // arguments
   SOUNDDLLPRO_PAR_INPUT ",",
   IOStatus,                                                            // function pointer
   1                                                                    // must be initialized
},
{  SOUNDDLLPRO_CMD_SETBUTTON,                                           // cmd
   "Name> " SOUNDDLLPRO_CMD_SETBUTTON "\n"                              // help
   "Help> enables button marking synchronized with playback. NOTE: after\n"
   "      marking and unmarking a particular button, the marking information\n"
   "      is resetted. To use the same marking information again you have to\n"
   "      call 'setbutton' again!!\n"
   "      NOTE: The MATLAB window containing the buttons must have a non-empty!\n"
   "            title (otherwise you will get an error 'value for field 'name'\n"
   "            is empty')!!\n"
   "      NOTE: only use this command from MATLAB (not with SoundDllLoader)!\n"
   "      NOTE: this command is not supported in Python\n"
      "Par.> handle:    (window) handle of the button\n"
   "      startpos:  starting point in samples\n"
   "      length:    marking length in samples\n"
   "      channel:   output channel with mix data to calculate samples\n"
   "Def.> channel:   0",
   SOUNDDLLPRO_PAR_LENGTH ","                                           // arguments
   SOUNDDLLPRO_PAR_STARTPOS ","
   SOUNDDLLPRO_PAR_NAME ","
   SOUNDDLLPRO_PAR_LEFT ","
   SOUNDDLLPRO_PAR_TOP ","
   SOUNDDLLPRO_PAR_WIDTH ","
   SOUNDDLLPRO_PAR_HEIGHT ","
   SOUNDDLLPRO_PAR_HANDLE ",",
   SetButton,                                                           // function pointer
   1                                                                    // must be initialized
},
{  SOUNDDLLPRO_CMD_DSPLOAD,                                             // cmd
   "Name> " SOUNDDLLPRO_CMD_DSPLOAD "\n"                                // help
   "Help> returns current and maximum dsp load that occurred. The dsp load is\n"
   "      the time consumed within a block for signal processing compared to\n"
   "      total available computing time for a block in percent.\n"
   "Ret.> value:     current dsp load,\n"
   "      maxvalue:  maximum dsp load since startup or last call to 'dsploadreset'",
   "",                                                                  // arguments
   DspLoad,                                                             // function pointer
   1                                                                    // must be initialized
},
{  SOUNDDLLPRO_CMD_DSPLOADRESET,                                        // cmd
   "Name> " SOUNDDLLPRO_CMD_DSPLOADRESET "\n"                           // help
   "Help> resets the dsp load maximum value (see command 'dspload').",
   "",                                                                  // arguments
   DspLoadReset,                                                        // function pointer
   1                                                                    // must be initialized
},
{  SOUNDDLLPRO_CMD_ADM,                                                 // cmd
   "Name> " SOUNDDLLPRO_CMD_ADM "\n"                                    // help
   "Help> interface to 'ASIO Direct Monitoring' for direct I/O wiring.\n"
   "      NOTE: successful call to command does not guarantee that command was\n"
   "      successfully done in driver. Drivers may ignore commands and/or\n"
   "      parameters that are not supported and return 'success' anyway!\n"
   "      Please see also the extra ADM chapter in the SoundMexPro manual!\n"
   "Par.> input:     input channel to monitor. -1 monitors all inputs (not\n"
   "                 supported by all drivers)\n"
   "      output:    output channel fpr monitoring the input. 'Snippet' from\n"
   "                 original ASIO help: 'Output is the base channel of a stereo\n"
   "                 channel pair, i.e. output is always an even channel (0,2...).\n"
   "                 If an odd input channel should be monitored and no panning\n"
   "                 or output routing can be applied, the driver has to use the\n"
   "                 next higher output (imply a hard right pan).'\n"
   "      gain:      gain to set ranging from 0 (-inf) to 2147483647 (12 dB, hex\n"
   "                 0x7fffffff), where 536870912 (hex 0x20000000) is 0 dB.\n"
   "                 NOTE: the 'gain' values may not be distibuted linearly\n"
   "                 neither on a dB nor on alinear loudness scale (depends on\n"
   "                 soundcard/driver)\n"
   "      pan:       pan to set, where 0 is 'left' and 2147483647 (hex 0x7fffffff)\n"
   "                 is right. NOTE: panning may be implemented different and\n"
   "                 may depend on 'output' (if odd or even channel used, see\n"
   "                 see above). Please check out behaviuor of your device!\n" 
   "      mode:      flag, what to set. 0 switches monitoring off, 1 switches\n"
   "                 monitoring on. NOTE: no other modes are part of the original\n"
   "                 ASIO interface. But some soundcard manufacturers support\n"
   "                 more undocumented modes (you may simply try it). The RME\n"
   "                 FireFace for example supports modes 2 and 3. Using these\n"
   "                 values, 'input' is ignored and the gain and pan of output\n"
   "                 channel 'output' are set instead of input channel 'input',\n"
   "                 where mode 2 switches 'off' (muted, i.e. gain 0), and\n"
   "                 mode 3 switches 'on', i.e. gain 'gain' is really set.\n" 
   "Def.> input:     0\n"
   "      output:    0\n"
   "      gain:      536870912 (hex 0x20000000, i.e. 0 dB)\n"
   "      pan:       1073741823 (hex 0x7fffffff/2, i.e. middle position)\n"
   "      mode:      1\n",
   SOUNDDLLPRO_PAR_INPUT ","                                            // arguments
   SOUNDDLLPRO_PAR_OUTPUT ","
   SOUNDDLLPRO_PAR_GAIN ","
   SOUNDDLLPRO_PAR_PAN ","
   SOUNDDLLPRO_PAR_MODE ",",
   AsioDirectMonitoring,                                                // function pointer
   1                                                                    // must be initialized
},
{  SOUNDDLLPRO_CMD_MIDIINIT,                                          // cmd
   "Name> " SOUNDDLLPRO_CMD_MIDIINIT "\n"                             // help
   "Help> initializes a MIDI device.\n"
   "Par.> driver:    name or index of MIDI driver to use\n"
   "Def.> driver:    0\n"
   "Ret.> driver:    name of used MIDI driver\n",
   SOUNDDLLPRO_PAR_DRIVER ",",                                           // arguments
   MIDIInit,                                                          // function pointer
   0                                                                    // must be initialized
},
{  SOUNDDLLPRO_CMD_MIDIEXIT,                                          // cmd
   "Name> " SOUNDDLLPRO_CMD_MIDIEXIT "\n"                             // help
   "Help> exits MIDI device.\n",
   "",                                                                  // arguments
   MIDIExit,                                                          // function pointer
   0                                                                    // must be initialized
},
{  SOUNDDLLPRO_CMD_MIDIGETDRIVERS,                                          // cmd
   "Name> " SOUNDDLLPRO_CMD_MIDIGETDRIVERS "\n"                             // help
   "Help> returns all available MIDI drivers\n"
   "Ret.> driver:    vector/array with MIDI driver names",
   "",                                                                  // arguments
   MIDIGetDrivers,                                                          // function pointer
   0                                                                    // must be initialized
},
{  SOUNDDLLPRO_CMD_MIDIPLAYNOTE,                                          // cmd
   "Name> " SOUNDDLLPRO_CMD_MIDIPLAYNOTE "\n"                             // help
   "Help> plays a note on MIDI device. Note is switched 'on' and 'off'\n"
   "      immediately afterwards again.\n"
   "Par.> note:   note to play between 0 and 127, where 0 is C-1 and 127\n"
   "              is G9 (example: 440 Hz (A4) is 69),\n"
   "      volume: volume (velocity) to use (between 0 and 127)\n"
   "      channel: MIDI channel to use (between 0 and 15)\n"
   "Def.> note:   mandatory\n"
   "      volume: 127\n"
   "      channel: 0\n",
   SOUNDDLLPRO_PAR_NOTE ","
   SOUNDDLLPRO_PAR_VOLUME ","
   SOUNDDLLPRO_PAR_CHANNEL ",",                                        // arguments
   MIDIPlayNote,                                                          // function pointer
   0                                                                    // must be initialized
},
{  SOUNDDLLPRO_CMD_MIDISHORTMSG,                                          // cmd
   "Name> " SOUNDDLLPRO_CMD_MIDISHORTMSG "\n"                             // help
   "Help> sends a message to MIDI device using API midiOutShortMsg. See\n"
   "      MIDI documentations for more information. All paramaters are \n"
   "      mandatory, all value must be between 0 and 255.\n"
   "Par.> status: status byte to send,\n"
   "      midi1:  first data byte to send,\n"
   "      midi2:  second data byte to send",
   SOUNDDLLPRO_PAR_STATUS ","
   SOUNDDLLPRO_PAR_MIDI1 ","
   SOUNDDLLPRO_PAR_MIDI2 ",",                                                                  // arguments
   MIDIShortMsg,                                                          // function pointer
   0                                                                    // must be initialized
},
{  SOUNDDLLPRO_CMD_ASYNCERROR,                                          // cmd
   "Name> " SOUNDDLLPRO_CMD_ASYNCERROR "\n"                             // help
   "Help> returns asynchroneous error status and error text\n"
   "Ret.> value:     1 if an asynchroneous error ocurred or 0 else,\n"
   "      error:     error text (if any)",
   "",                                                                  // arguments
   AsyncError,                                                          // function pointer
   1                                                                    // must be initialized
},
{  SOUNDDLLPRO_CMD_RESETERRORA,                                         // cmd
   "Name> " SOUNDDLLPRO_CMD_RESETERRORA "\n"                            // help
   "Help> resets asynchroneous error",
   "",                                                                  // arguments
   ResetError,                                                          // function pointer
   1                                                                    // must be initialized
},
{  SOUNDDLLPRO_CMD_CLIPTHRS,                                           // cmd
   "Name> " SOUNDDLLPRO_CMD_CLIPTHRS"\n"                               // help
   "Help> sets clipping threshold and returns current threshold values. The\n"
   "      clipping threshold is the normalized value (between 0 and 1) that\n"
   "      will be interpreted as clipping and thus will increase the clipcount\n"
   "      if exceeded. Default on startup is 1 for all channels (real digital\n"
   "      overdrive).\n"
   "Par.> type:      'input' or 'output' to set input (recording) threshold\n"
   "                 value(s) or output (playback) threshold value(s)\n"
   "      value:     vector/array with thresholds between 0 and 1. If no value is\n"
   "                 specified, no threshold is changed, current thresholds are\n"
   "                 returned. Either one threshold must be specified (applied\n"
   "                 to all channels) or lengths of threshold and channel vector\n"
   "                 must be identical (thresholds applied in corresponding order\n"
   "                 to specified channels).\n"
   "      channel:   vector/array with output channels (indices or array with\n"
   "                 names) to apply threshold to (no duplicates allowed)\n"
   "Def.> type:      'output'\n"
   "      value:     current threshold values (no changes)\n"
   "      channel:   vector/array with all allocated channels\n"
   "Ret.> value:     vector/array with current thresholds for all input or\n"
   "                 output channels",
   SOUNDDLLPRO_PAR_TYPE ","
   SOUNDDLLPRO_PAR_CHANNEL ","
   SOUNDDLLPRO_PAR_VALUE ",",
   ClipThreshold,                                                           // function pointer
   1                                                                    // must be initialized
},
{  SOUNDDLLPRO_CMD_CLIPCOUNT,                                           // cmd
   "Name> " SOUNDDLLPRO_CMD_CLIPCOUNT"\n"                               // help
   "Help> returns number of buffers (not samples!) where clipping occurred.\n"
   "      NOTE: for recording 'clipping' is defined as two consecutive full\n"
   "      scale samples. For the input channels clipping is checked before\n"
   "      any gain and signal processing (plugins) is applied to detect clipping\n"
   "      that occurred on D/A-conversion. If you use any plugins that amplify\n"
   "      the input, such clipping is _not_ detected by 'clipcount' command!\n"
   "Ret.> outut:  vector/array with number of buffers where clipping occurred\n"
   "              for each allocated ouptut channel,\n"
   "      input:  vector/array with number of buffers where clipping occurred\n"
   "              for each allocated input channel,\n"
   "      track:  vector/array with number of buffers where clipping occurred\n"
   "              for each allocated track",
   "",                                                                  // arguments
   ClipCount,                                                           // function pointer
   1                                                                    // must be initialized
},
{  SOUNDDLLPRO_CMD_RESETCLIPCOUNT,                                      // cmd
   "Name> " SOUNDDLLPRO_CMD_RESETCLIPCOUNT"\n"                          // help
   "Help> resets all clip counters to zero\n",
   "",                                                                  // arguments
   ResetClipCount,                                                      // function pointer
   1                                                                    // must be initialized
},
{  SOUNDDLLPRO_CMD_UNDERRUN,                                            // cmd
   "Name> " SOUNDDLLPRO_CMD_UNDERRUN "\n"                               // help
   "Help> returns underrun status of all tracks. 1 means that during playback\n"
   "      the corresponding track ran out of data.\n"
   "Ret.> value:  vector/array with zeros and ones denoting corresponding\n"
   "              underrun status, i.e. 1 if an underrun occurred on the track,\n"
   "              or 0 if not", 
   "",                                                                  // arguments
   Underrun,                                                            // function pointer
   1                                                                    // must be initialized
},
{  SOUNDDLLPRO_CMD_XRUN,                                                // cmd
   "Name> " SOUNDDLLPRO_CMD_XRUN "\n"                                   // help
   "Help> returns number of xruns occurred since last start\n"
   "Ret.> value:     total number of xruns occurred since last start,\n"
   "      xrunproc:  number of processing xruns occurred since last start,\n"
   "      xrundone:  number of xruns in visualization and recording to disk\n"
   "                 since last start.",
   "",                                                                  // arguments
   NumXRuns,                                                            // function pointer
   1                                                                    // must be initialized
},
{  SOUNDDLLPRO_CMD_BETATEST,                                               // cmd
   "Name> " SOUNDDLLPRO_CMD_BETATEST "\n"                                  // help
   "Help> Betatest command. No help",
   "",                                                                  // arguments
   BetaTest,                                                               // function pointer
   0                                                                    // must be initialized
},
{  SOUNDDLLPRO_CMD_HELP,                                                // cmd
   "Name> " SOUNDDLLPRO_CMD_HELP "\n"                                   // help
   "Help> prints help on command or command list\n"
   "Par.> help:   name of command of interest.\n"
   "              NOTE: wenn calling help from from MATLAB the name of the\n"
   "              parameter 'help' must be omitted, but when calling it from\n"
   "              SoundDllLoader it has to be used.\n",
   "",                                                                  // arguments
   NULL,                                                                // function pointer
   0                                                                    // must be initialized
},
{  SOUNDDLLPRO_CMD_HELPA,                                               // cmd
   "Name> " SOUNDDLLPRO_CMD_HELPA "\n"                                  // help
   "Help> prints help on command or command list\n"
   "Par.> help:   name of command of interest.\n"
   "              NOTE: wenn calling help from from MATLAB the name of the\n"
   "              parameter 'help' must be omitted, but when calling it from\n"
   "              SoundDllLoader it has to be used.\n",
   "",                                                                  // arguments
   NULL,                                                                // function pointer
   0                                                                    // must be initialized
},
{  SOUNDDLLPRO_CMD_SHOWERROR,                                           // cmd
   "Name> " SOUNDDLLPRO_CMD_SHOWERROR "\n"                              // help
   "Help> sets error handling/printing behavior of SoundMexPro and returns\n"
   "      current value\n"
   "      NOTE: this command is not supported in Python\n"
   "Par.> mode:   0: no errors are printed to MATLAB workspace\n"
   "              1: all errors are printed as warning to MATLAB workspace\n"
   "              2: MATLAB command 'error' is called with current errore\n"
   "Ret.> mode:   current value",
   "",                                                                  // arguments
   NULL,                                                                // function pointer
   0                                                                    // must be initialized
},
{  SOUNDDLLPRO_CMD_GETLASTERROR,                                        // cmd
   "Name> " SOUNDDLLPRO_CMD_GETLASTERROR "\n"                           // help
   "Help> returns last error\n"
   "Ret.> error:  last error as string",
   "",                                                                  // arguments
   NULL,                                                                // function pointer
   0                                                                    // must be initialized
},
{
   NULL,
   NULL,
   NULL,
   NULL,
   0
}
};
//------------------------------------------------------------------------------
#endif
