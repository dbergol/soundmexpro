//------------------------------------------------------------------------------
/// \file VSTHost.cpp
/// \author Berg
/// \brief Implementation of class VSTHost. Implements a VST-host handling
/// multiple instances of TVSTHostPlugin in mutliple channels. Intended to
/// be used in conjunction with CAsio class and descendants or compatible classes.
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

#include <stdio.h>
#include "VSTHost.h"
#include "SoundDllPro_Main.h"

#pragma warn -aus
//------------------------------------------------------------------------------


// initialize static member
SoundDllProMain* TVSTHost::sm_phtSound = NULL;


//------------------------------------------------------------------------------
/// Constructor. Initializes members
//------------------------------------------------------------------------------
TVSTHost::TVSTHost(UnicodeString usName, SoundDllProMain* phtSound, bool bTest)
   :  m_nLayers(VSTHOST_NUM_PLUGINS),
      m_bStarted(false),
      m_usName(usName),
      m_ptIdleTimer(NULL),
      m_ttThreadingType(TT_SINGLE),
      m_nThreadPriority(2) // corresponds to tpHighest
{
   sm_phtSound = phtSound;
   if (!bTest && !sm_phtSound)
      throw Exception("invalid sound instance passed to VSTHost");

   AssertSoundRunning();

   m_ptIdleTimer = new TTimer(NULL);
   m_ptIdleTimer->Interval = 50;
   m_ptIdleTimer->OnTimer = OnIdleTimer;
   m_bDebugOutputOnce = true;
}
//------------------------------------------------------------------------------

//------------------------------------------------------------------------------
/// Cleans up
//------------------------------------------------------------------------------
TVSTHost::~TVSTHost()
{
   if (m_ptIdleTimer)
      m_ptIdleTimer->Enabled = false;
   TRYDELETENULL(m_ptIdleTimer);
   try
      {
      Exit();
      }
   catch (...)
      {
      }
   sm_phtSound = NULL;
}
//------------------------------------------------------------------------------

//------------------------------------------------------------------------------
/// returns static sound instance member
/// \retval static sound instance member
//------------------------------------------------------------------------------
SoundDllProMain* TVSTHost::SoundInstance()
{
   return sm_phtSound;
}

//------------------------------------------------------------------------------


//------------------------------------------------------------------------------
/// checks if asio device is in state RUNNING
//------------------------------------------------------------------------------
void TVSTHost::AssertSoundRunning()
{
   if (!SoundInstance())
      return;
    if (SoundInstance()->DeviceIsRunning())
      throw Exception("access to VST-Host function denied: device is running");
}
//------------------------------------------------------------------------------

//------------------------------------------------------------------------------
/// returns current buffer size
/// \retval current buffer size
//------------------------------------------------------------------------------
long   TVSTHost::BufsizeCurrent()
{
   if (!SoundInstance())
      return 0;
   return (long)SoundInstance()->SoundBufsizeSamples();
}
//------------------------------------------------------------------------------

//------------------------------------------------------------------------------
/// returns current sample rete
/// \retval current sample rete
//------------------------------------------------------------------------------
double TVSTHost::SampleRate()
{
   if (!SoundInstance())
      return 0.0;
   return (double)SoundInstance()->SoundGetSampleRate();
}
//------------------------------------------------------------------------------

//------------------------------------------------------------------------------
/// checks if channel and layer are within allowed range
//------------------------------------------------------------------------------
void TVSTHost::AssertIndices(const unsigned int& nLayer, const unsigned int& nChannel)
{
   if (nLayer >= m_vvpPlugins.size())
      throw Exception("plugin layer out of range");
   if (nChannel  >= m_vvpPlugins[nLayer].size())
      throw Exception("plugin channel out of range");
}
//------------------------------------------------------------------------------


