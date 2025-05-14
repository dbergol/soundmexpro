//------------------------------------------------------------------------------
/// \file AHtVSTEq.cpp
/// \author Berg
/// \brief Implementation of class CHtVSTEq.
///
/// Project SoundMexPro
/// Module  HtVSTEq.dll
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
#include "AHtVSTEq.h"

#include <stdio.h>
#include <windows.h>
#include <math.h>
#include "eqinput.h"

#pragma warn -aus

//--------------------------------------------------------------------------
#ifndef TRYDELETENULL
   #define TRYDELETENULL(p) {if (p!=NULL) { try {delete p;} catch (...){;} p = NULL;}}
#endif


//#define DEBUG_FILTER2
#ifdef DEBUG_FILTER
TStringList *pslDebug;
#endif

struct TFilterValue
{
   float freq;
   CHtComplex valL;
   CHtComplex valR;
} ;



/// local prototypes
bool        IsDouble(AnsiString s);
AnsiString  DoubleToStr(double val, const char* lpcszFormat = NULL);
double      StrToDouble(AnsiString s);
CHtComplex  StrToComplex(AnsiString s);



//--------------------------------------------------------------------------
/// constructor: calls base class and initializes members
//--------------------------------------------------------------------------
CHtVSTEq::CHtVSTEq (audioMasterCallback audioMaster)
   :  AudioEffectX (audioMaster, 1, 2), // programs, parameters
      m_bIsValid(false),
      m_nUpdateInterval(30),
      m_bComplex(false),
      m_imInterpolMode(IM_LIN_LIN),
      m_fEnabled(1.0f),
      m_fVisible(0.0f),
      m_bMuted(false),
      m_pfrmVisual(NULL),
      m_pmsFileFilter(NULL),
      m_pOLA(NULL),
      m_pIni(NULL)
{
   try
      {
      #ifdef DEBUG_FILTER
      pslDebug = new TStringList();
      #endif

      // factor for painting spectrum reasonable with respect to fullscale
      m_fVisSpecFactor = dBToFactor(15.0);

      InitializeCriticalSection(&csDataSection);

      m_nNumChannels = 2;

      setNumInputs ((VstInt32)m_nNumChannels);
      setNumOutputs ((VstInt32)m_nNumChannels);
      setUniqueID (CCONST('H','t','E','q'));
      canProcessReplacing ();    // supports both accumulating and replacing output

      // set default values
      m_nFFTLen      = 512;

      m_pmsFileFilter = new TMemoryStream();

      m_pfrmVisual = new TfrmEqInput(this);

      InitFilter();

      ReadFilterNameFromIni();

      // set 'valid flag' (used in _main)
      m_bIsValid = true;
      }
   catch (...)
      {
      DeleteCriticalSection(&csDataSection);
      TRYDELETENULL(m_pmsFileFilter);
      TRYDELETENULL(m_pfrmVisual);
      }
}
//--------------------------------------------------------------------------

//--------------------------------------------------------------------------
/// destructor
//--------------------------------------------------------------------------
CHtVSTEq::~CHtVSTEq ()
{
   ExitOLA();
   TRYDELETENULL(m_pmsFileFilter);
   TRYDELETENULL(m_pfrmVisual);
   TRYDELETENULL(m_pIni);

   DeleteCriticalSection(&csDataSection);
   #ifdef DEBUG_FILTER
   TRYDELETENULL(pslDebug);
   #endif
}
//--------------------------------------------------------------------------

//--------------------------------------------------------------------------
/// sets program name
//--------------------------------------------------------------------------
void CHtVSTEq::setProgramName (char *name)
{
   AnsiString str = name;
   if (FileExists(str))
      LoadFilterFile(str);
}
//--------------------------------------------------------------------------

//--------------------------------------------------------------------------
/// returns actual program name
//--------------------------------------------------------------------------
void CHtVSTEq::getProgramName (char *name)
{
   if (m_strFilterFile.IsEmpty())
      strcpy (name, "(empty)");
   strcpy (name, m_strFilterFile.c_str());
}
//--------------------------------------------------------------------------


//--------------------------------------------------------------------------
/// sets a parameter value
//--------------------------------------------------------------------------
#pragma argsused
void CHtVSTEq::setParameter (VstInt32 index, float value)
{
   switch (index)
      {
      case 0:  m_fEnabled = value; m_pfrmVisual->btnSkip->Down = (m_fEnabled <= 0.0f); break;
      case 1:  m_fVisible = value; m_pfrmVisual->Visible = (m_fVisible > 0.5f); break;
      }
}
//--------------------------------------------------------------------------

//--------------------------------------------------------------------------
/// returns a parameter value
//--------------------------------------------------------------------------
#pragma argsused
float CHtVSTEq::getParameter (VstInt32 index)
{
   switch (index)
      {
      case 0:  return m_fEnabled;
      case 1:  return m_fVisible;
      }
   return 0.0f;
}
//--------------------------------------------------------------------------


