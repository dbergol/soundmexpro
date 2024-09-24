//------------------------------------------------------------------------------
/// \file SoundSoftwareBuffer.cpp
///
/// \author Berg
/// \brief Implementation of class TSoundSoftwareBuffer
///
/// Project SoundMexPro
/// Module  SoundDllPro.dll
/// Implementation of base class class TSoundSoftwareBuffer
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
#include <stdio.h>
#pragma hdrstop
#include "SoundSoftwareBuffer.h"
#include "SoundDllPro_SoundClassMMDevice.h"
#pragma package(smart_init)
//------------------------------------------------------------------------------
/// constructor. Sets up internal buffer and event
/// \param[in] pHtSound 'owning CHtSound instance
/// \param[in] nNumBuffers number of software buffers. 'Internal' size m_nNumBuffers
/// is set to nNumBuffers+1 because one buffer is always unused due to handling
/// of ReadIndex/WriteIndex
/// \param[in] nChannels number of channels
/// \param[in] nBufsize size of each software buffer
/// \exception EMwmException if oassed argument(s) invalid
//------------------------------------------------------------------------------
__fastcall TSoundSoftwareBuffer::TSoundSoftwareBuffer(SoundClassMMDevice*   pHtSound,
                                                      int         nNumBuffers,
                                                      int         nChannels,
                                                      int         nBufsize)
   :  TThread(true),
      m_pHtSound(pHtSound),
      m_nNumBuffers((unsigned int)nNumBuffers+1),
      m_nChannels((unsigned int)nChannels),
      m_nBufsize((unsigned int)nBufsize),
      m_nReadIndex(nNumBuffers),
      m_nWriteIndex(0),
      m_bProcessingError(false),
      m_hWriteHandle(0)
{
   if (!pHtSound || nNumBuffers <= 0 || nChannels <= 0 || nBufsize <=0)
      throw Exception("invalid value(s) passed to constructor");
   // set up vector of vector of float valarray
   m_vvvfSoftwareBuffer.resize(m_nNumBuffers);
   unsigned int n;
   unsigned int m;
   for (n = 0; n < m_nNumBuffers; n++)
      {
      m_vvvfSoftwareBuffer[n].resize(m_nChannels);
      for (m = 0; m < m_nChannels; m++)
         {
         m_vvvfSoftwareBuffer[n][m].resize(m_nBufsize);
         m_vvvfSoftwareBuffer[n][m] = 0.0f;
         }
      }
   // create event with manual reset and initial state 'signaled'
   m_hWriteHandle = CreateEvent(0, true, true, NULL);
   if (!m_hWriteHandle)
      throw Exception("error creating write handle");
}
//------------------------------------------------------------------------------

//------------------------------------------------------------------------------
/// destructor. Cleans up event
//------------------------------------------------------------------------------
__fastcall TSoundSoftwareBuffer::~TSoundSoftwareBuffer()
{
   if (m_hWriteHandle)
      {
      CloseHandle(m_hWriteHandle);
      m_hWriteHandle = 0;
      }
}
//------------------------------------------------------------------------------

//------------------------------------------------------------------------------
/// Thread function. Waits for event m_hWriteHandle to fill buffer(s).
//------------------------------------------------------------------------------
void __fastcall TSoundSoftwareBuffer::Execute()
{
   long nNextIndex;
   unsigned int n;
   DWORD dw;
   try
      {
      while (!Terminated)
         {
         // wait for write handle to be signaled with timeout (to check
         // wor Terminated flag in while loop)
         dw = WaitForMultipleObjects(1, &m_hWriteHandle, false, 100);
         // if m_hWriteHandle is set then one buffer (at least) should be empty now
         if (dw == WAIT_OBJECT_0)
            {
            ResetEvent(m_hWriteHandle);
            // check again, if a buffer is available (if indices are identical, then
            // buffer is full!!)
            if (m_nWriteIndex != m_nReadIndex)
               {
               for (n = 0; n < m_nChannels; n++)
                  m_vvvfSoftwareBuffer[(unsigned int)m_nWriteIndex][n] = 0.0f;
               // call processing of owning CHtSound class
               m_pHtSound->ProcessInternal(m_vvvfSoftwareBuffer[(unsigned int)m_nWriteIndex]);
               // increment ...
               nNextIndex = m_nWriteIndex + 1;
               // and wrap round write index
               if (nNextIndex >= (long)m_nNumBuffers)
                  nNextIndex = 0;
               // set it thread safe
               InterlockedExchange(&m_nWriteIndex, nNextIndex);
               // if afterwards there is still a buffer available then set signal
               // again to wake up again (do NOT a loop for all buffers to give other thread
               // a chance!!)
               if (m_nWriteIndex != m_nReadIndex)
                  SetEvent(m_hWriteHandle);
               }
            }
         }
      }
   catch (Exception &)
      {
      m_bProcessingError = true;
      }
   // Finally set Terminated to true to indicate thread end in any case
   // (is not done automatically).
   Terminate();
}
//------------------------------------------------------------------------------

//------------------------------------------------------------------------------
/// Reads a buffer from internal softwarebuffers. If one is available the
/// passed buffer is filled with data and m_hWriteHandle is set to tell thread
/// that a buffer was read (i.e. is available for writing again)
/// \param[in,out] vvfBuffer buffer tow rite audio data to
/// \retval true if data were available
/// \retval false if not (Xrun)
//------------------------------------------------------------------------------
bool TSoundSoftwareBuffer::GetBuffer(vvf& vvfBuffer)
{
   if (vvfBuffer.size() != m_nChannels)
      throw Exception("channel count mismatch in " + UnicodeString(__FUNC__));
   unsigned int n;
   for (n = 0; n < m_nChannels; n++)
      {
      if (vvfBuffer[n].size() != m_nBufsize)
         throw Exception("bufsize mismatch in " + UnicodeString(__FUNC__));
      vvfBuffer[n] = 0.0f;
      }
   if (m_bProcessingError)
      throw Exception("processing error in " + UnicodeString(__FUNC__));
   // check if we can read the next buffer
   long nNextIndex = m_nReadIndex+1;
   if (nNextIndex >= (long)m_nNumBuffers)
      nNextIndex = 0;
   if (nNextIndex == m_nWriteIndex)
      return false;
   InterlockedExchange(&m_nReadIndex, nNextIndex);
   vvfBuffer = m_vvvfSoftwareBuffer[(unsigned int)m_nReadIndex];
   // set signal for thread to wake up
   SetEvent(m_hWriteHandle);
   return true;
}
//------------------------------------------------------------------------------
