//------------------------------------------------------------------------------
/// \file AHtVSTEqualizer.cpp
/// \author Berg
/// \brief Implementation of class CHtVSTEqualizer.
///
/// Project SoundMexPro
/// Module  HtVSTEqualizer.dll
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
#include "AHtVSTEqualizer.h"
#include <stdio.h>
#include <windows.h>
#include <math.h>
#include "eqinput.h"

//--------------------------------------------------------------------------
#define TRYDELETENULL(p) {if (p!=NULL) { try {delete p;} catch (...){;} p = NULL;}}


#define DEBUG_FILTER2
#ifdef DEBUG_FILTER
TStringList *pslDebug;
#endif

struct TFilterValue
{
   float freq;
   TfCplx valL;
   TfCplx valR;
} ;


/// local prototypes
bool        IsDouble(AnsiString s);
AnsiString  DoubleToStr(double val, const char* lpcszFormat = NULL);
double      StrToDouble(AnsiString s);
TfCplx      StrToComplex(AnsiString s);



//--------------------------------------------------------------------------
/// constructor: calls base class and initializes members
//--------------------------------------------------------------------------
CHtVSTEqualizer::CHtVSTEqualizer (audioMasterCallback audioMaster)
   :  AudioEffectX (audioMaster, 1, 2), // programs, parameters
      m_bIsValid(false),
      m_pFilter(NULL),
      m_pfBuffer(NULL),
      m_fEnabled(1.0f),
      m_fVisible(0.0f),
      m_pmsFilter(NULL),
      m_pmsFileFilter(NULL),
      m_bComplex(false),
      m_pfrmVisual(NULL),
      m_nUpdateInterval(50),
      m_bMuted(false)
{
   try
      {
      #ifdef DEBUG_FILTER
      pslDebug = new TStringList();
      #endif

      InitializeCriticalSection(&csDataSection);

      setNumInputs (2);
      setNumOutputs (2);
      setUniqueID (CCONST('H','t','E','q'));
      canProcessReplacing ();    // supports both accumulating and replacing output

      // set default values
      m_nFFTLen      = 512;
      m_nWindowLen   = 400;
      m_nFWindowFeed = wffHalf;

      m_pmsFileFilter = new TMemoryStream();
      m_pmsFilter = new TMemoryStream();

      m_pfrmVisual = new TfrmEqInput(this);

      InitFilter();

      ReadFilterNameFromIni();

      // set 'valid flag' (used in _main)
      m_bIsValid = true;


      }
   catch (...)
      {
      DeleteCriticalSection(&csDataSection);
      TRYDELETENULL(m_pmsFilter);
      TRYDELETENULL(m_pmsFileFilter);
      TRYDELETENULL(m_pfrmVisual);
      }
}
//--------------------------------------------------------------------------

//--------------------------------------------------------------------------
/// destructor

//--------------------------------------------------------------------------
CHtVSTEqualizer::~CHtVSTEqualizer ()
{
   ExitOLA();
   TRYDELETENULL(m_pmsFilter);
   TRYDELETENULL(m_pmsFileFilter);
   TRYDELETENULL(m_pfrmVisual);
   DeleteCriticalSection(&csDataSection);
   #ifdef DEBUG_FILTER
   TRYDELETENULL(pslDebug);
   #endif
}
//--------------------------------------------------------------------------


//--------------------------------------------------------------------------
/// sets program name
//--------------------------------------------------------------------------
void CHtVSTEqualizer::setProgramName (char *name)
{
   AnsiString str = name;
   if (FileExists(str))
      LoadFilter(str);
}
//--------------------------------------------------------------------------

//--------------------------------------------------------------------------
/// returns actual program name
//--------------------------------------------------------------------------
void CHtVSTEqualizer::getProgramName (char *name)
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
void CHtVSTEqualizer::setParameter (long index, float value)
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
float CHtVSTEqualizer::getParameter (long index)
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
void CHtVSTEqualizer::getParameterName (long index, char *label)
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
void CHtVSTEqualizer::getParameterDisplay (long index, char *text)
{
   switch (index)
      {
      case 0:  sprintf(text, m_fEnabled > 0.5 ? "true" : "false"); break;
      case 1:  sprintf(text, m_fVisible > 0.5 ? "true" : "false"); break;
      }
}
//--------------------------------------------------------------------------