//--------------------------------------------------------------------------
/// returns a parameter name
//--------------------------------------------------------------------------
#pragma argsused
void CHtVSTEq::getParameterName (VstInt32 index, char *label)
{
   switch (index)
      {
      case 0:  strcpy (label, "enabled");  break;
      case 1:  strcpy (label, "visible");  break;
      }
}
//--------------------------------------------------------------------------

//--------------------------------------------------------------------------
/// returns a parameter display value
//--------------------------------------------------------------------------
#pragma argsused
void CHtVSTEq::getParameterDisplay (VstInt32 index, char *text)
{
   switch (index)
      {
      case 0:  sprintf(text, m_fEnabled > 0.5f ? "true" : "false"); break;
      case 1:  sprintf(text, m_fVisible > 0.5f ? "true" : "false"); break;
      }
}
//--------------------------------------------------------------------------

//--------------------------------------------------------------------------
/// returns a parameter  label (e.g. unit)
//--------------------------------------------------------------------------
#pragma argsused
void CHtVSTEq::getParameterLabel(VstInt32 index, char *label)
{
   strcpy (label, " ");       
}
//--------------------------------------------------------------------------

//--------------------------------------------------------------------------
/// returns effect name
//--------------------------------------------------------------------------
bool CHtVSTEq::getEffectName (char* name)
{
   strcpy (name, "Equalizer");
   return true;
}
//--------------------------------------------------------------------------

//--------------------------------------------------------------------------
/// returns effect product string
//--------------------------------------------------------------------------
bool CHtVSTEq::getProductString (char* text)
{
   strcpy (text, "HtVSTEqualizer");
   return true;
}
//--------------------------------------------------------------------------

//--------------------------------------------------------------------------
/// returns effect vendor string
//--------------------------------------------------------------------------
bool CHtVSTEq::getVendorString (char* text)
{
   strcpy (text, "Uni Ol");
   return true;
}
//--------------------------------------------------------------------------

//--------------------------------------------------------------------------
/// inits ola filter
//--------------------------------------------------------------------------
VstInt32 CHtVSTEq::startProcess ()
{
   InitOLA();
   return 0;
}
//--------------------------------------------------------------------------

//--------------------------------------------------------------------------
/// exits ola buffer
//--------------------------------------------------------------------------
VstInt32 CHtVSTEq::stopProcess ()
{
   ExitOLA();
   return 0;
}
//--------------------------------------------------------------------------

//--------------------------------------------------------------------------
/// calls startProcess
//--------------------------------------------------------------------------
void CHtVSTEq::resume ()
{
   startProcess();
}
//--------------------------------------------------------------------------

//--------------------------------------------------------------------------
/// calls stopProcess
//--------------------------------------------------------------------------
void CHtVSTEq::suspend ()
{
   stopProcess();
}
//--------------------------------------------------------------------------


//--------------------------------------------------------------------------
/// Processing routine called by process and processReplacing
//--------------------------------------------------------------------------
#pragma argsused
void CHtVSTEq::DoProcess (float **inputs, float **outputs, VstInt32 sampleFrames, bool bReplace)
{
   unsigned int nChannel;
   if ((m_fEnabled <= 0.5f) || !m_pOLA)
      {
      for (nChannel = 0; nChannel < m_nNumChannels; nChannel++)
         MoveMemory(outputs[nChannel], inputs[nChannel], (size_t)sampleFrames*sizeof(float));
      return;
      }
   else if (m_bMuted)
      {
      for (nChannel = 0; nChannel < m_nNumChannels; nChannel++)
         ZeroMemory(&outputs[nChannel][0], (size_t)sampleFrames*sizeof(float));
      return;
      }

   try
      {
      for (nChannel = 0; nChannel < m_nNumChannels; nChannel++)
         {
         m_vvafBuffer[nChannel] = 0.0f;
         // copy data
         CopyMemory(&m_vvafBuffer[nChannel][0], &inputs[nChannel][0], (size_t)sampleFrames*sizeof(float));
         }


      // call filter
      m_pOLA->DoOLA(m_vvafBuffer);

      for (nChannel = 0; nChannel < m_nNumChannels; nChannel++)
         {
         ZeroMemory(&outputs[nChannel][0], (size_t)sampleFrames*sizeof(float));
         // write data back
         CopyMemory(&outputs[nChannel][0], &m_vvafBuffer[nChannel][0], (size_t)sampleFrames*sizeof(float));
         }
      }
   catch (...)
      {
      OutputDebugString("error");
      }
}
//--------------------------------------------------------------------------

