//------------------------------------------------------------------------------
/// \file VSTHostPlugin.h
/// \author Berg
/// \brief Implementation of classes TVSTPluginEditor and TVSTHostPlugin.
/// Encapsulates one VST-plugin and a parameter editor for a VST-plugin
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
#ifndef VSTHostPluginH
#define VSTHostPluginH

#include <vcl.h>
#include <Classes.hpp>
#include <ComCtrls.hpp>
#include <Controls.hpp>
#include <ExtCtrls.hpp>
#include <Forms.hpp>
#include <Menus.hpp>
#include "VSTParameterFrame.h"
#include <vector>
#include <valarray>
//---------------------------------------------------------------------------
#include "VSTParameterFrame.h"
#include "VSTPluginProperties.h"
// switch off clang warnings from VST-SDK

#pragma clang diagnostic push
#pragma clang diagnostic ignored "-Wreserved-id-macro"
#pragma clang diagnostic ignored "-Wundef"
#pragma clang diagnostic ignored "-Wmismatched-tags"
#pragma clang diagnostic ignored "-Wextra-semi"
#pragma clang diagnostic ignored "-Wdocumentation-unknown-command"
#include "aeffectx.h"
#pragma clang diagnostic pop // restore options

#include "VSTVendorDefs.h"

#define TRYDELETENULL(p) {if (p!=NULL) { try {delete p;} catch (...){;} p = NULL;}}



typedef std::vector<std::valarray<float> > vvfVST;

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
/// definitions needed to use VST 2.4 syntax and types with VST 2.3
/// (defined in aeffect.h of VST-SDK 2.4). Not used up to now.
//------------------------------------------------------------------------------
#ifndef VST_2_4_EXTENSIONS
   // switch off clang warnings from VST-SDK
   #pragma clang diagnostic push
   #pragma clang diagnostic ignored "-Wreserved-id-macro"
   #pragma clang diagnostic ignored "-Wundef"
   #pragma clang diagnostic ignored "-Wmismatched-tags"
   #pragma clang diagnostic ignored "-Wextra-semi"
   #pragma clang diagnostic ignored "-Wunused-parameter"
   #pragma clang diagnostic ignored "-Wshadow-field-in-constructor"
   #include "AEffEditor.hpp"
   #pragma clang diagnostic pop
   typedef long VstInt32;        /// 32 bit integer type.
   typedef VstInt32 VstIntPtr;   /// platform-dependent integer type, same size as pointer
   typedef short VstInt16;       /// 16 bit integer type
#endif
//------------------------------------------------------------------------------

//------------------------------------------------------------------------------
/// define needed for VST-plugin handling...
//------------------------------------------------------------------------------
typedef AEffect* (*PluginEntryProc) (audioMasterCallback audioMaster);
//------------------------------------------------------------------------------


class TVSTHostPlugin;

//------------------------------------------------------------------------------
/// \class TVSTPluginEditor. VST-plugin-parameter editor
//------------------------------------------------------------------------------
class TVSTPluginEditor : public TForm
{
   __published:
      TMainMenu *mnuMain;
      TMenuItem *miSettings;
      TMenuItem *miParDlg;
      TMenuItem *miPluginDlg;
      TPageControl *pcPages;
      TTabSheet *tsParameter;
      TTabSheet *tsPlugin;
      TScrollBox *scbParameter;
      TStatusBar *sb;
      TMenuItem *miPrograms;
      TMenuItem *N1;
      TMenuItem *miUpdatePars;
      void __fastcall FormClose(TObject *Sender, TCloseAction &Action);
      void __fastcall FormDestroy(TObject *Sender);
      void __fastcall miDlgClick(TObject *Sender);
      void __fastcall miProgramClick(TObject *Sender);
      void __fastcall miUpdateParsClick(TObject *Sender);
      void __fastcall FormShow(TObject *Sender);
   private:
      TVSTHostPlugin*   m_pParentPlugin;
      bool              m_bHasEditor;
      int               m_nParameterDlgWidth;
      int               m_nParameterDlgHeight;
      void              SetEditor();
   public:
      __fastcall TVSTPluginEditor(TVSTHostPlugin* pParentPlugin);
      void              UpdateParameters();
      void              UpdatePrograms();
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
/// \class TVSTHostPlugin. Class for handling one VST-plugin
//------------------------------------------------------------------------------
class TVSTHostPlugin
{
   // allow full access to editor
   friend class TVSTPluginEditor;
   friend class TVSTThread;
   public:
      AnsiString              m_strEffectName;
//      AnsiString              m_strInternalName;
      AnsiString              m_strVendorString;
      AnsiString              m_strProductString;
      AnsiString              m_strLibName;

      TVSTHostPlugin(AnsiString                 strLibName,
                     audioMasterCallback        HostCallback,
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
      VstPlugCategory         GetCategory();
      bool                    SetUserConfig(AnsiString& as);
   private:
      AEffect*                m_pEffect;
      TVSTThread*             m_pProcThread;
      HANDLE                  m_hProcEvents[PLUG_EVENT_LAST];
      char*                   m_lpcszUserConfig;
      bool                    m_bCanReplacing;
      bool                    m_bBypass;
      VstPlugCategory         m_kPlugCategory;
      TStringList*            m_pslPrograms;
      TStringList*            m_pslParams;
      HMODULE                 m_hLib;
      TVSTPluginEditor*       m_pfrmEditor;
      TVSTPluginProperties*   m_pfrmProperties;
      int                     m_nBlockSize;
      AnsiString              m_asProcError;
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
