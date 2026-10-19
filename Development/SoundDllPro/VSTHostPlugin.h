//------------------------------------------------------------------------------
/// \file VSTHostPlugin.h
/// \author Berg
/// \brief Implementation of classes TVSTThread and TVSTHostPlugin.
/// Encapsulates one VST-plugin
///
/// Project SoundMexPro
/// Module  SoundDllPro.dll
///
/// ****************************************************************************
/// Copyright 2026 Daniel Berg, Oldenburg, Germany
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
#ifndef VSTHostPluginH
#define VSTHostPluginH

#include <vcl.h>
#include <Classes.hpp>
#include <vector>
#include <valarray>
//---------------------------------------------------------------------------
#include "VSTPluginEditor.h"
#include "VSTPluginProperties.h"
#include "VSTPluginImpl.h"
//------------------------------------------------------------------------------

typedef std::vector<std::valarray<float> > vvfVST;
class TVSTHostPlugin;
class TVSTPluginEditor;


//------------------------------------------------------------------------------
/// \class TVSTNode. Helper class describing a 'node' within the VST mapping matrix
/// (channel and layer)
//------------------------------------------------------------------------------
class TVSTNode
{
   public:
      TVSTNode()  : m_nChannel(-1), m_nLayer(-1){;}
      TVSTNode(int nLayer, int nChannel) : m_nChannel(nChannel), m_nLayer(nLayer){;}
      int               m_nChannel;
      int               m_nLayer;
};
//------------------------------------------------------------------------------

//------------------------------------------------------------------------------
// enum for plugin events for mulsti-thread synchronization
//------------------------------------------------------------------------------
enum PlugEvents {
   PLUG_EVENT_PROC = 0,
   PLUG_EVENT_STOP,
   PLUG_EVENT_DONE,
   PLUG_EVENT_LAST
};
//------------------------------------------------------------------------------

//------------------------------------------------------------------------------
/// \class thread class for plugin
//------------------------------------------------------------------------------
class TVSTThread : public TThread
{
   public:
	  TVSTThread(TVSTHostPlugin* pVSTPlugin, TThreadPriority tp);
	  void __fastcall Execute();
   private:
	  TVSTHostPlugin*   m_pVSTPlugin;
};
//------------------------------------------------------------------------------

//------------------------------------------------------------------------------
/// \class TVSTHostPlugin. Class for handling one VST-plugin. Public interface
/// unchanged; internally delegates all plugin-format-specific work to an
/// IVSTPluginImpl instance (m_pImpl).
//------------------------------------------------------------------------------
class TVSTHostPlugin
{
   // allow full access to editor
   friend class TVSTPluginEditor;
   friend class TVSTThread;
   public:
      AnsiString              m_strEffectName;
      AnsiString              m_strVendorString;
      AnsiString              m_strProductString;
      AnsiString              m_strLibName;

      TVSTHostPlugin(AnsiString                 strLibName,
                     float                      fSampleRate,
                     int                        nBlockSize,
                     const std::vector<TVSTNode>&  vMappingIn,
                     const std::vector<int>&       viMappingOut,
                     bool                       bUseThread = false,
                     TThreadPriority            tpThreadPriority = tpHighest
                     );
      ~TVSTHostPlugin();
      void                    ShowEditor();
      void                    Start(float fSampleRate, int nBlockSize);
      void                    Stop();
      void                    ShowProperties();
      bool                    CanBypass();
      bool                    GetBypass();
      void                    SetBypass(bool bBypass);
      AnsiString              GetProcError();
      int                     GetNumInputs();
      int                     GetNumOutputs();
      static void             GetNumChannels(AnsiString strLibName, int &nNumIn, int &nNumOut);
      AnsiString              GetPrograms();
      void                    SetProgram(AnsiString strName);
      void                    SetProgram(int nIndex);
      void                    SetProgramName(AnsiString strName);
      AnsiString              GetProgramName();
      int                     GetProgramIndex();
      AnsiString              GetParameterNames();
      AnsiString              GetParameters(AnsiString strParams);
      AnsiString              GetParameterValues(AnsiString strParams);
      void                    SetParameters(AnsiString strParamsAndValues);
      void                    SetParameters(AnsiString strParams, AnsiString strValues);
      void                    SetParameter(  AnsiString strName,
                                             float fValue,
                                             bool bIgnoreUnknownParameter = false,
                                             bool bUpdateGUI = true);
      void                    SetParameter(int nIndex, float fValue, bool bUpdateGUI = true);
      float                   GetParameter(AnsiString strName);
      float                   GetParameter(int nIndex);
      void                    Process(const vvfVST& vvfBuffer, const std::vector<vvfVST>& vvvfRecursion);
      HANDLE&                 GetProcHandle();
      const vvfVST&           GetOutData();
      const std::vector<int>&       GetOutputMapping();
      const std::vector<TVSTNode >& GetInputMapping();
      void                    CallEditIdle();
      EVSTPluginCategory      GetCategory();
      bool                    SetUserConfig(AnsiString& as);
   private:
      IVSTPluginImpl*         m_pImpl;
      TVSTThread*             m_pProcThread;
      HANDLE                  m_hProcEvents[PLUG_EVENT_LAST];
      bool                    m_bBypass;
      EVSTPluginCategory      m_kPlugCategory;
      TStringList*            m_pslPrograms;
      TStringList*            m_pslParams;
      TVSTPluginEditor*       m_pfrmEditor;
      TVSTPluginProperties*   m_pfrmProperties;
      int                     m_nBlockSize;
      std::vector<std::valarray<float> >  m_vvafIn;      /// internal input buffers
      std::vector<std::valarray<float> >  m_vvafOut;     /// internal output buffers
      std::valarray<float *>  m_vapfIn;      /// internal pointer lists to input buffers
      std::valarray<float *>  m_vapfOut;     /// internal pointer lists to output buffers
      bool                    m_bDebugOutputOnce;

      // channel 'mappings': plugin itself uses m_vMappingIn to copy correct channels
      // in ::Process, Host uses m_viMappingOut to copy correct output channels back
      std::vector<TVSTNode>   m_vMappingIn;
      std::vector<int>        m_viMappingOut;

      void                    DoProcess();
      void                    GetProperties();
      void                    Cleanup();
      void                    SetBufferSize(unsigned int nBufferSize);
};
//------------------------------------------------------------------------------
#endif