//--------------------------------------------------------------------------
/// accumulating processing routine
//--------------------------------------------------------------------------
void CHtVSTEq::process (float **inputs, float **outputs, VstInt32 sampleFrames)
{
   try
      {
      DoProcess(inputs, outputs, sampleFrames, false);
      }
   catch (...)
      {
      OutputDebugString(__FUNC__);
      throw;
      }
}
//--------------------------------------------------------------------------

//--------------------------------------------------------------------------
/// replacing processing routine
//--------------------------------------------------------------------------
void CHtVSTEq::processReplacing (float **inputs, float **outputs, VstInt32 sampleFrames)
{
   try
      {
      DoProcess(inputs, outputs, sampleFrames, true);
      }
   catch (...)
      {
      OutputDebugString(__FUNC__);
      throw;
      }
}
//--------------------------------------------------------------------------

//--------------------------------------------------------------------------
/// initialize data
//--------------------------------------------------------------------------
void CHtVSTEq::InitFilter()
{
   // alloc buffer for filter values and initialize as real filter with ones
   m_vvacFilter.resize(m_nNumChannels);
   m_vvacSpecPreFilter.resize(m_nNumChannels);
   m_vvacSpecPostFilter.resize(m_nNumChannels);
   m_vvacSpecPreFilterDraw.resize(m_nNumChannels);
   m_vvacSpecPostFilterDraw.resize(m_nNumChannels);
   unsigned int nChannel;
   for (nChannel = 0; nChannel < m_nNumChannels; nChannel++)
      {
      m_vvacFilter[nChannel].resize(m_nFFTLen/2);
      m_vvacFilter[nChannel] = 1.0f;
      m_vvacSpecPreFilter[nChannel].resize(m_nFFTLen/2+1);
      m_vvacSpecPostFilter[nChannel].resize(m_nFFTLen/2+1);
      m_vvacSpecPreFilterDraw[nChannel].resize(m_nFFTLen/2+1);
      m_vvacSpecPostFilterDraw[nChannel].resize(m_nFFTLen/2+1);
      }

   m_pfrmVisual->Initialize(m_nFFTLen, sampleRate);
}
//--------------------------------------------------------------------------

//--------------------------------------------------------------------------
// overloaded base class function: re-initalizes equalizer form on samplerate change
//--------------------------------------------------------------------------
void CHtVSTEq::setSampleRate (float sampleRate)
{
   AudioEffectX::setSampleRate (sampleRate);
   m_pfrmVisual->Initialize(m_nFFTLen, sampleRate);
}
//--------------------------------------------------------------------------

//--------------------------------------------------------------------------
/// alloc memory and OLA
//--------------------------------------------------------------------------
void CHtVSTEq::InitOLA()
{
   ExitOLA();

   m_vvafBuffer.resize(m_nNumChannels);
   unsigned int nChannel;
   for (nChannel = 0; nChannel < m_nNumChannels; nChannel++)
      m_vvafBuffer[nChannel].resize((unsigned int)blockSize);

   // initialize Ola-Filter
   unsigned int nBufSize  = (unsigned int)blockSize;
   if (nBufSize < m_nFFTLen)
      nBufSize = m_nFFTLen;

   m_pOLA = new CHtOLA(m_nNumChannels, m_nFFTLen, nBufSize, SpecProcessCallback);
}
//--------------------------------------------------------------------------

//--------------------------------------------------------------------------
/// free memory
//--------------------------------------------------------------------------
void CHtVSTEq::ExitOLA()
{
   TRYDELETENULL(m_pOLA);
}
//--------------------------------------------------------------------------

//--------------------------------------------------------------------------
/// spectrum processing callback
//--------------------------------------------------------------------------
void CHtVSTEq::SpecProcessCallback(vvac & vvacSpectrum)
{
   if (!vvacSpectrum.size())
      return;
   static int iNumCallbacks = 0;
   EnterCriticalSection(&csDataSection);
   try
      {
      // store pre-processing spectrum
      if (m_pfrmVisual->Visible && !(iNumCallbacks % m_nUpdateInterval))
         m_vvacSpecPreFilter = vvacSpectrum;

      // calculate new spectrum
      unsigned int nChannel;
      for (nChannel = 0; nChannel < m_nNumChannels; nChannel++)
         {
         vvacSpectrum[nChannel] *= m_vvacFilter[nChannel];
         }

      if (m_pfrmVisual->Visible && !(iNumCallbacks % m_nUpdateInterval))
         {
         // store post-processing spectrum
         m_vvacSpecPostFilter = vvacSpectrum;
         // send a message to draww data
         m_pfrmVisual->UpdateTimer->Tag = 1;
         }
      }
   __finally
      {
      LeaveCriticalSection(&csDataSection);
      iNumCallbacks++;
      }
}
//------------------------------------------------------------------------------

