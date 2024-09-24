//------------------------------------------------------------------------------
/// \file VSTPluginProperties.cpp
/// \author Berg
/// \brief Implementation of class TVSTPluginProperties holding properties of
/// a VST-plugin retrieved through VST interface
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
#include <vcl.h>
#pragma hdrstop

#include "VSTPluginProperties.h"
#include "VSTHostPlugin.h"
//---------------------------------------------------------------------------
#pragma package(smart_init)
#pragma resource "*.dfm"

#define VST_STR_PROP(p, name) \
   ZeroMemory(sz, 1024);\
   m_pEffect->dispatcher (m_pEffect, p, 0, 0, sz, 0);\
   pli = lv->Items->Add();\
   pli->Caption = name;\
   pli->SubItems->Add(sz);

#define VST_INT_PROP(p, name) \
   pli = lv->Items->Add();\
   pli->Caption = name;\
   pli->SubItems->Add((int)p);

//------------------------------------------------------------------------------

//------------------------------------------------------------------------------
/// constructor
//------------------------------------------------------------------------------
__fastcall TVSTPluginProperties::TVSTPluginProperties(AEffect* pEffect)
   : TForm((TComponent*)NULL), m_pEffect(pEffect)
{
}
//------------------------------------------------------------------------------

//------------------------------------------------------------------------------
/// OnShow callback of form: fills properties into TListView
//------------------------------------------------------------------------------
#pragma argsused
void __fastcall TVSTPluginProperties::FormShow(TObject *Sender)
{
   if (!m_pEffect)
      return;

   lv->Clear();

   TListItem* pli;
   char sz[1024];

   VST_STR_PROP(effGetEffectName,      "EffectName");
   VST_STR_PROP(effGetVendorString,    "VendorString");
   VST_STR_PROP(effGetProductString,   "ProductString");

   int nVersion = (int)m_pEffect->dispatcher(m_pEffect,effGetVstVersion,0,0,NULL,0.0f);
   if (nVersion < 1)
      nVersion = 1000;
   VST_INT_PROP(nVersion, "VST-Version");
   VST_INT_PROP(m_pEffect->version, "Version");


   VST_INT_PROP(m_pEffect->numPrograms, "numPrograms");
   VST_INT_PROP(m_pEffect->numParams, "numParams");
   VST_INT_PROP(m_pEffect->numInputs, "numInputs");
   VST_INT_PROP(m_pEffect->numOutputs, "numOutputs");
   VST_INT_PROP(m_pEffect->initialDelay, "initialDelay");
//   VST_INT_PROP(m_pEffect->realQualities, "realQualities");
//   VST_INT_PROP(m_pEffect->offQualities, "offQualities");
//   VST_FLOAT_PROP((double)m_pEffect->ioRatio, "ioRatio");
   VST_INT_PROP(m_pEffect->uniqueID, "uniqueID");

   VST_INT_PROP(!!(m_pEffect->flags & effFlagsHasEditor), "effFlagsHasEditor");
//   VST_INT_PROP(!!(m_pEffect->flags & effFlagsHasClip), "effFlagsHasClip");
//   VST_INT_PROP(!!(m_pEffect->flags & effFlagsHasVu), "effFlagsHasVu");
//   VST_INT_PROP(!!(m_pEffect->flags & effFlagsCanMono), "effFlagsCanMono");
   VST_INT_PROP(!!(m_pEffect->flags & effFlagsProgramChunks), "effFlagsProgramChunks");
   VST_INT_PROP(!!(m_pEffect->flags & effFlagsCanReplacing), "effFlagsCanReplacing");
   if (nVersion >= 2000)
      {
      VST_INT_PROP(!!(m_pEffect->flags & effFlagsIsSynth), "effFlagsIsSynth");
      VST_INT_PROP(!!(m_pEffect->flags & effFlagsNoSoundInStop), "effFlagsNoSoundInStop");
      VST_INT_PROP(!!m_pEffect->dispatcher(m_pEffect,effCanDo,0,0,(void*)"plugAsChannelInsert",0.0f), "plugAsChannelInsert");
      VST_INT_PROP(!!m_pEffect->dispatcher(m_pEffect,effCanDo,0,0,(void*)"plugAsSend",0.0f), "plugAsSend");
      VST_INT_PROP(!!m_pEffect->dispatcher(m_pEffect,effCanDo,0,0,(void*)"mixDryWet",0.0f), "mixDryWet");
      }

   VST_INT_PROP(!!(m_pEffect->dispatcher (m_pEffect, effCanDo, 0, 0, (void*)"bypass", 0)), "CanBypass");
}
//------------------------------------------------------------------------------

//------------------------------------------------------------------------------
/// FormKeyDown callback, calls FormShow again if F5 is pressed
//------------------------------------------------------------------------------
#pragma argsused
void __fastcall TVSTPluginProperties::FormKeyDown(TObject *Sender,
      WORD &Key, TShiftState Shift)
{
   if (Key == VK_F5)
      FormShow(NULL);
}
//------------------------------------------------------------------------------

