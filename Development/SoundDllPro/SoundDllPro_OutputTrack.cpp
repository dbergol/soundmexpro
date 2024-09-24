//------------------------------------------------------------------------------
/// \file SoundDllPro_OutputTrack.cpp
/// \author Berg
/// \brief Implementation of class SDPTrack: a virtual track that may contain
/// multiple subsequent instances of SDPOutputData. These multiple instances
/// are stored in a linked list
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
#pragma hdrstop

#include "SoundDllPro_OutputTrack.h"
#include "SoundDllPro_Main.h"

//------------------------------------------------------------------------------

//------------------------------------------------------------------------------
/// constructor. Intialises members
//------------------------------------------------------------------------------
SDPTrack::SDPTrack(unsigned int nTrackIndex, unsigned int nChannelIndex, bool bAutoCleanup)
   :  m_bNotify(false),
      m_nTrackIndex(nTrackIndex),
      m_nChannelIndex(nChannelIndex),
      m_bMultiply(false),
      m_nDataUnderrun(0),
      m_nDataUnderrunPos(-1),
      m_bEndlessLoop(false),
      m_bAutoCleanup(bAutoCleanup)
      
{
   if (!SoundClass())
      throw Exception("global SoundClass is invalid");
   InitializeCriticalSection(&m_csLock);

   m_strName = "Track " + IntToStr((int)m_nTrackIndex);
   m_psdpod[0]    = NULL;
   m_psdpod[1]    = NULL;
}
//------------------------------------------------------------------------------

//------------------------------------------------------------------------------
// destructor. calls Cleanup
//------------------------------------------------------------------------------
SDPTrack::~SDPTrack()
{
   Cleanup(true);
   DeleteCriticalSection(&m_csLock);
}
//------------------------------------------------------------------------------

//------------------------------------------------------------------------------
/// resets underrun flag
//------------------------------------------------------------------------------
void SDPTrack::Start()
{
   m_nDataUnderrun = 0;
}
//------------------------------------------------------------------------------

//------------------------------------------------------------------------------
/// resets underrun flag and calls Cleanup
//------------------------------------------------------------------------------
void SDPTrack::Stop()
{
   m_nDataUnderrun = 0;
   Cleanup(true);
}
//------------------------------------------------------------------------------

//------------------------------------------------------------------------------
/// Cleans up unused (bAllInstances == false) or all (bAllInstances == true)
/// stored instances of SDPOutputData and sets internal endless-loop-flag afterwards
//------------------------------------------------------------------------------
void SDPTrack::Cleanup(bool bAllInstances)
{
   EnterCriticalSection(&m_csLock);
   try
      {
      SDPOutputData* psdpod;

      m_bEndlessLoop    = false;

      while (m_psdpod[0])
         {
         // leave here, if still in use and not 'all' flag set
         if (m_psdpod[0]->m_bIsInUse && !bAllInstances)
            break;
         psdpod = m_psdpod[0]->m_psdpodNext;

         TRYDELETENULL(m_psdpod[0]);
         m_psdpod[0] = psdpod;
         }
      if (!m_psdpod[0])
         m_psdpod[1] = NULL;

      // check if any endless looped data buffer is left
      psdpod = m_psdpod[1];
      while (psdpod)
         {
         if (psdpod->IsEndlessLoop())
            {
            m_bEndlessLoop = true;
            break;
            }
         psdpod = psdpod->m_psdpodNext;
         }

      if (bAllInstances)
         {
         m_nDataUnderrun = 0;
         m_nDataUnderrunPos = -1;
         }
      }
   __finally
      {
      LeaveCriticalSection(&m_csLock);
      }
}
//------------------------------------------------------------------------------