//------------------------------------------------------------------------------
/// Updates GUI called by GUI when it receives corresponding message
//------------------------------------------------------------------------------
void CHtVSTEq::Update()
{
   EnterCriticalSection(&csDataSection);
   try
      {
      m_vvacSpecPreFilterDraw = m_vvacSpecPreFilter;
      m_vvacSpecPostFilterDraw = m_vvacSpecPostFilter;
      }
   catch (...)
      {
      OutputDebugString(__FUNC__);
      LeaveCriticalSection(&csDataSection);
      return;
      }
   LeaveCriticalSection(&csDataSection);


   int nBins = (int)m_vvacSpecPreFilterDraw[0].size();

   m_pfrmVisual->PreSpecSeriesL->BeginUpdate();
   m_pfrmVisual->PreSpecSeriesR->BeginUpdate();
   m_pfrmVisual->PostSpecSeriesL->BeginUpdate();
   m_pfrmVisual->PostSpecSeriesR->BeginUpdate();
   for (int i = 1; i < nBins; i++)
      {
      m_pfrmVisual->PreSpecSeriesL->YValues->Value[i] = (double)(m_fVisSpecFactor*abs(m_vvacSpecPreFilterDraw[0][(unsigned int)i]));
      if (m_nNumChannels > 1)
         m_pfrmVisual->PreSpecSeriesR->YValues->Value[i] = (double)(m_fVisSpecFactor*abs(m_vvacSpecPreFilterDraw[1][(unsigned int)i]));

      m_pfrmVisual->PostSpecSeriesL->YValues->Value[i] = (double)(m_fVisSpecFactor*abs(m_vvacSpecPostFilterDraw[0][(unsigned int)i]));
      if (m_nNumChannels > 1)
         m_pfrmVisual->PostSpecSeriesR->YValues->Value[i] = (double)(m_fVisSpecFactor*abs(m_vvacSpecPostFilterDraw[1][(unsigned int)i]));
      }
   m_pfrmVisual->PreSpecSeriesL->EndUpdate();
   m_pfrmVisual->PreSpecSeriesR->EndUpdate();
   m_pfrmVisual->PostSpecSeriesL->EndUpdate();
   m_pfrmVisual->PostSpecSeriesR->EndUpdate();

   if (m_pfrmVisual->PreSpecSeriesL->Active)
      m_pfrmVisual->PreSpecSeriesL->Repaint();
   if (m_pfrmVisual->PreSpecSeriesR->Active)
      m_pfrmVisual->PreSpecSeriesR->Repaint();
   if (m_pfrmVisual->PostSpecSeriesL->Active)
      m_pfrmVisual->PostSpecSeriesL->Repaint();
   if (m_pfrmVisual->PostSpecSeriesR->Active)
      m_pfrmVisual->PostSpecSeriesR->Repaint();

}
//------------------------------------------------------------------------------

//------------------------------------------------------------------------------
// LoadFilterFile only checks validity of values and converts values to a binary
// values for later interpolation but does no 'resampling' from frequencies to bins
//------------------------------------------------------------------------------
bool __fastcall CHtVSTEq::LoadFilterFile(AnsiString sFileName)
{
   bool bReturn = true;
   TRYDELETENULL(m_pIni);

   try
      {
      if(!FileExists(sFileName))
         throw Exception("Filter file '" + sFileName + "' not found");
      m_pIni = new TMemIniFile(sFileName);

      m_bLog = m_pIni->ReadBool("Settings", "Log", false);
      if (m_pIni->ReadBool("Settings", "LogFreqAxis", false))
         {
         m_pfrmVisual->cbLogFreq->Checked = true;
         m_pfrmVisual->cbLogFreqClick(NULL);
         }

      m_bWriteDebug = m_pIni->ReadBool("Settings", "WriteDebug", false);
      m_pfrmVisual->DebugSeries->Active = m_bWriteDebug;


      m_imInterpolMode = IM_LIN_LIN;
      UnicodeString us = LowerCase(m_pIni->ReadString("Settings", "InterpolMode", "lin_lin"));
      if (us == "log_lin")
         m_imInterpolMode = IM_LOG_LIN;
      else if (us == "lin_db")
         m_imInterpolMode = IM_LIN_DB;
      else if (us == "log_db")
         m_imInterpolMode = IM_LOG_DB;
      else if (us != "lin_lin")
         throw Exception("invalid InterpolMode found in settings");

      // read FFT properties
      m_nFFTLen = (unsigned int)m_pIni->ReadInteger("Settings", "FFTLen", 512);
      m_nFFTLen = (unsigned int)ConvertToPowerOfTwo((int)m_nFFTLen);

      if (m_nFFTLen < 64)
         m_nFFTLen = 64;
      else if (m_nFFTLen > 32768)
         m_nFFTLen = 32768;

      m_strFilterFile = sFileName;
      return LoadFilter("Filter");
      }
   catch (Exception &e)
      {
      AnsiString str = "Error loading filter file '" + sFileName + "': " + e.Message;
      m_pmsFileFilter->Clear();
      MessageBox(0, str.c_str(), "Error A", MB_ICONERROR);
      bReturn = false;
      }
   return bReturn;

}
//------------------------------------------------------------------------------

