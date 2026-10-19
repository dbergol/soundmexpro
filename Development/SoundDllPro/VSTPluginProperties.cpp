//------------------------------------------------------------------------------
/// \file VSTPluginProperties.cpp
/// \author Berg
/// \brief Implementation of class TVSTPluginProperties holding properties of
/// a VST-plugin retrieved through IVSTPluginImpl (format-independent)
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
#include "VSTPluginImpl.h"
//---------------------------------------------------------------------------
#pragma package(smart_init)
#pragma resource "*.dfm"

//------------------------------------------------------------------------------

//------------------------------------------------------------------------------
/// constructor
//------------------------------------------------------------------------------
__fastcall TVSTPluginProperties::TVSTPluginProperties(IVSTPluginImpl* pImpl)
   : TForm((TComponent*)NULL), m_pImpl(pImpl)
{
}
//------------------------------------------------------------------------------

//------------------------------------------------------------------------------
/// OnShow callback of form: fills properties into TListView
//------------------------------------------------------------------------------
#pragma argsused
void __fastcall TVSTPluginProperties::FormShow(TObject *Sender)
{
   if (!m_pImpl)
      return;

   lv->Clear();

   std::vector<TVSTInfoProperty> vProps;
   m_pImpl->GetInfoProperties(vProps);

   TListItem* pli;
   for (unsigned int i = 0; i < vProps.size(); i++)
      {
      pli = lv->Items->Add();
      pli->Caption = vProps[i].strName;
      pli->SubItems->Add(vProps[i].strValue);
      }
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
