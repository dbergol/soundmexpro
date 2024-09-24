//------------------------------------------------------------------------------
/// \file SoundDllPro_OutputChannelData.cpp
/// \author Berg
/// \brief Implementation of class SDPOutputData describing output data either
/// as memory block or as file descriptor (SDPFileChannel). Support for an offser
/// (zeros to be returned prior to data) and looping. Used by SDPOutput
/// to create track audio data.
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
#include <objbase.h>
#pragma hdrstop
#include "SoundDllPro_OutputChannelData.h"
#include "SoundDllPro_WaveReader_libsndfile.h"
#include "SoundDllPro_Main.h"

//------------------------------------------------------------------------------
extern bool g_bUseRamps;

//------------------------------------------------------------------------------
/// = operator of class SDPOD_AUDIO
//------------------------------------------------------------------------------
SDPOD_AUDIO& SDPOD_AUDIO::operator=(const SDPOD_AUDIO& r)
{
   nChannelIndex = r.nChannelIndex;
   nNumChannels  = r.nNumChannels;
   nNumSamples   = r.nNumSamples;
   nLoopCount    = r.nLoopCount;
   nOffset       = r.nOffset;
   nStartOffset  = r.nStartOffset;
   nFileOffset   = r.nFileOffset;
   nGlobalPosition = r.nGlobalPosition;
   fGain         = r.fGain;
   strName       = r.strName;
   pData         = r.pData;
   bIsFile       = r.bIsFile;
   nRampLenght       = r.nRampLenght;
   nLoopRampLenght   = r.nLoopRampLenght;
   bLoopCrossfade    = r.bLoopCrossfade;
   nCrossfadeLengthLeft    = r.nCrossfadeLengthLeft;
   nCrossfadeLengthRight   = r.nCrossfadeLengthRight;
   return *this;
}
//------------------------------------------------------------------------------

//------------------------------------------------------------------------------
/// constructor
//------------------------------------------------------------------------------
SDPOutputData::SDPOutputData(SDPOD_AUDIO &rsdpod, SDPOutputData* psdop)
   :  m_psdpodNext(NULL),
      m_bIsInUse(false),
      m_bReady(false),
      m_psdpwr(NULL),
      m_nPosition(0),
      m_nTotalPosition(0),
      m_nSingleLoopSamples((unsigned int)rsdpod.nNumSamples)
 {
   if (!SoundClass())
      throw Exception("invalid global SoundClass instance");
   m_sdopAudio = rsdpod;
   // check constraints that are identical for file AND mem
   if (!!m_sdopAudio.nOffset && !!m_sdopAudio.nCrossfadeLengthLeft)
      throw Exception("offset must not be specified if crossfade length is != 0");
   if (m_sdopAudio.bIsFile)
      InitializeFileWav();
   else
      InitializeMemWav(psdop);
   SetPosition(0);
   m_strID = InitializeGUID();
   m_bIsInUse = true;
}
//------------------------------------------------------------------------------

//------------------------------------------------------------------------------
/// inti procedure for memory type of audio data. Initialises members and copies
/// audio data to internal memory buffer
//------------------------------------------------------------------------------
void SDPOutputData::InitializeMemWav(SDPOutputData* psdop)
{
   m_vafBuffer.resize((unsigned int)m_nSingleLoopSamples);
   // check start offset (applied only for first loop!
   if (m_sdopAudio.nStartOffset >= m_nSingleLoopSamples - m_sdopAudio.nLoopRampLenght)
      throw Exception("start offset exceeds vector length - loopramp length");
   // read data
   // - if a 'source' SDPOutputData was passed, then we copy the buffer
   // from the source
   if (!!psdop)
      m_vafBuffer = psdop->m_vafBuffer;
   // otherwise data are passed from MATLAB in m_sdopAudio.pData
   else
      {
      unsigned int nSample;
      // NOTE: data are non-interleaved in pData!!!
      m_sdopAudio.pData += m_nSingleLoopSamples*m_sdopAudio.nChannelIndex;
      for (nSample = 0; nSample < m_nSingleLoopSamples; nSample++)
         {
         m_vafBuffer[nSample] = (float)*m_sdopAudio.pData++;
         }
      }
   bool bCrossfade = m_sdopAudio.nLoopRampLenght && m_sdopAudio.bLoopCrossfade;
   m_nPosition = (uint64_t)m_sdopAudio.nStartOffset;
   // NOTE: Loopcount 0 must ALWAYS lead to total length of 0 (endless!!!)
   if (!m_sdopAudio.nLoopCount)
      m_nTotalLength = 0;
   else
      {
      m_nTotalLength = m_nSingleLoopSamples*m_sdopAudio.nLoopCount - m_nPosition;
      // in crossfade mode we have to adjust the total length here BEFORE calling
      // InitializeRamps (which checks constraints depending on total length!!
      if (bCrossfade)
         m_nTotalLength -= ((m_sdopAudio.nLoopCount-1) * m_sdopAudio.nLoopRampLenght);
      }
   InitializeRamps();
   // in crossfade mode we now load the first nLoopRampLenght samples 'ramped-up'
   // to m_vafCrossfadeBuffer
   if (bCrossfade)
      {
      m_vafCrossfadeBuffer.resize((unsigned int)m_sdopAudio.nLoopRampLenght);
      unsigned int n;
      for (n = 0; n < m_sdopAudio.nLoopRampLenght; n++)
         {
         if (!g_bUseRamps)
            m_vafCrossfadeBuffer[n] = m_vafBuffer[n];
         else
            m_vafCrossfadeBuffer[n] = m_vafBuffer[n] * GetHanningRamp(n, m_sdopAudio.nLoopRampLenght, true);
         }
      }
}
//------------------------------------------------------------------------------