//------------------------------------------------------------------------------
/// loads a filter by section name
//------------------------------------------------------------------------------
bool __fastcall CHtVSTEq::LoadFilter(AnsiString sSectionName)
{

   bool bReturn = true;
   m_bMuted = true;
   // temporary StringList for filter values from INI Filterfile
   TStringList *pslFilter   = NULL;
   TStringList *pslValues   = NULL;
   m_pmsFileFilter->Clear();
   m_bComplex = false;
   try
      {
      if (!m_pIni->SectionExists(sSectionName))
         throw Exception("Section '" + sSectionName + "' not found in filter file '" + m_strFilterFile + "'");

      pslFilter = new TStringList();
      pslValues = new TStringList();
      pslValues->Delimiter = ';';

      // Read the filter values to StringList
      m_pIni->ReadSectionValues(sSectionName, pslFilter);
      if (pslFilter->Count < 2)
         throw Exception("At least two values needed in section '" + sSectionName + "'");

      m_pmsFileFilter->Size = pslFilter->Count * (int64_t)sizeof(TFilterValue);
      TFilterValue *lpFilterValues = (TFilterValue*)m_pmsFileFilter->Memory;
      TFilterValue fv;


      float fLastFreq = -1;
      AnsiString strFreq;

      for (int i = 0; i < pslFilter->Count; i++)
         {
         try
            {
            strFreq = pslFilter->Names[i];
            if (!IsDouble(strFreq))
               throw Exception("non-float frequency detected");
            fv.freq = (float)StrToDouble(strFreq);
            if (fv.freq < 0)
               throw Exception("Negative frequencies not allowed!");
            if (fLastFreq >= fv.freq)
               throw Exception("frequencies must be sorted ascending (no duplicates)");
            fLastFreq = fv.freq;


            pslValues->DelimitedText = pslFilter->Values[strFreq];
            if (pslValues->Text.Pos("i") != 0)
               m_bComplex = true;

            // check fieldcount (';' is default separator) and convert
            // to Complex
            // we have one field at least, because we checked fielcount
            // separator with '=' above to be == 2 !!
            fv.valL = StrToComplex(pslValues->Strings[0]);
            if (pslValues->Count == 1)
               fv.valR = fv.valL;
            else if (pslValues->Count == 2)
               fv.valR = StrToComplex(pslValues->Strings[1]);
            else
               throw Exception("wrong number of complex numbers");

            if (m_bLog)
               {
               if (fv.valR.imag() != 0.0f || fv.valL.imag() != 0.0f)
                  throw Exception("logarithmic filters must not contain complex numbers");
               fv.valR.real(dBToFactor(fv.valR.real()));
               fv.valL.real(dBToFactor(fv.valL.real()));
               }

            lpFilterValues[i] = fv;
            }
         catch (Exception &e)
            {
            AnsiString sError;
            sError.sprintf("Filter File: format 'frequency=complex' or "
            "'frequency=complex;complex' expected (error: '%s' in line '%s', linenumber %d of section '%s')",
            e.Message.c_str(), pslFilter->Strings[i].c_str(), i, sSectionName.c_str());
            throw Exception(sError);
            }
         }


      #ifdef DEBUG_FILTER
      pslDebug->Clear();
      for (int i = 0; i < pslFilter->Count; i++)
         {
         AnsiString sTmp;
         sTmp.printf("%f, %f, %f, %f",
            lpFilterValues[i].valR.re,
            lpFilterValues[i].valR.im,
            lpFilterValues[i].valL.re,
            lpFilterValues[i].valL.im
            );
         pslDebug->Add(sTmp);
         }
      pslDebug->SaveToFile("c:\\" + sFileName);
      #endif

      InitFilter();

      ResampleFilter();


      }
   catch (Exception &e)
      {
      AnsiString str = "Error loading filter file '" + m_strFilterFile + "': " + e.Message;
      m_pmsFileFilter->Clear();
      MessageBox(0, str.c_str(), "Error A", MB_ICONERROR);
      bReturn = false;
      }
   TRYDELETENULL(pslFilter);
   TRYDELETENULL(pslValues);
   m_bMuted = false;
   return bReturn;

}
//------------------------------------------------------------------------------