//------------------------------------------------------------------------------
/// Initializes plugin vector and internal buffer
//------------------------------------------------------------------------------
#pragma argsused
void TVSTHost::Init( unsigned int      nChannels,
                     unsigned int      nBlockSize,
                     TThreadingType    ttThreadingType,
                     int               nThreadPriority)
{
   AssertSoundRunning();
   Exit();

   m_ttThreadingType    = ttThreadingType;
   if (m_nThreadPriority < 0 || m_nThreadPriority > 3)
      throw Exception("invalid thread priority passd to VSTHost->Init");
   m_nThreadPriority    = nThreadPriority;

   unsigned int nLayer, nChannel;
   m_nBlockSize = nBlockSize;
   // Setup internal processing buffer needed for vertical plugins
   m_vvfInternalBuffers.resize(nChannels);

   m_vhProcHandles.resize(nChannels);
   for (nChannel = 0; nChannel < nChannels; nChannel++)
      m_vvfInternalBuffers[nChannel].resize(nBlockSize);

   // setup 'array' of plugins and 'recursion buffers'. The 'recursion buffers'
   // contain the data within one channel AFTER each layer to be used for recursions
   // (i.e. 'virtual' inputs for plugins) in the next buffer loop
   m_vvpPlugins.resize(m_nLayers);
   m_vvvfRecursionBuffers.resize(m_nLayers);
   m_vvnRecursionBufferUsage.resize(m_nLayers);

   for (nLayer = 0; nLayer < m_nLayers; nLayer++)
      {
      m_vvvfRecursionBuffers[nLayer].resize(nChannels);
      m_vvnRecursionBufferUsage[nLayer].resize(nChannels);
      for (nChannel = 0; nChannel < nChannels; nChannel++)
         {
         m_vvpPlugins[nLayer].push_back(TVSTHostPluginInstance());
         m_vvvfRecursionBuffers[nLayer][nChannel].resize(nBlockSize);
         m_vvvfRecursionBuffers[nLayer][nChannel] = 0.0f;
         m_vvnRecursionBufferUsage[nLayer][nChannel] = 0;
         }
      }

   m_nChannels = nChannels;
}
//------------------------------------------------------------------------------

//------------------------------------------------------------------------------
/// Unloads all plugins an clears internal verctors
//------------------------------------------------------------------------------
void TVSTHost::Exit()
{
   AssertSoundRunning();
   Stop();
   unsigned int nLayer, nChannel;
   for (nLayer = 0; nLayer < m_vvpPlugins.size(); nLayer++)
      {
      for (nChannel = 0; nChannel < m_vvpPlugins[nLayer].size(); nChannel++)
         {
         UnloadPlugin(nLayer, nChannel);
         }
      m_vvpPlugins[nLayer].clear();
      }
   m_vvpPlugins.clear();
   m_vvfInternalBuffers.clear();
   m_vvvfRecursionBuffers.clear();
   m_vvnRecursionBufferUsage.clear();
}
//------------------------------------------------------------------------------

//------------------------------------------------------------------------------
/// Returns pointer to plugin at channel/layer, throws Exception, if no plugin
/// is loaded to that channel/layer
//------------------------------------------------------------------------------
TVSTHostPlugin* TVSTHost::Plugin(const unsigned int& nLayer,
                                 const unsigned int& nChannel,
                                 bool  bNoError)
{
   AssertIndices(nLayer, nChannel);
   if (!bNoError && !m_vvpPlugins[nLayer][nChannel].m_pPlugin)
      throw Exception("no plugin loaded to channel " + IntToStr((int)nChannel) + ", layer " + IntToStr((int)nLayer));
   return m_vvpPlugins[nLayer][nChannel].m_pPlugin;
}
//------------------------------------------------------------------------------