//------------------------------------------------------------------------------
/// creates a new SDPOutputData object containing audio data and appends
/// it to internal linked list
//------------------------------------------------------------------------------
SDPOutputData* SDPTrack::LoadAudio(SDPOD_AUDIO& rsdpod)
{
   // cleanup unused data
   if (m_bAutoCleanup)
      Cleanup(false);

   AssertDataAllowed();

   SDPOutputData* psdpod = NULL;
   EnterCriticalSection(&m_csLock);
   try
      {
      // search last loaded data class (where next-pointer is NULL)
      SDPOutputData* psdpodTmp = m_psdpod[0];
      while (psdpodTmp && psdpodTmp->m_psdpodNext)
         psdpodTmp = psdpodTmp->m_psdpodNext;



      if (rsdpod.nLoopCount == 0)
         m_bEndlessLoop = true;


      // copy left crossfade length from current object to right crossfade length
      // of last loaded object - if any
      if (psdpodTmp)
         psdpodTmp->m_sdopAudio.nCrossfadeLengthRight = rsdpod.nCrossfadeLengthLeft;
      // if no object before available, the rest left crossfade length of new object to 0
      // since it does not make sense!
      else
         rsdpod.nCrossfadeLengthLeft = 0;

      // set global position within track
      rsdpod.nGlobalPosition = NumTrackSamples() + rsdpod.nOffset - rsdpod.nCrossfadeLengthLeft;
      psdpod = new SDPOutputData(rsdpod);

      // if no data at all, set first and current data pointer!
      if (!m_psdpod[0])
         {
         m_psdpod[0] = psdpod;
         m_psdpod[1] = psdpod;
         }
      // otherwise append it
      else
         {
         psdpodTmp->m_psdpodNext = psdpod;
         // if no current data set it as well
         if (!m_psdpod[1])
            m_psdpod[1] = psdpod;
         }
      }
   __finally
      {
      LeaveCriticalSection(&m_csLock);
      }
   return psdpod;
}
//------------------------------------------------------------------------------

//------------------------------------------------------------------------------
/// returns first global sample position where NOW data are loaded yet
//------------------------------------------------------------------------------
uint64_t SDPTrack::NumTrackSamples()
{
   uint64_t n = 0;
   SDPOutputData* psdpodTmp = m_psdpod[0];
   if (!!psdpodTmp)
      {
      // search last loaded data class (where next-pointer is NULL)
      while (psdpodTmp->m_psdpodNext)
         psdpodTmp = psdpodTmp->m_psdpodNext;
      n = psdpodTmp->GetGlobalPosition() + psdpodTmp->GetTotalLength();
      }

   return n;
}
//------------------------------------------------------------------------------

//------------------------------------------------------------------------------
/// writes audio data from track to passed valarray
//------------------------------------------------------------------------------
void SDPTrack::GetBuffer(std::valarray<float>& vafBuffer)
{
   EnterCriticalSection(&m_csLock);
   uint64_t nPos = SoundClass()->GetLoadPosition();
   try
      {
      if (m_bMultiply)
         vafBuffer = 1.0f;
      else
         vafBuffer = 0.0f;

      unsigned int nSamples = (unsigned int)vafBuffer.size();
      for (unsigned int n = 0; n < nSamples; n++,nPos++)
         {
         if (m_psdpod[1] && m_psdpod[1]->m_bReady)
            {
            // check if we have reached startposition of audio snippet at all
            // (otherwise play zero (or one for mutiplying track, see above)
            if (nPos > m_psdpod[1]->GetGlobalPosition())
               {
               vafBuffer[n] = m_psdpod[1]->GetSample();

               // if we have a crossfade between different SDPOutputData and
               // are within the crossfade ramp, then we have to retrieve a sample
               // from the next (!!) buffer and add it up
               if (  m_psdpod[1]->m_sdopAudio.nCrossfadeLengthRight
                  && m_psdpod[1]->m_psdpodNext
                  && m_psdpod[1]->RemainingLength() < m_psdpod[1]->m_sdopAudio.nCrossfadeLengthRight
                  )
                  vafBuffer[n] += m_psdpod[1]->m_psdpodNext->GetSample();




               // if not in use any longer, then it's done: set current data
               // pointer to next
               if (!m_psdpod[1]->m_bIsInUse)
                  {
                  if (m_bNotify)
                     SoundClass()->m_lpfnExtDataNotify();

                  m_psdpod[1] = m_psdpod[1]->m_psdpodNext;
                  }
               }
            m_nDataUnderrunPos = -1;
            }
         else
            {
            m_nDataUnderrun++;
            if (m_nDataUnderrunPos == -1)
               m_nDataUnderrunPos = (int64_t)nPos;
            }
         }
      }
   __finally
      {
      LeaveCriticalSection(&m_csLock);
      }
}
//------------------------------------------------------------------------------