//--------------------------------------------------------------------------
/// returns a parameter  label (e.g. unit)
//--------------------------------------------------------------------------
#pragma argsused
void CHtVSTEqualizer::getParameterLabel(long index, char *label)
{
   strcpy (label, " ");       
}
//--------------------------------------------------------------------------

//--------------------------------------------------------------------------
/// returns effect name
//--------------------------------------------------------------------------
bool CHtVSTEqualizer::getEffectName (char* name)
{
   strcpy (name, "Equalizer");
   return true;
}
//--------------------------------------------------------------------------

//--------------------------------------------------------------------------
/// returns effect product string
//--------------------------------------------------------------------------
bool CHtVSTEqualizer::getProductString (char* text)
{
   strcpy (text, "HtVSTEqualizer");
   return true;
}
//--------------------------------------------------------------------------

//--------------------------------------------------------------------------
/// returns effect vendor string
//--------------------------------------------------------------------------
bool CHtVSTEqualizer::getVendorString (char* text)
{
   strcpy (text, "HörTech");
   return true;
}
//--------------------------------------------------------------------------

//--------------------------------------------------------------------------
/// inits ola filter
//--------------------------------------------------------------------------
long CHtVSTEqualizer::startProcess ()
{
   InitOLA();
   return 0;
}
//--------------------------------------------------------------------------

//--------------------------------------------------------------------------
/// exits ola buffer
//--------------------------------------------------------------------------
long CHtVSTEqualizer::stopProcess ()
{
   ExitOLA();
   return 0;
}
//--------------------------------------------------------------------------

//--------------------------------------------------------------------------
///
//--------------------------------------------------------------------------
void CHtVSTEqualizer::resume ()
{
   startProcess();
}
//--------------------------------------------------------------------------

//--------------------------------------------------------------------------
///
//--------------------------------------------------------------------------
void CHtVSTEqualizer::suspend ()
{
   stopProcess();
}
//--------------------------------------------------------------------------