//------------------------------------------------------------------------------
/// SAves filter to INI file
//------------------------------------------------------------------------------
bool __fastcall CHtVSTEq::SaveFilter(AnsiString sFileName)
{
   if (m_bComplex)
      return false;
   DeleteFile(sFileName);
   TRYDELETENULL(m_pIni);
   m_pIni = new TMemIniFile(ExpandFileName(sFileName));
   try
      {
      m_pIni->WriteBool("Settings", "Log", false);
      // write FFT properties
      m_pIni->WriteInteger("Settings", "FFTLen", (int)m_nFFTLen);

      // go through filter
      DWORD dwBins = (DWORD)m_vvacFilter[0].size();
      float f4BinSize = sampleRate / (float)m_nFFTLen;
      float fFreq = 0.0f;
      AnsiString sValue;
      unsigned int nChannel;
      EnterCriticalSection(&csDataSection);
      try
         {
         for (DWORD i = 0; i < dwBins; i++)
            {
            sValue = DoubleToStr((double)m_vvacFilter[0][i].real());
            for (nChannel = 1; nChannel < m_nNumChannels; nChannel++)
               sValue += ";" + DoubleToStr((double)m_vvacFilter[nChannel][i].real());
            m_pIni->WriteString("Filter", DoubleToStr((double)fFreq), sValue);
            fFreq += f4BinSize;
            }
         }
      __finally
         {
         LeaveCriticalSection(&csDataSection);
         m_pIni->UpdateFile();
         }
      }
   catch (Exception &e)
      {
      TRYDELETENULL(m_pIni);
      AnsiString str = "Error saving filter file '" + sFileName + "': " + e.Message;
      MessageBox(0, str.c_str(), "Error B", MB_ICONERROR);
      return false;
      }
   TRYDELETENULL(m_pIni);
   return true;
}
//------------------------------------------------------------------------------

//------------------------------------------------------------------------------
/// Reads chart series values to filter
//------------------------------------------------------------------------------
bool __fastcall CHtVSTEq::ChartToFilter()
{
   if (m_bComplex)
      return false;

   unsigned int nNumValues = (unsigned int)m_vvacFilter[0].size();
   if (  nNumValues != (unsigned int)m_pfrmVisual->FilterSeriesL->YValues->Count
      || nNumValues != (unsigned int)m_pfrmVisual->FilterSeriesR->YValues->Count
      )
      {
      MessageBox(0,"internal error 2",  "Error C", 0);
      return false;
      }

   unsigned int nChannel;
   vvac vvacTmp(m_nNumChannels);
   for (nChannel = 0; nChannel < m_nNumChannels; nChannel++)
      vvacTmp[nChannel].resize(nNumValues);

   for (unsigned int i = 0; i < nNumValues; i++)
      {
      vvacTmp[0][i] = (float)m_pfrmVisual->FilterSeriesL->YValues->Value[(int)i];
      if (m_nNumChannels > 1)
         vvacTmp[1][i] = (float)m_pfrmVisual->FilterSeriesR->YValues->Value[(int)i];
      }

   EnterCriticalSection(&csDataSection);
   try
      {
      m_vvacFilter = vvacTmp;
      }
   __finally
      {
      LeaveCriticalSection(&csDataSection);
      }

   return true;
}
//------------------------------------------------------------------------------

//------------------------------------------------------------------------------
/// writes filter to chart series
//------------------------------------------------------------------------------
void __fastcall CHtVSTEq::FilterToChart()
{
   #define CREATEML
   #ifdef CREATEML
   AnsiString asF = "x = [";
   AnsiString asL = "y = [";
   AnsiString as;
   m_pfrmVisual->DebugSeries->Clear();
   #endif

   int nNumValues = (int)m_vvacFilter[0].size();

   for (int i = 0; i < nNumValues; i++)
      {
      m_pfrmVisual->FilterSeriesL->YValues->Value[i] = (double)abs(m_vvacFilter[0][(unsigned int)i]);
      if (m_nNumChannels > 1)
         m_pfrmVisual->FilterSeriesR->YValues->Value[i] = (double)abs(m_vvacFilter[1][(unsigned int)i]);

      #ifdef CREATEML
      if (m_bWriteDebug)
         {
         m_pfrmVisual->DebugSeries->AddXY(m_pfrmVisual->FilterSeriesL->XValues->Value[i], m_pfrmVisual->FilterSeriesL->YValues->Value[i]);
         as.printf("%lf", m_pfrmVisual->FilterSeriesL->XValues->Value[i]);
         asF += as + " ";
         as.printf("%lf", m_pfrmVisual->FilterSeriesL->YValues->Value[i]);
         asL += as + " ";
         }
      #endif

      }

   #ifdef CREATEML
   if (m_bWriteDebug)
      {
      asF += "];";
      asL += "];";
      TStringList* psl = new TStringList();
      psl->Add(asF);
      psl->Add(asL);
      psl->SaveToFile("T:\\myexamples\\flt_pl.m");
      delete psl;
      }
   #endif


   m_pfrmVisual->FilterSeriesL->Repaint();
   m_pfrmVisual->FilterSeriesR->Repaint();

   m_pfrmVisual->SetFilterProperties();

}
//------------------------------------------------------------------------------