//------------------------------------------------------------------------------
/// loads a plugin to channel/layer
//------------------------------------------------------------------------------
TVSTHostPlugin* TVSTHost::LoadPlugin(  UnicodeString usLibName,
                                       const std::vector<int>& viIn,
                                       const std::vector<int>& viOut,
                                       std::vector<TVSTNode>& vRecurse,
                                       const unsigned int& nLayer)
{
   // not allowed if device is running
   AssertSoundRunning();

   // check indices
   unsigned int i;
   unsigned int nRecursions = 0;
   std::vector<TVSTNode > vNodes;
   // - input
   // translate inputs and recursions to input vector with TVSTNodes for constructor
   // of plugin
   for (i = 0; i < viIn.size(); i++)
      {
      if (viIn[i] >= 0)
         {
         AssertIndices(nLayer, (unsigned int)viIn[i]);
         // additionally the corresponding input must not be used yet in any plugin of this layer
         if (!!m_vvpPlugins[nLayer][(unsigned int)viIn[i]].m_pPlugin)
            throw Exception("a plugin at position " + IntToStr((int)nLayer)  + " is already using input channel " + IntToStr((int)viIn[i]));
         // add a node with Layer -1 (i.e. real channel to be used)
         vNodes.push_back(TVSTNode(-1, viIn[i]));
         }
      else
         {
         if (vRecurse.size() <= nRecursions)
            throw Exception("the plugin at position " + IntToStr((int)nLayer)  + " specified more -1 values as input channels than recursion channels");
         // NOTE: if -1 is passed as recursion layer parameter, that plugin is instructed to use an
         // OWN output as input, i.e. use passed layer (Nodes are located BEHIND the plugin)
         if (vRecurse[nRecursions].m_nLayer == -1)
            vRecurse[nRecursions].m_nLayer = (int)nLayer;
         AssertIndices((unsigned int)vRecurse[nRecursions].m_nLayer, (unsigned int)vRecurse[nRecursions].m_nChannel);
         vNodes.push_back(TVSTNode(vRecurse[nRecursions].m_nLayer, vRecurse[nRecursions].m_nChannel));
         nRecursions++;
         }
      }


   if (vRecurse.size() != nRecursions)
      throw Exception("number of recursions and number of negative input channels must match (2)!");

   // - check outputs 
   for (i = 0; i < viOut.size(); i++)
      AssertIndices(nLayer, (unsigned int)viOut[(unsigned int)i]);

   // if we do multithreading with one thread per plugin, then a maximum of MAXIMUM_WAIT_OBJECTS
   // real plugins is allowed within one layer (for WaitForMultipleObjects)
   if (m_ttThreadingType == TT_PLUGIN)
      {
      unsigned int nChannel;
      int nPlugsInLayer = 0;
      for (nChannel = 0; nChannel < m_nChannels; nChannel++)
         {
         // only call 'real' plugins, no references!
         if (IsPlugin(nLayer, nChannel))
            nPlugsInLayer++;
         }
      if (nPlugsInLayer > MAXIMUM_WAIT_OBJECTS)
         throw Exception("cannot load more than 64 plugins to one layer in multithreading mode");
      }

   TThreadPriority tp = (TThreadPriority)((int)tpNormal + m_nThreadPriority);
   // create new plugin
   TVSTHostPlugin *pPlugin = new TVSTHostPlugin(usLibName,
                                                VSTHostCallback,
                                                (float)SampleRate(),
                                                (int)m_nBlockSize,
                                                vNodes,
                                                viOut,
                                                m_ttThreadingType == TT_PLUGIN,
                                                tp
                                                );

   try
      {
      // check category: synth not supported!!
      if (pPlugin->GetCategory() == kPlugCategSynth)
         throw Exception("synth plugins not supported");


      unsigned int nMappedChannels     = (unsigned int)viIn.size();
      int nReference = -1;
      // copy instance pointer to all channels
      for (i = 0; i < nMappedChannels; i++)
         {
         // do it only for 'regular use' channels, NOT for recurse channels!
         if (viIn[i] >= 0)
            {
            m_vvpPlugins[nLayer][(unsigned int)viIn[i]].m_pPlugin = pPlugin;
            if (nReference == -1)
               nReference = (int)i;
            else
            // and write the 'reference' to all but first plugin to 'mark'
            // them as 'reference only, no real plugin'
               m_vvpPlugins[nLayer][(unsigned int)viIn[i]].m_nReference = viIn[(unsigned int)nReference];
            }
         }
      // finally increment usage counter for the VSTNodes, that are used
      // for recursion by this plugin
      for (i = 0; i < vNodes.size(); i++)
         {
         if (vNodes[i].m_nLayer >= 0)
            m_vvnRecursionBufferUsage[(unsigned int)vNodes[i].m_nLayer][(unsigned int)vNodes[i].m_nChannel]++;
         }
      }
   catch (...)
      {
      TRYDELETENULL(pPlugin);
      throw;
      }
   return pPlugin;
}
//------------------------------------------------------------------------------

