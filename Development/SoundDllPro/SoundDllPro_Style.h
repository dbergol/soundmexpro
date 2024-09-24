//------------------------------------------------------------------------------
/// \file SoundDllPro_Style.h
/// \author Berg
/// \brief Implementation of helper functions for VCL styles
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
#ifndef SoundDllPro_StyleH
#define SoundDllPro_StyleH


/// colors
#define SMP_CHNL_BLUE_LIGHT      clSkyBlue
#define SMP_CHNL_BLUE_DARK       TColor(0x695F34)
#define SMP_CHNL_GREEN_LIGHT     clMoneyGreen
#define SMP_CHNL_GREEN_DARK      TColor(0x4C7828)
#define SMP_CHNL_ORANGE_LIGHT    TColor(0x00A6CAF0)
#define SMP_CHNL_ORANGE_DARK     TColor(0x5F3455)

#define SMP_TRCK_BLUE_LIGHT      clSkyBlue
#define SMP_TRCK_BLUE_DARK       TColor(0x6C291A)
#define SMP_TRCK_GREEN_LIGHT     clMoneyGreen
#define SMP_TRCK_GREEN_DARK      TColor(0x015501)
#define SMP_TRCK_ORANGE_LIGHT    TColor(0x00A6CAF0)
#define SMP_TRCK_ORANGE_DARK     TColor(0x5F3455)

//------------------------------------------------------------------------------
/// style functions
//------------------------------------------------------------------------------
void SetStyle();
bool IsDarkTheme();
#endif