//------------------------------------------------------------------------------
/// inti procedure for file type of audio data. Initialises members and creates an
/// instance of SDPFileChannel for accessing one channel of an audio file
//------------------------------------------------------------------------------
void SDPOutputData::InitializeFileWav()
{
   // calculate minimum needed file buffer size
   unsigned int nMinBufsize = (unsigned int)SoundClass()->SoundBufsizeSamples() * SoundClass()->GetQueueNumBufs();
   m_psdpwr = new SDPWaveReader( m_sdopAudio,
                                 (unsigned int)SoundClass()->SoundGetSampleRate(),
                                 nMinBufsize,
                                 // never use time critical priority for file reading...
                                 tpHighest // SoundClass()->ThreadPriority()
                                 );

   // NOTE: start offset and crossfade length is handled by file itself and _not_ to
   // be subtracted here
   m_nTotalLength = m_psdpwr->TotalLength();
   m_nSingleLoopSamples = (unsigned int)m_psdpwr->UsedLength();
   InitializeRamps();
   // in crossfade mode we now to load the first nLoopRampLenght samples 'ramped-up'
   // to m_vafCrossfadeBuffer.
   if (m_sdopAudio.nLoopRampLenght && m_sdopAudio.bLoopCrossfade)
      {
      // create buffer with first nLoopRampLenght up-ramped samples of object
      // for later crossfades
      m_vafCrossfadeBuffer.resize(m_sdopAudio.nLoopRampLenght);
      // here we use a second instance of SDPWaveReader, where we keep fileoffset
      // but neglect startoffset!!
      SDPOD_AUDIO sdpod = m_sdopAudio;
      sdpod.nStartOffset = 0;
      SDPWaveReader* psdpwr = NULL;
      try
         {
         psdpwr = new SDPWaveReader( sdpod,
                                    (unsigned int)SoundClass()->SoundGetSampleRate(),
                                    nMinBufsize,
                                    // never use time critical priority for file reading...
                                    tpHighest // SoundClass()->ThreadPriority()
                                    );
         psdpwr->SetPosition(0);
         unsigned int n;
         for (n = 0; n < m_sdopAudio.nLoopRampLenght; n++)
            {
            if (!g_bUseRamps)
               m_vafCrossfadeBuffer[n] = psdpwr->GetFileSample();
            else
               m_vafCrossfadeBuffer[n] = psdpwr->GetFileSample() * GetHanningRamp(n, m_sdopAudio.nLoopRampLenght, true);
            }
         }
      __finally
         {
         TRYDELETENULL(psdpwr);
         }
      }
}
//------------------------------------------------------------------------------