//------------------------------------------------------------------------------
/// unloads a plugin from channel/layer
//------------------------------------------------------------------------------
void TVSTHost::UnloadPlugin(const unsigned int& nLayer, const unsigned int& nChannel)
{
   AssertSoundRunning();
   AssertIndices(nLayer, nChannel);
   TVSTHostPlugin *pPlugin = m_vvpPlugins[nLayer][nChannel].m_pPlugin;
   if (!!pPlugin)
      {
      unsigned int n;
      // go through all plugins of this layer
      for (n = 0; n < m_vvpPlugins[nLayer].size(); n++)
         {
         if (m_vvpPlugins[nLayer][n].m_pPlugin == pPlugin)
            {
            m_vvpPlugins[nLayer][n].m_pPlugin     = NULL;
            m_vvpPlugins[nLayer][n].m_nReference  = -1;
            }
         }

      // decrement usage counters for VSTNodes used for recursion in this plugin
      const std::vector<TVSTNode >& vNodes = pPlugin->GetInputMapping();
      for (unsigned i = 0; i < vNodes.size(); i++)
         {
         if (vNodes[i].m_nLayer >= 0)
            m_vvnRecursionBufferUsage[(unsigned int)vNodes[i].m_nLayer][(unsigned int)vNodes[i].m_nChannel]--;
         }

      TRYDELETENULL(pPlugin);
      }
}
//------------------------------------------------------------------------------

//------------------------------------------------------------------------------
/// returns true if at least one plugin is loaded
//------------------------------------------------------------------------------
bool TVSTHost::HasPlugins()
{
   unsigned int nLayer, nChannel;
   for (nLayer = 0; nLayer < m_vvpPlugins.size(); nLayer++)
      {
      for (nChannel = 0; nChannel < m_vvpPlugins[nLayer].size(); nChannel++)
         {
         try
            {
            // only call 'real' plugins, no references
            if (IsPlugin(nLayer, nChannel))
               return true;
            }
         catch (...)
            {
            }
         }
      }
   return false;
}
//------------------------------------------------------------------------------

//------------------------------------------------------------------------------
/// returns true if plugin is a 'real' plugin (not a reference)
//------------------------------------------------------------------------------
bool TVSTHost::IsPlugin(const unsigned int& nLayer, const unsigned int& nChannel)
{
   return (!!m_vvpPlugins[nLayer][nChannel].m_pPlugin && m_vvpPlugins[nLayer][nChannel].m_nReference == -1);
}
//------------------------------------------------------------------------------



//------------------------------------------------------------------------------
/// calls Stop function of all plugins
//------------------------------------------------------------------------------
void TVSTHost::Start()
{
   if (m_bStarted)
      return;
   m_bDebugOutputOnce = true;
   if (BufsizeCurrent() != (int)m_nBlockSize)
      throw Exception("error starting VST-Host: blocksize error");
   unsigned int nLayer, nChannel;
   for (nLayer = 0; nLayer < m_vvpPlugins.size(); nLayer++)
      {
      for (nChannel = 0; nChannel < m_vvpPlugins[nLayer].size(); nChannel++)
         {
         // change on 24.03.09: try/catch removed: let start fail completely, if satart of one plugin fails
         // only call 'real' plugins, no references
         if (IsPlugin(nLayer, nChannel))
               m_vvpPlugins[nLayer][nChannel].m_pPlugin->Start((float)SampleRate(), (int)m_nBlockSize);
         }
      }
   m_bStarted = true;
}
//------------------------------------------------------------------------------

