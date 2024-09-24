//------------------------------------------------------------------------------
/// \file SoundDllPro_MarkButtons.h
/// \author Berg
/// \brief Implementation of classes ButtonProperties and SDPMarkButtons for
/// synchronized button marking
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
#ifndef SoundDllPro_MarkButtonsH
#define SoundDllPro_MarkButtonsH
//------------------------------------------------------------------------------
#include <vcl.h>
#include <windows.h>
#include <vector>
//------------------------------------------------------------------------------

//------------------------------------------------------------------------------
/// structure containing information on buttons
//------------------------------------------------------------------------------
struct FindStruct {
   RECT        rc;
   HWND        handle;
   AnsiString  sWindowCaption;
   int         iWindowCount;
   bool        bIsButtonHandle;
};
//------------------------------------------------------------------------------


//------------------------------------------------------------------------------
/// \class Helper class for storing properties of a button.
//------------------------------------------------------------------------------
class ButtonProperties
{
   public:
      ButtonProperties(HWND hWindowHandle, RECT &rc, int64_t nStart, int64_t nStop, bool bHandleIsButtonHandle);
      HWND       m_hWindowHandle;
      RECT       m_rc;
      int64_t    m_nMarkStart;
      int64_t    m_nMarkStop;
      bool       m_bIsMarked;
      bool       m_bDone;
      bool       m_bHandleIsButtonHandle;
 };
//------------------------------------------------------------------------------

//------------------------------------------------------------------------------
/// \class SDPMarkButtons. Helper class to highlight buttons synchronized with
/// a 'position' of a device
//------------------------------------------------------------------------------
class SDPMarkButtons
{
   public:
      SDPMarkButtons();
      ~SDPMarkButtons();
      void AddButton(HWND hWindowHandle, RECT &rc, int64_t nStart, int64_t nLength, bool bHandleIsButtonHandle = true);
      void AddButton(RECT &rc, AnsiString strCaption, int64_t nStart, int64_t nLength);
      void DoButtonMarking(int64_t nPosition);
      void ResetButtons();
      static FindStruct       sm_fs;      ///< static FindStruct member used when searching window with particular caption
   private:
      void MarkButton(unsigned int nIndex, bool b);
      void RemoveDoneButtons(void);
      std::vector<ButtonProperties> m_vbp;
      CRITICAL_SECTION        m_cs;
};
//------------------------------------------------------------------------------
#endif