//------------------------------------------------------------------------------
/// initializes 'master' ramp and 'loop' ramp
//------------------------------------------------------------------------------
void SDPOutputData::InitializeRamps()
{
   if (m_nTotalLength > 0 && m_sdopAudio.nRampLenght > (m_nTotalLength/2))
      throw Exception("ramp lengths must not exceed half of total playback length of object");   // disable loop ramp if only one loop!
   if (m_sdopAudio.nLoopCount == 1)
      m_sdopAudio.nLoopRampLenght = 0;
   if (m_sdopAudio.nLoopRampLenght > (m_nSingleLoopSamples)/2)
      throw Exception("loop ramp lengths must not exceed half of length of object");
   if (m_sdopAudio.bLoopCrossfade && m_sdopAudio.nLoopRampLenght > 1000000)
      throw Exception("loop ramp lengths must not exceed 1000000 samples in crossfade mode");
   if (m_sdopAudio.nCrossfadeLengthLeft > m_nTotalLength/2)
      throw Exception("crossfade length must not exceed half of total playback length of object");
}
//------------------------------------------------------------------------------

//------------------------------------------------------------------------------
/// returns a GUID to store is as ID of channel
//------------------------------------------------------------------------------
AnsiString SDPOutputData::InitializeGUID()
{
   //create ID
   GUID guid;
   if (S_OK != CoCreateGuid(&guid))
      throw Exception(_T("Error creating a GUID"));
   return GUIDToString(guid);
}
//------------------------------------------------------------------------------

//------------------------------------------------------------------------------
/// destructor. Does cleanup
//------------------------------------------------------------------------------
SDPOutputData::~SDPOutputData()
{
   m_vafBuffer.resize(0);
   m_vafCrossfadeBuffer.resize(0);
   TRYDELETENULL(m_psdpwr);
}
//------------------------------------------------------------------------------

//------------------------------------------------------------------------------
/// Resets data for reuse to a certain position.
//------------------------------------------------------------------------------
void SDPOutputData::SetPosition(uint64_t nPosition)
{
   // NOTE: caller must take care of processing critical section!!
   if (!!m_nTotalLength && nPosition >= m_nTotalLength)
      throw Exception("SDPOutputData reset position error (" + m_sdopAudio.strName + "): " + IntToStr((int)nPosition) + ", " + IntToStr((int)m_nTotalLength));
   m_nTotalPosition  = nPosition;
   // calculate Position within data
   if (m_sdopAudio.bIsFile)
      m_psdpwr->SetPosition(nPosition);
   else
      {
      // NOTE: the first m_vafCrossfadeBuffer.size() samples are
      // only used in the very first loop (later the crossfade buffer
      // is used for adding up). Thus we have to check, if we are within
      // the first loop
      if (nPosition < (uint64_t)(m_vafBuffer.size() - (uint64_t)m_sdopAudio.nStartOffset))
         m_nPosition = (uint64_t)(m_sdopAudio.nStartOffset) + nPosition;
      else
         {
         nPosition -= (uint64_t)(m_vafBuffer.size() - (uint64_t)m_sdopAudio.nStartOffset);
         // afterwards we have to set position modulo m_vafBuffer.size() - m_vafCrossfadeBuffer.size() and
         // add m_vafCrossfadeBuffer.size() (first samples used from crossfade-buffer!
         m_nPosition = (nPosition % (m_vafBuffer.size()- m_vafCrossfadeBuffer.size())) + m_vafCrossfadeBuffer.size();
         }
      // OutputDebugStringW(("M_pos: " + IntToStr((int)m_nPosition)).w_str());
      }
   m_bIsInUse        = true;
}
//------------------------------------------------------------------------------

//------------------------------------------------------------------------------
/// returns total position i.e. playback position within looped object
//------------------------------------------------------------------------------
uint64_t SDPOutputData::GetPosition()
{
   return m_nTotalPosition;
}
//------------------------------------------------------------------------------

