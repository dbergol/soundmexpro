//------------------------------------------------------------------------------
/// \file AHtVSTVisualize.h
/// \author Berg
/// \brief Implementation of class CHtVSTVisualize.
///
/// Project SoundMexPro
/// Module  HtVSTVisualize.dll
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
#ifndef __AHTVSTVISUALIZE_H
#define __AHTVSTVISUALIZE_H

#include <vcl.h>
#include "audioeffectx.h"




//--------------------------------------------------------------------------
class TfrmVisual;
//--------------------------------------------------------------------------
///
//--------------------------------------------------------------------------
class CHtVSTVisualize : public AudioEffectX
{
   public:
      CHtVSTVisualize (audioMasterCallback audioMaster);
      ~CHtVSTVisualize ();
      bool m_bIsValid;

      // Processes
      virtual long startProcess ();
      virtual long stopProcess ();
      virtual void resume ();
      virtual void suspend ();
      virtual void process (float **inputs, float **outputs, long sampleFrames);
      virtual void processReplacing (float **inputs, float **outputs, long sampleFrames);
      virtual void DoProcess (float **inputs, float **outputs, long sampleFrames, bool bReplace);

      // Program
      virtual void setProgramName (char *name);
      virtual void getProgramName (char *name);

      // Parameters
      virtual void setParameter (long index, float value);
      virtual float getParameter (long index);
      virtual void getParameterLabel (long index, char *label);
      virtual void getParameterDisplay (long index, char *text);
      virtual void getParameterName (long index, char *text);

      virtual bool getEffectName (char* name);
      virtual bool getVendorString (char* text);
      virtual bool getProductString (char* text);
      virtual long getVendorVersion () { return 1000; }

      virtual VstPlugCategory getPlugCategory () { return kPlugCategAnalysis; }

   protected:
      float       m_fVisible;
      TfrmVisual* m_pfrmVisual;
};
#endif