//------------------------------------------------------------------------------
/// calls Stop function of all plugins
//------------------------------------------------------------------------------
void TVSTHost::Stop()
{
   if (!m_bStarted)
      return;
   unsigned int nLayer, nChannel;
   for (nLayer = 0; nLayer < m_vvpPlugins.size(); nLayer++)
      {
      for (nChannel = 0; nChannel < m_vvpPlugins[nLayer].size(); nChannel++)
         {
         try
            {
            // only call 'real' plugins, no references
            if (IsPlugin(nLayer, nChannel))
               m_vvpPlugins[nLayer][nChannel].m_pPlugin->Stop();
            }
         catch (...)
            {
            }
         }
      }
   m_bStarted = false;
}
//------------------------------------------------------------------------------

//------------------------------------------------------------------------------
/// calls processing routine for multi-/singlethreading
//------------------------------------------------------------------------------
void TVSTHost::Process(vvfVST& vvfBuffers)
{
   if (!HasPlugins())
      return;
   if (m_ttThreadingType == TT_PLUGIN)
      ProcessMT(vvfBuffers);
   else
      ProcessST(vvfBuffers);
}
//------------------------------------------------------------------------------

//------------------------------------------------------------------------------
/// calls Process function of all plugins
//------------------------------------------------------------------------------
void TVSTHost::ProcessST(vvfVST& vvfBuffers)
{
   try
      {
      if (!m_bStarted)
         throw Exception("VSTHost " + m_usName + " not started: call to Process not allowed!");
      unsigned int nChannels = (unsigned int)vvfBuffers.size();
      if (m_vvfInternalBuffers.size()  != nChannels)
         throw Exception("fatal sizing error 1 in VSTHost");
      unsigned int nMappedChannelsOut, nPluginChannelsOut;
      unsigned int nChannel, nLayer, nMappedChannel;
      UnicodeString usError;

      // processing scheme
      // -  plugins are loaded with a so called 'mapping' for input and output channels
      // -  plugins that are within one layer (horizontal) are all processed with
      //    the identical input data, output data are accumulated.
      // -  Before that accumulation (for each horizonztal layer) all accumulation
      //    buffers that are used for any plugin (i.e. if an input is used at all)
      //    are cleared. This has the following consequences:
      //    -  output channels, that are used for any plugin as input will contain
      //       processed data only (may be a sum of multiple processed buffers, if
      //       multiple plugins use the same output channel)
      //    -  output channels that are _not_ used for any plugin as input will contain
      //       the original contained data + processed data (again may be a sum of
      //       multiple processing plugins)
      //    -  output channels that are _not_ used as input nor as ouput will contain
      //       their original data!

      // This scheme is implemented using the external (passed) buffer vvfBuffers and
      // the internal buffer m_vvfInternalBuffers as follows
      // -  internal buffer is used for 'accumulation' of data within one horizontal
      //    layer, therefore
      //    -  before every layer loop data are copied form external to internal buffer
      //    -  afterwards channels that are used on any input within this layer are
      //       cleared
      // -  external (passed) buffer is used for calling the plugins: within one layer
      //    all plugins need the identical, not accumulated input!
      // -  after each layer loop the accumulated data are copied back to external buffer

      // In the beginning, copy external data to internal data for first loop
      // NOTE: this is only done once (not within layer loop), because at
      // the end of the layer loop, data are copied form internal back to
      // external, so _after_ first loop, they are already equal!!
      for (nChannel = 0; nChannel < nChannels; nChannel++)
         {
         if (m_vvfInternalBuffers[nChannel].size() != vvfBuffers[nChannel].size())
            throw Exception("fatal sizing error 1.5 in VSTHost");
         m_vvfInternalBuffers[nChannel] = vvfBuffers[nChannel];
         }

      // got through layers ('vertical' plugins)
      for (nLayer = 0; nLayer < m_vvpPlugins.size(); nLayer++)
         {
         if (m_vvpPlugins[nLayer].size()  != nChannels)
            throw Exception("fatal sizing error 2 in VSTHost");

         // for new layer clear channels, that are used as input (to be done
         // before processing loop below)
         for (nChannel = 0; nChannel < nChannels; nChannel++)
            {
            if (!!m_vvpPlugins[nLayer][nChannel].m_pPlugin)
               m_vvfInternalBuffers[nChannel] = 0.0f;
            }

         // got through channels ('horizontal' plugins)
         for (nChannel = 0; nChannel < nChannels; nChannel++)
            {
            // only call 'real' plugins, no references!
            if (IsPlugin(nLayer, nChannel))
               {
               // pass complete (!) external buffer and recursion buffer to plugin. Plugin will
               // 'pick' correct channels according to it's input channel mapping
               m_vvpPlugins[nLayer][nChannel].m_pPlugin->Process(vvfBuffers, m_vvvfRecursionBuffers);
               // retrieve error from plugin
               usError = m_vvpPlugins[nLayer][nChannel].m_pPlugin->GetProcError();
               if (!usError.IsEmpty())
                  throw Exception(usError);

               const vvfVST& vvfOut = m_vvpPlugins[nLayer][nChannel].m_pPlugin->GetOutData();

               // retrieve reference to output mapping
               const std::vector<int>&  viMappingOut  = m_vvpPlugins[nLayer][nChannel].m_pPlugin->GetOutputMapping();
               nMappedChannelsOut   = (unsigned int)viMappingOut.size();

               // accumulate data in internal buffer with respect to output mapping
               // stored in plugin itself!
               nPluginChannelsOut = (unsigned int)vvfOut.size();
               for (nMappedChannel = 0; nMappedChannel < nMappedChannelsOut; nMappedChannel++)
                  {
                  if (nMappedChannel > nPluginChannelsOut)
                     break;
                  if (viMappingOut[nMappedChannel] < (int)nChannels)
                     m_vvfInternalBuffers[(unsigned int)viMappingOut[nMappedChannel]] += vvfOut[nMappedChannel];
                  }
               }
            }
         // after processing one layer, copy all (!) channels back to external buffer
         // and to recursion buffers (only if it is used at all)
         for (nChannel = 0; nChannel < nChannels; nChannel++)
            {
            vvfBuffers[nChannel] = m_vvfInternalBuffers[nChannel];
            if (m_vvnRecursionBufferUsage[nLayer][nChannel])
               {
               m_vvvfRecursionBuffers[nLayer][nChannel] = m_vvfInternalBuffers[nChannel];
               #ifdef DEBUG_RECURSE
               if (m_bDebugOutputOnce)
                  {
                  AnsiString as;
                  as.printf("STORE recurse copy from: %d:%d", nLayer, nChannel);
                  OutputDebugString(as.c_str());
                  }
               #endif
               }
            }
         }
      }
   catch (Exception &e)
      {
      throw Exception("exception in VSTHost processing callback: " + e.Message);
      }
   catch (...)
      {
      throw Exception("unknown exception in VSTHost processing callback");
      }
   m_bDebugOutputOnce = false;
}
//------------------------------------------------------------------------------