//------------------------------------------------------------------------------
/// returns current 'is-endless-loop' status
//------------------------------------------------------------------------------
bool  SDPTrack::EndlessLoop()
{
   return m_bEndlessLoop;
}
//------------------------------------------------------------------------------

//------------------------------------------------------------------------------
/// checks, if data may be added to track now. Forbidden, if 'is-endless-loop'
/// status is true
//------------------------------------------------------------------------------
void  SDPTrack::AssertDataAllowed()
{
   // New data are _not_ allowed, if
   // -  channel is running an endless loop
   if (m_bEndlessLoop)
      throw Exception("cannot load data to a channel where data where loaded for endless loop (loopcount=0). Call 'cleardata' to clear previously loaded data");
}
//------------------------------------------------------------------------------

//------------------------------------------------------------------------------
/// iterates through all stored instances of SDPOutputData and sets their 'ready'
/// flag to true
//------------------------------------------------------------------------------
void  SDPTrack::PushData()
{
   // set 'ready' flag for all non-used data segments to true
   // NOTE: we access element [1] because 'old' (done) segments does not
   // have to be set
   SDPOutputData* sdpod = m_psdpod[1];
   while (!!sdpod)
      {
      sdpod->m_bReady = true;
      sdpod = sdpod->m_psdpodNext;
      }
}
//------------------------------------------------------------------------------

//------------------------------------------------------------------------------
/// returns true, if 'ready data' are present in track now. 
//------------------------------------------------------------------------------
bool SDPTrack::HasData()
{
   return (!!m_psdpod[1] && m_psdpod[1]->m_bReady);
}
//------------------------------------------------------------------------------

//------------------------------------------------------------------------------
/// returns, if a data underrun occurred at any time
//------------------------------------------------------------------------------
bool SDPTrack::Underrun()
{
   return m_nDataUnderrun > 0;
}
//------------------------------------------------------------------------------

//------------------------------------------------------------------------------
/// Checks, if track is playing data _and_ device is running.
//------------------------------------------------------------------------------
bool SDPTrack::IsPlaying()
{
   if (!SoundClass()->DeviceIsRunning())
      return false;

   // NOTE: track is (still) playing data if
   // - no underrun occurred 'ever'
   // - underrun occurred, but 'underrun position' has not reached (audible) output yet
   // - track has data at all
   return   (  (m_nDataUnderrunPos < 0)
            || (uint64_t)m_nDataUnderrunPos > SoundClass()->GetSamplePosition()
            || ( HasData() )
            );
}
//------------------------------------------------------------------------------

//------------------------------------------------------------------------------
/// returns data length in samples that are remaining in track at the moment
//------------------------------------------------------------------------------
int64_t SDPTrack::RemainingDataLength()
{
   int64_t nLength = 0;
   SDPOutputData* sdpod = m_psdpod[1];
   while (!!sdpod)
      {
      nLength += sdpod->RemainingLength();
      sdpod = sdpod->m_psdpodNext;
      }
   return nLength;
}
//------------------------------------------------------------------------------

