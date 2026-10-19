//------------------------------------------------------------------------------
/// \file VSTHost.h
/// \author Berg
/// \brief Implementation of class VSTHost. Implements a VST-host handling
/// multiple instances of TVSTHostPlugin in mutliple channels. Intended to
/// be used in conjunction with CAsio class and descendants.
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
#ifndef VSTHostH
#define VSTHostH

#include <vcl.h>
#include <vector>
#include "VSTHostPlugin.h"

//------------------------------------------------------------------------------

#define VSTHOST_NUM_PLUGINS   5



class SoundDllProMain;

//------------------------------------------------------------------------------
/// \class TVSTHostPluginInstance. Helper class for storing plugin instances an
/// references to them respectively
//------------------------------------------------------------------------------
class TVSTHostPluginInstance
{
   public:
      TVSTHostPluginInstance() : m_pPlugin(NULL), m_nReference(-1), m_bRecursionChannel(false){;}
      TVSTHostPlugin*            m_pPlugin;
      int                        m_nReference;
      bool                       m_bRecursionChannel;
};
//------------------------------------------------------------------------------

//------------------------------------------------------------------------------
/// enums for types of thread handling
/// references to them respectively
//------------------------------------------------------------------------------
enum TThreadingType
{
   TT_SINGLE = 0, // all plugins run in one thread
   TT_PLUGIN,     // each plugin runs in a separate thread
   TT_FIXNUM      // host uses a fix num of threads
};

//------------------------------------------------------------------------------
/// \class TVSTHost. Class for handling multiple instances of TVSTHostPlugin
/// in mutliple channels.
//------------------------------------------------------------------------------
class TVSTHost
{
   public:
      TVSTHost(UnicodeString usName, SoundDllProMain* phtSound, bool bTest = false);
      ~TVSTHost();
      void                 Init( unsigned int      nChannels,
                                 unsigned int      nBlockSize,
                                 TThreadingType    ttThreadingType,
                                 int               nThreadPriority);
      void                 Start();
      void                 Stop();
      TVSTHostPlugin*      LoadPlugin( UnicodeString usLibName,
                                       const std::vector<int>& viIn,
                                       const std::vector<int>& viOut,
                                       std::vector<TVSTNode>& vRecurse,
                                       const unsigned int& nLayer);
      void                 UnloadPlugin(const unsigned int& nLayer, const unsigned int& nChannel);
      TVSTHostPlugin*      Plugin(const unsigned int& nLayer, const unsigned int& nChannel, bool bNoError = false);
      bool                 IsPlugin(const unsigned int& nLayer, const unsigned int& nChannel);
      void                 Process(vvfVST& vvfBuffers);
      unsigned int         m_nChannels;
      unsigned int         m_nLayers;
      bool                 m_bStarted;
      UnicodeString        m_usName;

      static SoundDllProMain*     SoundInstance();
      // static host callback
      static VstIntPtr VSTCALLBACK VSTHostCallback (  AEffect* effect,
                                                      VstInt32 opcode,
                                                      VstInt32 index,
                                                      VstIntPtr value,
                                                      void* ptr,
                                                      float opt);
   protected:

   private:
      static SoundDllProMain*     sm_phtSound;
      bool                 m_bDebugOutputOnce;
      TTimer*              m_ptIdleTimer;
      unsigned int         m_nBlockSize;
      TThreadingType       m_ttThreadingType;
      int                  m_nThreadPriority;
      vvfVST               m_vvfInternalBuffers;
      std::vector<vvfVST>             m_vvvfRecursionBuffers;
      std::vector<std::vector <int> > m_vvnRecursionBufferUsage;
      std::vector<HANDLE>  m_vhProcHandles;
      std::vector<std::vector<TVSTHostPluginInstance> > m_vvpPlugins;
      void                 Exit();
      void                 AssertSoundRunning();
      long                 BufsizeCurrent();
      double               SampleRate();
      void                 AssertIndices(const unsigned int& nLayer, const unsigned int& nChannel);
      void __fastcall      OnIdleTimer(TObject *Sender);
      void                 ProcessST(vvfVST& vvfBuffers);
      void                 ProcessMT(vvfVST& vvfBuffers);
      bool                 HasPlugins();

};
//------------------------------------------------------------------------------
#endif
