//------------------------------------------------------------------------------
/// \file VSTPluginProperties.h
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
#ifndef VSTPluginPropertiesH
#define VSTPluginPropertiesH
//------------------------------------------------------------------------------
#include <Classes.hpp>
#include <Controls.hpp>
#include <StdCtrls.hpp>
#include <Forms.hpp>
#include <ComCtrls.hpp>

class AEffect;

//------------------------------------------------------------------------------
/// Class holding VST-Plugin properties
//------------------------------------------------------------------------------
class TVSTPluginProperties : public TForm
{
   __published:
      TListView *lv;
   void __fastcall FormShow(TObject *Sender);
   void __fastcall FormKeyDown(TObject *Sender, WORD &Key,
          TShiftState Shift);
   private:
      AEffect* m_pEffect;
   public:
      __fastcall TVSTPluginProperties(AEffect* pEffect);
};
//------------------------------------------------------------------------------
#endif