//------------------------------------------------------------------------------
/// resamples a filter
//------------------------------------------------------------------------------
void __fastcall CHtVSTEq::ResampleFilter()
{

   float f4BinSize = sampleRate / (float)m_nFFTLen;
   DWORD dwSamples = m_nFFTLen/2;

   vvac vvacTmp(m_nNumChannels);
   unsigned int nChannel;
   for (nChannel = 0; nChannel < m_nNumChannels; nChannel++)
      vvacTmp[nChannel].resize(dwSamples);

   // get number of filter values specified in file
   DWORD dwNumberOfFileValues = (DWORD)((size_t)m_pmsFileFilter->Size / sizeof(TFilterValue));
   if (dwNumberOfFileValues <  2)
      throw Exception("Filter contains less than 2 values!");

   TFilterValue *lpFilterValues = (TFilterValue*)m_pmsFileFilter->Memory;

   float f4Frequency;
   TFilterValue fvLower, fvUpper;
   float f4FreqQuotient;

   for (UINT i = 0; i < dwSamples; i++)
      {
      f4Frequency = f4BinSize * (float)i;
      // first value is 0 Hz in bin-filter. We set all bins below first appearing
      // frequency in file filter to that first frequencies value
      if (f4Frequency <= lpFilterValues[0].freq)
         {
         vvacTmp[0][i] = lpFilterValues[0].valL;
         if (m_nNumChannels > 1)
            vvacTmp[1][i] = lpFilterValues[0].valR;
         }
      // same on upper border
      else if (f4Frequency >= lpFilterValues[dwNumberOfFileValues-1].freq)
         {
         vvacTmp[0][i] = lpFilterValues[dwNumberOfFileValues-1].valL;
         if (m_nNumChannels > 1)
            vvacTmp[1][i] = lpFilterValues[dwNumberOfFileValues-1].valR;
         }
      else // not below lower or above upper frequency
         {
         // not 'high performance': we search for upper and lower freq for all bins...
         for (UINT j = 0; j < dwNumberOfFileValues; j++)
            {
            // found it exactly? Write value and break;
            #pragma clang diagnostic push
            #pragma clang diagnostic ignored "-Wfloat-equal"
            if (lpFilterValues[j].freq == f4Frequency)
               {
               vvacTmp[0][i] = lpFilterValues[j].valL;
               if (m_nNumChannels > 1)
                  vvacTmp[1][i] = lpFilterValues[j].valR;
               break;
               }
            #pragma clang diagnostic pop            
            // below desired freq? Remember and continue
            else if (lpFilterValues[j].freq < f4Frequency)
               fvLower = lpFilterValues[j];
            // reaching this point we are above desired frequency and ready to interpolate!
            else
               {
               fvUpper = lpFilterValues[j];
               TFilterValue fv;
               fv.freq = f4Frequency;

               // interpolate frequency:
               // - on the log scale ....
               if (m_imInterpolMode == IM_LOG_LIN || m_imInterpolMode == IM_LOG_DB)
                  {
                  // (f-f1)/(f2-f1) on te log scale!!
                  f4FreqQuotient = (log10(fv.freq)-log10(fvLower.freq))/(log10(fvUpper.freq)-log10(fvLower.freq));
                  }
               // - on the lin scale ....
               else
                  {
                  // (f-f1)/(f2-f1)
                  f4FreqQuotient = (fv.freq-fvLower.freq)/(fvUpper.freq-fvLower.freq);
                  }

               // interpolate level:
               // - on the dB scale ...
               if (m_imInterpolMode == IM_LIN_DB || m_imInterpolMode == IM_LOG_DB)
                  {
                  fv.valL.real(dBToFactor(FactorTodB(fvLower.valL.real()) + (FactorTodB(fvUpper.valL.real())-FactorTodB(fvLower.valL.real())) * f4FreqQuotient));
                  fv.valL.imag(dBToFactor(FactorTodB(fvLower.valL.imag()) + (FactorTodB(fvUpper.valL.imag())-FactorTodB(fvLower.valL.imag())) * f4FreqQuotient));
                  vvacTmp[0][i] = fv.valL;

                  if (m_nNumChannels > 1)
                     {
                     fv.valR.real(dBToFactor(FactorTodB(fvLower.valR.real()) + (FactorTodB(fvUpper.valR.real())-FactorTodB(fvLower.valR.real())) * f4FreqQuotient));
                     fv.valR.imag(dBToFactor(FactorTodB(fvLower.valR.imag()) + (FactorTodB(fvUpper.valR.imag())-FactorTodB(fvLower.valR.imag())) * f4FreqQuotient));
                     vvacTmp[1][i] = fv.valR;
                     }
                  }
               // - ... on the linear factor scale
               else
                  {
                  fv.valL.real(fvLower.valL.real() + (fvUpper.valL.real()-fvLower.valL.real()) * f4FreqQuotient);
                  fv.valL.imag(fvLower.valL.imag() + (fvUpper.valL.imag()-fvLower.valL.imag()) * f4FreqQuotient);
                  vvacTmp[0][i] = fv.valL;

                  if (m_nNumChannels > 1)
                     {
                     fv.valR.real(fvLower.valR.real() + (fvUpper.valR.real()-fvLower.valR.real()) * f4FreqQuotient);
                     fv.valR.imag(fvLower.valR.imag() + (fvUpper.valR.imag()-fvLower.valR.imag()) * f4FreqQuotient);
                     vvacTmp[1][i] = fv.valR;
                     }
                  }


               break;
               }
            // this should never happen: reaching the 'last filefilter' value
            if (j == dwNumberOfFileValues-1)
               throw Exception("Complex filter interpolation error 1");
            }
         }
      }


   //copy this generated filter from the chart series to the real filter!
   EnterCriticalSection(&csDataSection);
   try
      {
      m_vvacFilter = vvacTmp;
      }
   __finally
      {
      LeaveCriticalSection(&csDataSection);
      }
   FilterToChart();

}
//------------------------------s------------------------------------------------
 