//------------------------------------------------------------------------------
/// returns number of instances of SDPOutputData 'pending' for output at the moment
//------------------------------------------------------------------------------
unsigned int SDPTrack::TrackLoad()
{
   unsigned int nNum = 0;
   if (!!m_psdpod[1])
      {
      nNum = 1;
      SDPOutputData* psdpodTmp = m_psdpod[1];
      while (psdpodTmp->m_psdpodNext)
         {
         nNum++;
         psdpodTmp = psdpodTmp->m_psdpodNext;
         }
      }
   return nNum;
}
//------------------------------------------------------------------------------

//------------------------------------------------------------------------------
/// returns channel index, i.e. output channel, where track is mapped to
//------------------------------------------------------------------------------
unsigned int SDPTrack::ChannelIndex()
{
   return m_nChannelIndex;
}
//------------------------------------------------------------------------------

//------------------------------------------------------------------------------
/// sets channel index, i.e. output channel, where track is mapped to
//------------------------------------------------------------------------------
void SDPTrack::ChannelIndex(unsigned int nChannelIndex)
{
   m_nChannelIndex = nChannelIndex;
}
//------------------------------------------------------------------------------

//------------------------------------------------------------------------------
/// returns, if the track is a 'multiplier' or an 'adder'
//------------------------------------------------------------------------------
bool SDPTrack::Multiply()
{
   return m_bMultiply;
}
//------------------------------------------------------------------------------

//------------------------------------------------------------------------------
/// sets 'add-or-multiply' status
//------------------------------------------------------------------------------
void SDPTrack::Multiply(bool bMultiply)
{
   m_bMultiply = bMultiply;
}
//------------------------------------------------------------------------------

//------------------------------------------------------------------------------
/// returns, if the track does auto cleanup of unused data
//------------------------------------------------------------------------------
bool SDPTrack::AutoCleanup()
{
   return m_bAutoCleanup;
}
//------------------------------------------------------------------------------

//------------------------------------------------------------------------------
/// sets if the track does auto cleanup of unused data
//------------------------------------------------------------------------------
void SDPTrack::AutoCleanup(bool bAutoCleanup)
{
   m_bAutoCleanup = bAutoCleanup;
}
//------------------------------------------------------------------------------

//------------------------------------------------------------------------------
/// resets track to a certain sample position
//------------------------------------------------------------------------------
void SDPTrack::SetPosition(uint64_t nPosition)
{
   // NOTE: caller must take care of processing critical section!!
   SDPOutputData* psdpod = m_psdpod[0];
   // if requested position is BEHIND all data of track we simply set all to done
   if (!EndlessLoop() && NumTrackSamples() < nPosition)
      {
      while (psdpod)
         {
         psdpod->m_bIsInUse = false;
         psdpod = psdpod->m_psdpodNext;
         }
      }
   else
      {
      int64_t nPos;
      // find sample position where to start playback again
      bool bStarted     = false;
      while (psdpod)
         {
         // continue until we need to reset the first buffer
         if (!bStarted)
            {
            //
            if (  psdpod->IsEndlessLoop()
               || nPosition >= psdpod->GetGlobalPosition()
               )
               {
               nPos = (int64_t)(nPosition - psdpod->GetGlobalPosition());
               // position to set within THIS buffer - or behind?
               if (nPos < (int64_t)psdpod->TotalLength())
                  {
                  bStarted = true;
                  psdpod->SetPosition(nPosition - psdpod->GetGlobalPosition());
                  // adjust current playback pointer!
                  m_psdpod[1] = psdpod;
                  }
               else
                  psdpod->m_bIsInUse = false;
               }
            }
         // reset all later buffers to very start
         else
            psdpod->SetPosition(0);

         psdpod = psdpod->m_psdpodNext;
         }
      }
}
//------------------------------------------------------------------------------

//------------------------------------------------------------------------------
/// returns track name
//------------------------------------------------------------------------------
AnsiString SDPTrack::Name(void)
{
   return m_strName;
}
//------------------------------------------------------------------------------

//------------------------------------------------------------------------------
/// sets track name
//------------------------------------------------------------------------------
void SDPTrack::Name(AnsiString strName)
{
   m_strName = strName;
}
//------------------------------------------------------------------------------