//--------------------------------------------------------------------------
/// Processing routine called by process and processReplacing
//--------------------------------------------------------------------------
void CHtVSTEqualizer::DoProcess (float **inputs, float **outputs, long sampleFrames, bool bReplace)
{
   if ((m_fEnabled <= 0.5) || !m_pFilter)
      {
      MoveMemory(outputs[0], inputs[0], sampleFrames*sizeof(float));
      MoveMemory(outputs[1], inputs[1], sampleFrames*sizeof(float));
      return;
      }
   else if (m_bMuted)
      {
      ZeroMemory(outputs[0], sampleFrames*sizeof(float));
      ZeroMemory(outputs[1], sampleFrames*sizeof(float));
      return;
      }

   try
      {
      memset(m_pfBuffer, 0, 2*blockSize* sizeof(float));

      long nSample;
      float* pf = &m_pfBuffer[0];
      // write data interleaved
      for (nSample = 0; nSample < sampleFrames; nSample++)
         {
         *pf++ = inputs[0][nSample];
         *pf++ = inputs[1][nSample];
         }

      // call filter
      DWORD dwLen = sampleFrames*sizeof(float)*2;
      DoFloatOlaFlt(m_pFilter, (char*)&m_pfBuffer[0], &dwLen);
      dwLen /= (sizeof(float)*2);



      if (bReplace)
         {
         memset(outputs[0], 0, sampleFrames * sizeof (float));
         memset(outputs[1], 0, sampleFrames * sizeof (float));
         }

      pf = &m_pfBuffer[0];
      // write data back with prepending zeros. NOTE: due to changed implementation
      // of Mmfloatolaflt this must never happen (see Mmfloatolaflt::ResetFloatOlaFlt(
      long nZeroes = sampleFrames - dwLen;
      if (nZeroes > 0)
         {
         memset(outputs[0], 0, nZeroes*sizeof(float));
         memset(outputs[1], 0, nZeroes*sizeof(float));
         }
      for (nSample = nZeroes; nSample < sampleFrames; nSample++)
         {
         outputs[0][nSample] = *pf++;
         outputs[1][nSample] = *pf++;
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
void CHtVSTEqualizer::process (float **inputs, float **outputs, long sampleFrames)
{
   DoProcess(inputs, outputs, sampleFrames, false);
}
//--------------------------------------------------------------------------

//--------------------------------------------------------------------------
/// replacing processing routine
//--------------------------------------------------------------------------
void CHtVSTEqualizer::processReplacing (float **inputs, float **outputs, long sampleFrames)
{
   DoProcess(inputs, outputs, sampleFrames, true);
}
//--------------------------------------------------------------------------

//--------------------------------------------------------------------------
/// initialize data
//--------------------------------------------------------------------------
void CHtVSTEqualizer::InitFilter()
{
   // alloc buffer for filter values and initialize as real filter with ones
   m_pmsFilter->Size = m_nFFTLen * sizeof(TfCplx);

   TfCplx* lp = (TfCplx*)m_pmsFilter->Memory;
   for (unsigned int i = 0; i < m_nFFTLen; i++)
      {
      lp[i].re = 1.0f;
      lp[i].im = 0.0f;
      }

   m_pfrmVisual->Initialize(m_nFFTLen, sampleRate);
}
//--------------------------------------------------------------------------

//--------------------------------------------------------------------------
/// alloc memory and OLA
//--------------------------------------------------------------------------
void CHtVSTEqualizer::InitOLA()
{
   ExitOLA();
   // alloc buffer for interleaved sample values
   m_pfBuffer = (float*)GlobalAllocMem(2*blockSize*sizeof(float));

   // initialize Ola-Filter
   int nRealBufSize  = Max(m_nFFTLen, blockSize);
   nRealBufSize *= 2*sizeof(float);

   m_pFilter = Mmfloatolaflt::InitFloatOlaFlt(2,
                                             m_nFFTLen,
                                             m_nWindowLen,
                                             m_nFWindowFeed,
                                             nRealBufSize,
                                             0
                                             );

   // and attach processing callback
   Mmfloatolaflt::SetFloatOlaSpectrumWaveReady(m_pFilter, FloatOlaSpectrumWaveReady);
}
//--------------------------------------------------------------------------

//--------------------------------------------------------------------------
/// free memory
//--------------------------------------------------------------------------
void CHtVSTEqualizer::ExitOLA()
{
   if (m_pFilter)
      {
      Mmfloatolaflt::DoneFloatOlaFlt(m_pFilter);
      m_pFilter = NULL;
      }
   if (m_pfBuffer)
      {
      GlobalFreeMem((void*)m_pfBuffer);
      m_pfBuffer = NULL;
      }
}
//--------------------------------------------------------------------------


//------------------------------------------------------------------------------
// LoadFilter only checks validity of values and converts values to a binary
// values for later interpolation but does no 'resampling' from frequencies to bins
//------------------------------------------------------------------------------
bool __fastcall CHtVSTEqualizer::LoadFilter(AnsiString sFileName)
{
   m_bMuted = true;

   bool bReturn = true;
   // temporary StringList for filter values from INI Filterfile
   TMemIniFile *pIni        = NULL;
   TStringList *pslFilter   = NULL;
   TStringList *pslValues   = NULL;
   m_pmsFileFilter->Clear();
   m_bComplex = false;

   try
      {
      if(!FileExists(sFileName))
         throw Exception("Filter file '" + sFileName + "' not found");
      pIni = new TMemIniFile(sFileName);

      bool bLog = pIni->ReadBool("Settings", "Log", false);

      // read FFT properties
      m_nFFTLen = pIni->ReadInteger("Settings", "FFTLen", 512);
      m_nFFTLen = ConvertToPowerOfTwo(MinMax(m_nFFTLen, 64, 32768));
      m_nWindowLen = pIni->ReadInteger("Settings", "WindowLen", 400);
      m_nWindowLen = MinMax(m_nWindowLen, m_nFFTLen/2, m_nFFTLen);
      AnsiString str = pIni->ReadString("Settings", "WindowShift", "0.5");
      MessageBox(0, str.c_str(), str.c_str() , 0);
      if (str == "0.5")
         m_nFWindowFeed = wffHalf;
      else if (str == "0.25")
         m_nFWindowFeed = wffQuarter;
      else if (str == "0.125")
         m_nFWindowFeed = wffEighth;
      else
         throw Exception("invalid WindowShift specified");


      pslFilter = new TStringList();
      pslValues = new TStringList();
      pslValues->Delimiter = ';';

      // Read the filter values to StringList
      pIni->ReadSectionValues("Filter", pslFilter);
      if (pslFilter->Count < 2)
         throw Exception("At least two values needed in 'Filter' section");

      m_pmsFileFilter->Size = pslFilter->Count * sizeof(TFilterValue);
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
            fv.freq = StrToDouble(strFreq);
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

            if (bLog)
               {
               if (fv.valR.im != 0.0 || fv.valL.im != 0.0)
                  throw Exception("logarithmic filters must not contain complex numbers");
               fv.valR.re = dBToFactor(fv.valR.re);
               fv.valL.re = dBToFactor(fv.valL.re);
               }

            lpFilterValues[i] = fv;
            }
         catch (Exception &e)
            {
            AnsiString sError;
            sError.sprintf("Filter File: format 'frequency=complex' or "
            "'frequency=complex;complex' expected (error: '%s' in line '%s', linenumber %d of section 'Filter')",
            e.Message.c_str(), pslFilter->Strings[i].c_str(), i);
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

      m_strFilterFile = sFileName;
      }
   catch (Exception &e)
      {
      AnsiString str = "Error loading filter file '" + sFileName + "': " + e.Message;
      m_pmsFileFilter->Clear();
      MessageBox(0, str.c_str(), "Error", MB_ICONERROR);
      bReturn = false;
      }
   TRYDELETENULL(pIni);
   TRYDELETENULL(pslFilter);
   TRYDELETENULL(pslValues);
   m_bMuted = false;
   return bReturn;

}
//------------------------------------------------------------------------------

//------------------------------------------------------------------------------
///
//------------------------------------------------------------------------------
bool __fastcall CHtVSTEqualizer::SaveFilter(AnsiString sFileName)
{
   if (m_bComplex)
      return false;
   DeleteFile(sFileName);
   TMemIniFile *pIni = new TMemIniFile(ExpandFileName(sFileName));
   try
      {
      pIni->WriteBool("Settings", "Log", false);
      // write FFT properties
      pIni->WriteInteger("Settings", "FFTLen", m_nFFTLen);
      pIni->WriteInteger("Settings", "WindowLen", m_nWindowLen);
      AnsiString sWindowShift = "0.5";
      if (m_nFWindowFeed == wffQuarter)
         sWindowShift = "0.25";
      else if (m_nFWindowFeed == wffEighth)
         sWindowShift = "0.125";
      pIni->WriteString("Settings", "WindowShift", sWindowShift);

      // go through filter
      TfCplx *lp = (TfCplx*)m_pmsFilter->Memory;
      DWORD dwBins = m_pmsFilter->Size / (2*sizeof(TfCplx));
      float f4BinSize = sampleRate / (float)m_nFFTLen;
      float fFreq = 0.0f;
      AnsiString sValue;
      EnterCriticalSection(&csDataSection);
      // NOTE: m_pmsFilter contains NON-interleaved data!!
      try
         {
         for (DWORD i = 0; i < dwBins; i++)
            {
            sValue =    DoubleToStr(lp[i].re);
            sValue += ";" + DoubleToStr(lp[dwBins+i].re);
            pIni->WriteString("Filter", DoubleToStr(fFreq), sValue);
            fFreq += f4BinSize;
            }
         }
      __finally
         {
         LeaveCriticalSection(&csDataSection);
         pIni->UpdateFile();
         }
      }
   catch (Exception &e)
      {
      TRYDELETENULL(pIni);
      AnsiString str = "Error saving filter file '" + sFileName + "': " + e.Message;
      MessageBox(0, str.c_str(), "Error", MB_ICONERROR);
      return false;
      }
   TRYDELETENULL(pIni);
   return true;
}
//------------------------------------------------------------------------------

//------------------------------------------------------------------------------
///
//------------------------------------------------------------------------------
bool __fastcall CHtVSTEqualizer::ChartToFilter()
{
   if (m_bComplex)
      return false;
   TMemoryStream* pms;
   try
      {
      int nNumValues = m_pmsFilter->Size / sizeof(TfCplx) / 2;
      if (  nNumValues != m_pfrmVisual->FilterSeriesL->YValues->Count
         || nNumValues != m_pfrmVisual->FilterSeriesR->YValues->Count
         )
         {
         MessageBox(0,"internal error 2",  "Error", 0);
         return false;
         }


      pms = new TMemoryStream();
      pms->Size = m_pmsFilter->Size;
      TfCplx* lpc = (TfCplx*)pms->Memory;
      for (int i = 0; i < nNumValues; i++)
         {
         lpc[i].re = m_pfrmVisual->FilterSeriesL->YValues->Value[i];
         lpc[i].im = 0.0f;
         lpc[nNumValues+i].re = m_pfrmVisual->FilterSeriesR->YValues->Value[i];
         lpc[nNumValues+i].im = 0.0f;
         }

      EnterCriticalSection(&csDataSection);
      try
         {
         m_pmsFilter->Clear();
         m_pmsFilter->CopyFrom(pms, 0);
         }
      __finally
         {
         LeaveCriticalSection(&csDataSection);
         }
      TRYDELETENULL(pms);
      }
   catch (Exception &e)
      {
      TRYDELETENULL(pms);
      throw;
      }

   return true;
}
//------------------------------------------------------------------------------

//------------------------------------------------------------------------------
///
//------------------------------------------------------------------------------
void __fastcall CHtVSTEqualizer::FilterToChart()
{
   int nNumValues = m_pmsFilter->Size / sizeof(TfCplx) / 2;

   TfCplx* lp = (TfCplx*)m_pmsFilter->Memory;
   TfCplx cplx;
   for (int i = 0; i < nNumValues; i++)
      {
      cplx = *lp++;
      m_pfrmVisual->FilterSeriesL->YValues->Value[i] = sqrt(cplx.re*cplx.re + cplx.im*cplx.im);
      }
   for (int i = 0; i < nNumValues; i++)
      {
      cplx = *lp++;
      m_pfrmVisual->FilterSeriesR->YValues->Value[i] = sqrt(cplx.re*cplx.re + cplx.im*cplx.im);
      }
   m_pfrmVisual->FilterSeriesL->Repaint();
   m_pfrmVisual->FilterSeriesR->Repaint();

   m_pfrmVisual->SetFilterProperties();

}
//------------------------------------------------------------------------------

//------------------------------------------------------------------------------
///
//------------------------------------------------------------------------------
void __fastcall CHtVSTEqualizer::ResampleFilter()
{
   TMemoryStream* pms = NULL;
   try
      {
      float f4BinSize = sampleRate / (float)m_nFFTLen;
      DWORD dwSamples = m_nFFTLen/2;
      int   nChannels = 2;

      // get number of filter values specified in file
      DWORD dwNumberOfFileValues = m_pmsFileFilter->Size / sizeof(TFilterValue);
      if (dwNumberOfFileValues <  2)
         throw Exception("Filter contains less than 2 values!");

      TFilterValue *lpFilterValues = (TFilterValue*)m_pmsFileFilter->Memory;

      pms = new TMemoryStream();
      pms->Size =   nChannels * dwSamples *sizeof(TfCplx);
      TfCplx* lpc = (TfCplx*)pms->Memory;

      float f4Frequency;
      TFilterValue fvLower, fvUpper;
      float f4FreqQuotient;


      for (int ch = 0; ch < nChannels; ch++)
         {
         for (UINT i = 0; i < dwSamples; i++)
            {
            f4Frequency = f4BinSize * (float)i;
            // first value is 0 Hz in bin-filter. We set all bins below first appearing
            // frequency in file filter to that first frequencies value
            if (f4Frequency <= lpFilterValues[0].freq)
               {
               if (!ch)
                  *lpc++ = lpFilterValues[0].valL;
               else
                  *lpc++ = lpFilterValues[0].valR;
               }
            //same on upper border
            else if (f4Frequency >= lpFilterValues[dwNumberOfFileValues-1].freq)
               {
               if (!ch)
                  *lpc++ = lpFilterValues[dwNumberOfFileValues-1].valL;
               else
                  *lpc++ = lpFilterValues[dwNumberOfFileValues-1].valR;
               }
            else //not below lower or above upper frequency
               {
               // not 'high performance': we seacrh for upper and lower freq for all bins...
               for (UINT j = 0; j < dwNumberOfFileValues; j++)
                  {
                  // found it exactly? Write value and break;
                  if (lpFilterValues[j].freq == f4Frequency)
                     {
                     if (!ch)
                        *lpc++ = lpFilterValues[j].valL;
                     else
                        *lpc++ = lpFilterValues[j].valR;
                     break;
                     }
                  // below desired freq? Remember and continue
                  else if (lpFilterValues[j].freq < f4Frequency)
                     fvLower = lpFilterValues[j];
                  // reaching this point we are above desired frequency and ready to interpolate!
                  else
                     {
                     fvUpper = lpFilterValues[j];
                     TFilterValue fv;
                     fv.freq = f4Frequency;
                     // (f-f1)/(f2-f1)
                     f4FreqQuotient = (fv.freq-fvLower.freq)/(fvUpper.freq-fvLower.freq);
                     if (!ch)
                        {
                        // y = y1 + (y2-y1) * f4FreqQuotient;
                        fv.valL.re = fvLower.valL.re + (fvUpper.valL.re-fvLower.valL.re) * f4FreqQuotient;
                        fv.valL.im = fvLower.valL.im + (fvUpper.valL.im-fvLower.valL.im) * f4FreqQuotient;
                        *lpc++ = fv.valL;
                        }
                     //same for right Channel
                     else
                        {
                        fv.valR.re = fvLower.valR.re + (fvUpper.valR.re-fvLower.valR.re) * f4FreqQuotient;
                        fv.valR.im = fvLower.valR.im + (fvUpper.valR.im-fvLower.valR.im) * f4FreqQuotient;
                        *lpc++ = fv.valR;
                        }
                     break;
                     }
                  // this should never happen: reaching the 'last filefilter' value
                  if (j == dwNumberOfFileValues-1)
                     throw Exception("Complex filter interpolation error 1");
                  }

               }
            }
         }

      //copy this generated filter from the chart series to the memorystream!
      EnterCriticalSection(&csDataSection);
      try
         {
         m_pmsFilter->Clear();
         m_pmsFilter->CopyFrom(pms, 0);
         }
      __finally
         {
         LeaveCriticalSection(&csDataSection);
         }
      FilterToChart();
      TRYDELETENULL(pms);
      }
   catch (Exception &e)
      {
      TRYDELETENULL(pms);
      throw;
      }
}
//------------------------------------------------------------------------------

//------------------------------------------------------------------------------
// attention!! spec is _not_ interleaved. But we have placed the floats in
// the deconvolution stream non-interleaved too: so we can simply iterate through
// all Bins!
//------------------------------------------------------------------------------
#pragma argsused
void __fastcall CHtVSTEqualizer::FloatOlaSpectrumWaveReady(TfCplx *lpSpectrum,
      DWORD dwBins, float *lpWaveData, DWORD dwSamples, bool &bUseWave)
{
   static int iNumCallbacks = 0;
   EnterCriticalSection(&csDataSection);
   try
      {
      bUseWave = false;

      //we decide here if spectra to be drawn, so that we dont have to check
      //on every sample
      if (m_pfrmVisual->Visible && !(iNumCallbacks % m_nUpdateInterval))
         {
         DWORD dwCalcBins = dwBins/2;
         for (UINT i = 0; i < dwCalcBins; i++)
            {
            m_pfrmVisual->PreSpecSeriesL->YValues->Value[i] = sqrt(lpSpectrum[i].re*lpSpectrum[i].re + lpSpectrum[i].im*lpSpectrum[i].im);
            m_pfrmVisual->PreSpecSeriesR->YValues->Value[i] = sqrt(lpSpectrum[dwCalcBins + i].re*lpSpectrum[dwCalcBins+i].re + lpSpectrum[dwCalcBins+i].im*lpSpectrum[dwCalcBins+i].im);
            }
         if (m_pfrmVisual->PreSpecSeriesL->Active)
            m_pfrmVisual->PreSpecSeriesL->Repaint();
         if (m_pfrmVisual->PreSpecSeriesR->Active)
            m_pfrmVisual->PreSpecSeriesR->Repaint();
         }


      TfCplx *lp = (TfCplx*)m_pmsFilter->Memory;
      if (m_bComplex)
         {
         TfCplx cplx;
         //calculate new spectrum
         for (UINT i = 0; i < dwBins; i++)
            {
            cplx = lpSpectrum[i];
            // do complex multiplication
            // z1 * z2 = (a + bi) * (c + di) = (ac - bd ) + (ad + bc)i.
            lpSpectrum[i].re = cplx.re * lp[i].re - cplx.im * lp[i].im;
            lpSpectrum[i].im = cplx.re * lp[i].im + cplx.im * lp[i].re;
            }
         }
      else
         {
         //calculate new spectrum
         for (UINT i = 0; i < dwBins; i++)
            {
            lpSpectrum[i].re *= lp[i].re;
            lpSpectrum[i].im *= lp[i].re;
            }
         }

      if (m_pfrmVisual->Visible && !(iNumCallbacks % m_nUpdateInterval))
         {
         DWORD dwCalcBins = dwBins/2;
         //decide, how to calculate: left, right, or both
         for (UINT i = 0; i < dwCalcBins; i++)
            {
            m_pfrmVisual->PostSpecSeriesL->YValues->Value[i] = sqrt(lpSpectrum[i].re*lpSpectrum[i].re + lpSpectrum[i].im*lpSpectrum[i].im);
            m_pfrmVisual->PostSpecSeriesR->YValues->Value[i] = sqrt(lpSpectrum[dwCalcBins + i].re*lpSpectrum[dwCalcBins+i].re + lpSpectrum[dwCalcBins+i].im*lpSpectrum[dwCalcBins+i].im);
            }
         if (m_pfrmVisual->PostSpecSeriesL->Active)
            m_pfrmVisual->PostSpecSeriesL->Repaint();
         if (m_pfrmVisual->PostSpecSeriesR->Active)
            m_pfrmVisual->PostSpecSeriesR->Repaint();

         }
      }
   __finally
      {
      LeaveCriticalSection(&csDataSection);
      iNumCallbacks++;
      }

}
//------------------------------------------------------------------------------

void CHtVSTEqualizer::ReadFilterNameFromIni()
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

//---------------------------------------------------------------------------
//             TOOLS
//---------------------------------------------------------------------------


//---------------------------------------------------------------------------
/// rounds a float value to an int
//---------------------------------------------------------------------------
int round(float f4Value)
{
if (  fabs(f4Value - floor(f4Value)) < 0.5)
   return (int)floor(f4Value);
else
   return (int)ceil(f4Value);
}
//---------------------------------------------------------------------------


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
//---------------------------------------------------------------------------

//---------------------------------------------------------------------------
//
//---------------------------------------------------------------------------
TfCplx  StrToComplex(AnsiString s)
{
   TfCplx c;
   s = Trim(s);
   //no 'i' in it? Must be real only!!
   if (s.Pos("i") == 0)
      {
      c.re = StrToDouble(s);
      c.im = 0;
      }
   else
      {
      char *endptr = NULL;
      //extract real part
      c.re = (float)strtod(s.c_str(), &endptr);
      //if its NULL, imaginary is missing
      if (!endptr)
         throw Exception("not a complex");
      //extract imaginary part
      c.im = (float)strtod(endptr, &endptr);
      //at the end there must be 'i' ...
      if (*endptr != 'i')
         throw Exception("not a complex");
      //... and nothing else behind the 'i'!
      if (!!(*(endptr+1)))
         throw Exception("not a complex");
      }
   return c;
}
//---------------------------------------------------------------------------

//---------------------------------------------------------------------------
//
//---------------------------------------------------------------------------
float dBToFactor(const float f4dB)
{
return pow(10,f4dB/20);
}
//---------------------------------------------------------------------------

//-----------------------------------------------------------------------------
//factor / dB conversion
//---------------------------------------------------------------------------
float FactorTodB(const float f4Factor)
{
if (f4Factor <= 0)
   return -1000.0;
return 20*log10(f4Factor);
}
//---------------------------------------------------------------------------