//------------------------------------------------------------------------------
/// reads filter name from ini file if exists
/// /* TODO: wh the deprectaed filebame of DLL??? */
//------------------------------------------------------------------------------
void CHtVSTEq::ReadFilterNameFromIni()
{
   char c[4096];
   ZeroMemory(c, 4096);
   GetModuleFileName(GetModuleHandle("HtVSTEqualizer2.dll"), c, 4096);
   AnsiString strIni = ChangeFileExt(c, ".ini");
   if (!FileExists(strIni))
      return;
   TIniFile *pIni = NULL;
   try
      {
      pIni = new TIniFile(strIni);
      AnsiString strFilter = pIni->ReadString("Settings", "Filter", "");
      if (!strFilter.IsEmpty() && FileExists(strFilter))
         {
            setProgramName(strFilter.c_str());
         }
      }
   catch (...)
      {
      }
   if (pIni)
      delete pIni;
}
//------------------------------------------------------------------------------

//------------------------------------------------------------------------------
//             TOOLS
//------------------------------------------------------------------------------


//------------------------------------------------------------------------------
/// comments see SoundDllPro_Tools.h
//------------------------------------------------------------------------------
bool IsDouble(AnsiString s)
{
   char *endptr = NULL;
   s = Trim(s);
   strtod(s.c_str(), &endptr);
   if (*endptr != NULL || s.Length()==0)
      return false;
   return true;
}
//------------------------------------------------------------------------------

//------------------------------------------------------------------------------
/// comments see SoundDllPro_Tools.h
//------------------------------------------------------------------------------
AnsiString DoubleToStr(double val, const char* lpcszFormat)
{
   AnsiString s;
   if (!lpcszFormat)
      lpcszFormat = "%lf";
   s.sprintf(lpcszFormat, val);
   return s;
}
//------------------------------------------------------------------------------

//------------------------------------------------------------------------------
/// comments see SoundDllPro_Tools.h
//------------------------------------------------------------------------------
double StrToDouble(AnsiString s)
{
   char *endptr = NULL;
   double value;
   s = Trim(s);
   value = strtod(s.c_str(), &endptr);
   if (*endptr != NULL || s.Length()==0)
      throw Exception("not a double");
   return value;
}
//------------------------------------------------------------------------------

//------------------------------------------------------------------------------
/// converts a string in form "x+6i" to a complex
//------------------------------------------------------------------------------
CHtComplex  StrToComplex(AnsiString s)
{
   CHtComplex c;
   s = Trim(s);
   //no 'i' in it? Must be real only!!
   if (s.Pos("i") == 0)
      {
      c = (float)StrToDouble(s);
      }
   else
      {
      char *endptr = NULL;
      //extract real part
      c.real((float)strtod(s.c_str(), &endptr));
      //if its NULL, imaginary is missing
      if (!endptr)
         throw Exception("not a complex");
      //extract imaginary part
      c.imag((float)strtod(endptr, &endptr));
      //at the end there must be 'i' ...
      if (*endptr != 'i')
         throw Exception("not a complex");
      //... and nothing else behind the 'i'!
      if (!!(*(endptr+1)))
         throw Exception("not a complex");
      }
   return c;
}
//------------------------------------------------------------------------------

//------------------------------------------------------------------------------
/// converts a dB value to linear fctor
//------------------------------------------------------------------------------
float dBToFactor(const float f4dB)
{
   return (float)pow(10.0,(double)f4dB/20.0);
}
//------------------------------------------------------------------------------

//------------------------------------------------------------------------------
/// converts a linear fctor to dB value 
//------------------------------------------------------------------------------
float FactorTodB(const float f4Factor)
{
   if (f4Factor <= 0)
      return -1000.0;
   return 20*log10(f4Factor);
}
//------------------------------------------------------------------------------