//------------------------------------------------------------------------------
/// returns one sample from memory or file respectively with respect to looping
/// and offset (prepending zeros)
//------------------------------------------------------------------------------
float SDPOutputData::GetSample(bool bNoRamp)
{
   float fValue;
   m_nTotalPosition++;
   uint64_t uLoopPosition = 0;
   if (m_sdopAudio.bIsFile)
      {
      try
         {
         uLoopPosition = m_psdpwr->GetLoopPosition();
         // retrieve sample from file. NOTE: looping is done by class SDPWaveReader!
         fValue = m_psdpwr->GetFileSample();
         // check if file is done now
         if (m_psdpwr->Done())
            m_bIsInUse = false;
         }
      catch (Exception &e)
         {
         throw Exception("unexpected exception when retrieving a file sample: " + e.Message + " (" + AnsiString(e.ClassName()) + ")");
         }
      }
   else                                            
      {
      unsigned int nSize      = (unsigned int)m_vafBuffer.size();
      // anything to play?
      if (!nSize || !m_bIsInUse)
         throw Exception("trying to retrieve sample from empty or 'not-in-use' SDPOutputData");
      uLoopPosition = m_nPosition;
      // get value. NOTE: m_nPosition is only incremented and checked in this function,
      // so we don't check on function entry!
      fValue = m_vafBuffer[(unsigned int)m_nPosition++];
      // check for loop of main buffer
      if (m_nPosition >= nSize)
         {
         // reset in-use-flag data if done
         if (!!m_nTotalLength && m_nTotalPosition >= m_nTotalLength)
            m_bIsInUse = false;
         // otherwise reset internal position to play next loop
         else
            m_nPosition = m_vafCrossfadeBuffer.size();
         }
      }
   // Now sample is retrieved. Apply ramp, loopramp and crossfade
   // apply overall ramp.
   // NOTE: initial 'ramp up' is already activated in constructor.
   if (g_bUseRamps)
      {
      // 'regular' ramp
      if (m_sdopAudio.nRampLenght && !bNoRamp)
         {
         // ramp up?
         if (m_nTotalPosition <= m_sdopAudio.nRampLenght)
            fValue *= GetHanningRamp((unsigned int)m_nTotalPosition, m_sdopAudio.nRampLenght, true);
         // ramp down?
         else if (RemainingLength() <= m_sdopAudio.nRampLenght)
            fValue *= GetHanningRamp((unsigned int)(m_sdopAudio.nRampLenght - RemainingLength()), m_sdopAudio.nRampLenght, false);
         }
      // crossfade ramps
      if (m_sdopAudio.nCrossfadeLengthLeft && !bNoRamp && m_nTotalPosition <= m_sdopAudio.nCrossfadeLengthLeft)
         fValue *= GetHanningRamp((unsigned int)m_nTotalPosition, m_sdopAudio.nCrossfadeLengthLeft, true);
      if (m_sdopAudio.nCrossfadeLengthRight && !bNoRamp && RemainingLength() <= m_sdopAudio.nCrossfadeLengthRight)
         fValue *= GetHanningRamp((unsigned int)(m_sdopAudio.nCrossfadeLengthRight - RemainingLength()), m_sdopAudio.nCrossfadeLengthRight, false);
      }
   // apply looping ramp
   if (m_sdopAudio.nLoopRampLenght)
      {
      // loop ramp up?
      // NOTE: is NOT done in the very beginning!
      // NOTE: in crossfade-mode the first condition is never true for loops other
      // than the first, because the first samples are skipped and then up-ramped
      // samples are added instead below. The first loop is skipped due to second
      // condition -> everything's fine!
      if (  uLoopPosition < m_sdopAudio.nLoopRampLenght
         && (m_nTotalPosition) > m_sdopAudio.nLoopRampLenght)
         {
         if (g_bUseRamps)
            fValue *= GetHanningRamp((unsigned int)uLoopPosition, m_sdopAudio.nLoopRampLenght, true);
         }
      // loop ramp down?
      // NOTE: is NOT done in the end, as well as adding up crossfade value!
      else if (   (m_nSingleLoopSamples - uLoopPosition ) <= m_sdopAudio.nLoopRampLenght
               && RemainingLength() > (m_sdopAudio.nLoopRampLenght-1))
         {
         unsigned int nPos = m_sdopAudio.nLoopRampLenght - (unsigned int)(m_nSingleLoopSamples - uLoopPosition);
         if (g_bUseRamps)
            fValue *= GetHanningRamp(nPos, m_sdopAudio.nLoopRampLenght, false);
         if (m_vafCrossfadeBuffer.size())
            fValue += m_vafCrossfadeBuffer[nPos];
         }
      }
   // finally apply data-gain
   return fValue*m_sdopAudio.fGain;
}
//------------------------------------------------------------------------------

//------------------------------------------------------------------------------
/// returns true, if data to be played in endless loop or false else
//------------------------------------------------------------------------------
bool SDPOutputData::IsEndlessLoop()
{
   if (!m_sdopAudio.bIsFile)
      return (m_sdopAudio.nLoopCount == 0);
   try
      {
      if (m_psdpwr)
         return m_psdpwr->IsEndlessLoop();
      }
   catch (...)
      {
      }
   return false;
}
//------------------------------------------------------------------------------