//------------------------------------------------------------------------------
/// calls Process function of all plugins
//------------------------------------------------------------------------------
void TVSTHost::ProcessMT(vvfVST& vvfBuffers)
{
   try
      {
      if (!m_bStarted)
         throw Exception("VSTHost not started: call to Process not allowed!");
      unsigned int nChannels = (unsigned int)vvfBuffers.size();
      if (m_vvfInternalBuffers.size()  != nChannels)
         throw Exception("fatal sizing error 1 in VSTHost ("
                              + m_usName + ": "
                              + IntToStr((int)m_vvfInternalBuffers.size()) + "/"
                              + IntToStr((int)nChannels) + ")");
      unsigned int nMappedChannelsOut, nPluginChannelsOut;
      unsigned int nChannel, nLayer, nMappedChannel, nHandleCounter;
      UnicodeString usError;

      // processing scheme
      // -  plugins are loaded with a so called 'mapping' for input and output channels
      // -  plugins that are within one layer (horizontal) are all processed with
      //    the identical input data, output data are accumulated.
      // -  Before that accumulation (for each horizonztal layer) all accumulation
      //    buffers that are used for any plugin (i.e. if an input is used at all)
      //    are cleared. This has the follwing consequences:
      //    -  output channels, that are used for any plugin as input will contain
      //       processed data only (may be a sum of multiple processed buffers, if
      //       multiple plugins use the same output channel)
      //    -  output channels that are _not_ used for any plugin as input will contain
      //       the original contained data + processed data (again may be a sum of
      //       multiple processing plugins)
      //    -  output channels that are _not_ used as input nor as ouput will contain
      //       their original data!

      // This scheme is implemented using the external (passed) buffer vvfBuffers and
      // the internal buffer m_vvfInternalBuffers as follows
      // -  internal buffer is used for 'accumulation' of data within one horizontal
      //    layer, therefore
      //    -  before every layer loop data are copied form external to internal buffer
      //    -  afterwards channels that are used on any input within this layer are
      //       cleared
      // -  external (passed) buffer is used for calling the plugins: within one layer
      //    all plugins need the identical, not accumulated input!
      // -  after each layer loop the accumulated data are copied back to external buffer

      // In the beginning, copy external data to internal data for first loop
      // NOTE: this is only done once (not within layer loop), because at
      // the end of the layer loop, data are copied form internal back to
      // external, so _after_ first loop, they are already equal!!
      for (nChannel = 0; nChannel < nChannels; nChannel++)
         {
         if (m_vvfInternalBuffers[nChannel].size() != vvfBuffers[nChannel].size())
            throw Exception("fatal sizing error 1.5 in VSTHost");
         m_vvfInternalBuffers[nChannel] = vvfBuffers[nChannel];
         }

      // got through layers ('vertical' plugins)
      for (nLayer = 0; nLayer < m_vvpPlugins.size(); nLayer++)
         {
         if (m_vvpPlugins[nLayer].size()  != nChannels)
            throw Exception("fatal sizing error 2 in VSTHost");


         // for new layer clear channels, that are used as input (to be done
         // before processing loop below)
         for (nChannel = 0; nChannel < nChannels; nChannel++)
            {
            if (!!m_vvpPlugins[nLayer][nChannel].m_pPlugin)
               m_vvfInternalBuffers[nChannel] = 0.0f;
            }


         nHandleCounter = 0;

         // got through channels ('horizontal' plugins) and call process.
         // Process returns immediately and we have to wait after loop until all horizontal
         // plugins are done!
         for (nChannel = 0; nChannel < nChannels; nChannel++)
            {
            // only call 'real' plugins, no references!
            if (IsPlugin(nLayer, nChannel))
               {
               // retrieve 'done' signal handle of plugin for waitng below
               m_vhProcHandles[nHandleCounter++] = m_vvpPlugins[nLayer][nChannel].m_pPlugin->GetProcHandle();

               // pass complete (!) external buffer and recursion buffer to plugin. Plugin will
               // 'pick' correct channels according to it's input channel mapping
               m_vvpPlugins[nLayer][nChannel].m_pPlugin->Process(vvfBuffers, m_vvvfRecursionBuffers);
               // retrieve error from plugin
               usError = m_vvpPlugins[nLayer][nChannel].m_pPlugin->GetProcError();
               if (!usError.IsEmpty())
                  throw Exception(usError);

               if (nHandleCounter > MAXIMUM_WAIT_OBJECTS)
                  throw Exception("more than 64 plugins detected within one layer in multithreading mode");
               }
            }
         // wait for all (!) plugins to have their processing done
         if (WaitForMultipleObjects(nHandleCounter, &m_vhProcHandles[0], true, 10000) == WAIT_TIMEOUT)
            throw Exception("unexpected timeout waiting for plugin processing");

         // got through channels ('horizontal' plugins) and retrieve processed data
         for (nChannel = 0; nChannel < nChannels; nChannel++)
            {
            // only call 'real' plugins, no references!
            if (IsPlugin(nLayer, nChannel))
               {
               const vvfVST& vvfOut = m_vvpPlugins[nLayer][nChannel].m_pPlugin->GetOutData();
               // retrieve reference to output mapping
               const std::vector<int>&  viMappingOut  = m_vvpPlugins[nLayer][nChannel].m_pPlugin->GetOutputMapping();
               nMappedChannelsOut   = (unsigned int)viMappingOut.size();

               // accumulate data in internal buffer with respect to output mapping
               // stored in plugin itself!
               nPluginChannelsOut = (unsigned int)vvfOut.size();
               for (nMappedChannel = 0; nMappedChannel < nMappedChannelsOut; nMappedChannel++)
                  {
                  if (nMappedChannel > nPluginChannelsOut)
                     break;
                  if (viMappingOut[nMappedChannel] < (int)nChannels)
                     m_vvfInternalBuffers[(unsigned int)viMappingOut[nMappedChannel]] += vvfOut[nMappedChannel];
                  }
               }
            }

         // after processing one layer, copy all (!) channels back to external buffer
         // and to recursion buffers (only if it is used at all)
         for (nChannel = 0; nChannel < nChannels; nChannel++)
            {
            vvfBuffers[nChannel] = m_vvfInternalBuffers[nChannel];
            if (m_vvnRecursionBufferUsage[nLayer][nChannel])
               {
               m_vvvfRecursionBuffers[nLayer][nChannel] = m_vvfInternalBuffers[nChannel];
               #ifdef DEBUG_RECURSE
               if (m_bDebugOutputOnce)
                  {
                  AnsiString as;
                  as.printf("STORE recurse copy from: %d:%d", nLayer, nChannel);
                  OutputDebugString(as.c_str());
                  }
               #endif
               }
            }
         }
      }
   catch (Exception &e)
      {
      throw Exception("exception in VSTHost processing callback: " + e.Message);
      }
   catch (...)
      {
      throw Exception("unknown exception in VSTHost processing callback");
      }

   m_bDebugOutputOnce = false;
}
//------------------------------------------------------------------------------