//------------------------------------------------------------------------------
/// returns total playback length of data
//------------------------------------------------------------------------------
uint64_t SDPOutputData::TotalLength()
{
   return m_nTotalLength;
}
//------------------------------------------------------------------------------

//------------------------------------------------------------------------------
/// returns remaining playback length of data
//------------------------------------------------------------------------------
uint64_t SDPOutputData::RemainingLength()
{
   return m_nTotalLength - m_nTotalPosition;
}
//------------------------------------------------------------------------------

//------------------------------------------------------------------------------
/// Adjusts global position
//------------------------------------------------------------------------------
void SDPOutputData::SetGlobalPosition(uint64_t nGlobalPosition)
{
   if (m_bReady)
	  throw Exception("GlobalPosition cannot be changed after data are set to 'ready'");
   m_sdopAudio.nGlobalPosition = nGlobalPosition - m_sdopAudio.nCrossfadeLengthLeft;
}
//------------------------------------------------------------------------------

//------------------------------------------------------------------------------
/// returns either filename or data name
//------------------------------------------------------------------------------
AnsiString SDPOutputData::Name()
{
   AnsiString str;
   if (m_sdopAudio.bIsFile)
	  {
	  str = m_psdpwr->GetFileName();
	  // append indication, if not full file is used
      if (  m_psdpwr->UsedLength() != m_psdpwr->FileSize()
         || (m_sdopAudio.nLoopCount == 1 && GetStartOffset() != 0)
         )
         str += " (part)";
      }
   else
      str = m_sdopAudio.strName.IsEmpty() ? AnsiString("unnamed vector") : m_sdopAudio.strName;
   return str;
}
//------------------------------------------------------------------------------

//------------------------------------------------------------------------------
/// returns either filename or data name
//------------------------------------------------------------------------------
AnsiString SDPOutputData::Id()
{
   return m_strID;
}
//------------------------------------------------------------------------------

//------------------------------------------------------------------------------
/// returns flag, if it's file or memory
//------------------------------------------------------------------------------
bool SDPOutputData::IsFile()
{
   return m_sdopAudio.bIsFile;
}
//------------------------------------------------------------------------------

//------------------------------------------------------------------------------
/// returns length in samples of a single loop of pure data without offset
//------------------------------------------------------------------------------
unsigned int SDPOutputData::LengthSingleLoop()
{
   unsigned int nReturn = m_nSingleLoopSamples;
   if (m_sdopAudio.bLoopCrossfade)
      nReturn -= m_sdopAudio.nLoopRampLenght;
   return nReturn;
}
//------------------------------------------------------------------------------

//------------------------------------------------------------------------------
/// returns loop count
//------------------------------------------------------------------------------
unsigned int SDPOutputData::LoopCount()
{
   return m_sdopAudio.nLoopCount;
}
//------------------------------------------------------------------------------

//------------------------------------------------------------------------------
/// returns file offset (or 0 for vectors)
//------------------------------------------------------------------------------
uint64_t SDPOutputData::GetFileOffset()
{
   if (IsFile())
	  return m_psdpwr->GetFileOffset();
   return 0;
}
//------------------------------------------------------------------------------

//------------------------------------------------------------------------------
/// returns used file length file (which is 0 if complete file is used)
/// (or 0 for vectors)
//------------------------------------------------------------------------------
uint64_t SDPOutputData::GetFileUsedLength()
{
   if (IsFile())
	  return m_psdpwr->UsedLength();
   return 0;
}
//------------------------------------------------------------------------------

//------------------------------------------------------------------------------
/// returns start offset
//------------------------------------------------------------------------------
uint64_t SDPOutputData::GetStartOffset()
{
   return (uint64_t)m_sdopAudio.nStartOffset;
}
//------------------------------------------------------------------------------

//------------------------------------------------------------------------------
/// returns global position of first sample
//------------------------------------------------------------------------------
uint64_t SDPOutputData::GetGlobalPosition()
{
   return m_sdopAudio.nGlobalPosition;
}
//------------------------------------------------------------------------------

//------------------------------------------------------------------------------
/// returns global position of first sample
//------------------------------------------------------------------------------
uint64_t SDPOutputData::GetTotalLength()
{
   return m_nTotalLength;
}
//------------------------------------------------------------------------------