//------------------------------------------------------------------------------
/// Idle timer. Calls idle for all effect editors
//------------------------------------------------------------------------------
#pragma argsused
void __fastcall TVSTHost::OnIdleTimer(TObject *Sender)
{
   m_ptIdleTimer->Enabled = false;

   unsigned int nLayer, nChannel;
   for (nLayer = 0; nLayer < m_vvpPlugins.size(); nLayer++)
      {
      for (nChannel = 0; nChannel < m_vvpPlugins[nLayer].size(); nChannel++)
         {
         try
            {
            // only call 'real' plugins, no references
            if (IsPlugin(nLayer, nChannel))
               m_vvpPlugins[nLayer][nChannel].m_pPlugin->CallEditIdle();
            }
         catch (...)
            {
            }
         }
      }
   m_ptIdleTimer->Enabled = true;
}
//------------------------------------------------------------------------------

//------------------------------------------------------------------------------
/// VST-Host callback (static member function)
//------------------------------------------------------------------------------
#pragma argsused
VstIntPtr VSTCALLBACK TVSTHost::VSTHostCallback( AEffect* effect,
                                       VstInt32 opcode,
                                       VstInt32 index,
                                       VstIntPtr value,
                                       void* ptr,
                                       float opt)
{
   VstIntPtr result = 0;
   switch (opcode)
      {
      case audioMasterVersion :
            #ifdef VST_2_4_EXTENSIONS
            result = kVstVersion;
            #else
            result = 2300;
            #endif
            break;
      case  audioMasterCanDo:
            result = 0;
            break;

      // return and set current block size in plugin
      case audioMasterGetBlockSize:
            if (!!SoundInstance())
               {
               result = (VstIntPtr)SoundInstance()->SoundBufsizeSamples();
               // send sample rate and block size again to plugin
               effect->dispatcher(effect, effSetBlockSize, 0, result, 0, 0);
               }
            break;
      case audioMasterGetSampleRate:
            if (!!SoundInstance())
               {
               result = (int)SoundInstance()->SoundGetSampleRate();
               // send sample rate and block size again to plugin
               effect->dispatcher(effect, effSetSampleRate, 0, 0, 0, result);
               }
            break;
      }

   return result;
}
//------------------------------------------------------------------------------


