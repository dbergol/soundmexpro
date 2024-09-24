// ------------------------------------------------------------------------------
/// \file SoundDataExchanger.cpp
/// \author Berg
/// \brief Implementation of class SoundDataExchanger
///
/// Project SoundMexPro
/// Module SoundDllPro
///
/// Implementation of class SoundDataExchanger. Uses multiple SoundDataQueue instances
/// to buffer input (recording) and output (playback) data of an ASIO device
/// \sa SoundDataQueue.cpp
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
// ------------------------------------------------------------------------------
#include "SoundDataExchanger.h"
#include <algorithm>
#include "casio.h"
#include "casioExceptions.h"

#pragma clang diagnostic push
#pragma clang diagnostic ignored "-Wheader-hygiene"
using namespace Asio;
#pragma clang diagnostic pop


void SoundDataExchanger::InitProcQueues(unsigned  nProcQueueBuffers,
                                        unsigned  nCaptureChannels,
                                        unsigned  nPlaybackChannels,
                                        unsigned  nFrames)
{
    m_psdqProcCapture = new SoundDataQueue(nCaptureChannels,
                                           nFrames,
                                           nProcQueueBuffers);
    ++sm_nObjects;
    m_psdqProcPlayback = new SoundDataQueue(nPlaybackChannels,
                                            nFrames,
                                            nProcQueueBuffers);
    ++sm_nObjects;
}


void SoundDataExchanger::InitDoneQueues(unsigned  nDoneQueueBuffers,
                                        unsigned  nCaptureChannels,
                                        unsigned  nPlaybackChannels,
                                        unsigned  nFrames)
{
    m_psdqDoneCapture = new SoundDataQueue(nCaptureChannels,
                                           nFrames,
                                           nDoneQueueBuffers);
    ++sm_nObjects;
    m_psdqDonePlayback = new SoundDataQueue(nPlaybackChannels,
                                            nFrames,
                                            nDoneQueueBuffers);
    ++sm_nObjects;
}

void SoundDataExchanger::InitTmpBuffers(unsigned nCaptureChannels,
                                        unsigned nPlaybackChannels,
                                        unsigned nFrames)
{
    m_sdCapture.Reinitialize(nCaptureChannels, nFrames);
    m_sdPlayback.Reinitialize(nPlaybackChannels, nFrames);
}

SoundDataExchanger::SoundDataExchanger(unsigned nProcQueueBuffers,
                                       unsigned nDoneQueueBuffers,
                                       unsigned nCaptureChannels,
                                       unsigned nPlaybackChannels,
                                       unsigned nFrames)
    : m_psdqProcCapture(0),
      m_psdqProcPlayback(0),
      m_psdqDoneCapture(0),
      m_psdqDonePlayback(0),
      m_bRealtimeProcessing(nProcQueueBuffers == 0U),
      m_pcaInst(CASIO_CLASS_NAME::Instance()),
      m_bProcXrun(false),
      m_bDoneXrun(false),
      m_nCurrentDoubleBufferIndex(0),
      m_bCalledGetPlaybackQueueBuffers(false),
      m_bCalledGetCaptureQueueBuffers(false),
      m_bCalledRetrieveCaptureData(false),
      m_bCalledPushCaptureQueues(false),
      m_bCalledHandlePlaybackDataPrivate(false),
      m_bCalledHandleCapturePrivate(false),
      m_nExceptionalDoneQueueClearCount(0)
{
    // if realtime processing, then the ProcQueue is only used for
    // synchronization.
    InitProcQueues((m_bRealtimeProcessing ? 1U : nProcQueueBuffers),
                   nCaptureChannels,
                   nPlaybackChannels,
                   nFrames);
    if (nDoneQueueBuffers > 0U)
    {
        InitDoneQueues(nDoneQueueBuffers,
                       nCaptureChannels,
                       nPlaybackChannels,
                       nFrames);
    }
    InitTmpBuffers(nCaptureChannels, nPlaybackChannels, nFrames);
}

SoundDataExchanger::~SoundDataExchanger()
{
    ClearQueues();
    if (m_psdqProcCapture)
    {
        delete m_psdqProcCapture;
        m_psdqProcCapture = 0;
        --sm_nObjects;
    }
    if (m_psdqProcPlayback)
    {
        delete m_psdqProcPlayback;
        m_psdqProcPlayback = 0;
        --sm_nObjects;
    }
    if (m_psdqDoneCapture)
    {
        delete m_psdqDoneCapture;
        m_psdqDoneCapture = 0;
        --sm_nObjects;
    }
    if (m_psdqDonePlayback)
    {
        delete m_psdqDonePlayback;
        m_psdqDonePlayback = 0;
        --sm_nObjects;
    }
    m_pcaInst = 0;
}

void SoundDataExchanger::StaticBufferSwitch(long nDoubleBufferIndex,
                                            long bProcessNow)
{
    // Ignore bProcessNow flag: Always assume false (worst case).
    (void) bProcessNow;

    CASIO_CLASS_NAME * pcaInst = CASIO_CLASS_NAME::Instance();
    if (pcaInst)
    {
        pcaInst->m_nDrvBufferSwitches++;
        SoundDataExchanger * psxInst = pcaInst->GetSoundDataExchanger();
        if (psxInst)
        {
            psxInst->OnBufferSwitch(nDoubleBufferIndex);
        }
    }
}

void SoundDataExchanger::OnBufferSwitch(long nDoubleBufferIndex)
{
    m_nCurrentDoubleBufferIndex = nDoubleBufferIndex;
    bool bStopping =
        m_pcaInst->SilenceOutputsIfStopping(nDoubleBufferIndex);
    if (!bStopping)
    {
        bool bRealtimeXrun =
            m_bRealtimeProcessing && m_pcaInst->CheckForRealtimeXrun();
        if (!bRealtimeXrun)
        {
            HandleCaptureData(nDoubleBufferIndex);
            if (m_bRealtimeProcessing == false)
            {
                HandlePlaybackData(nDoubleBufferIndex);
            }
        }
    }
}

void SoundDataExchanger::GetCaptureQueueBuffers(SoundData * rgpsdBuffers[2])
{
    m_bCalledGetCaptureQueueBuffers = true;
    sm_rgpsdGetCaptureQueueBuffersLastBuffers[0] =
        sm_rgpsdGetCaptureQueueBuffersLastBuffers[1] =
        rgpsdBuffers[0] =
        rgpsdBuffers[1] = 0;
    if(m_bProcXrun == false){
        sm_rgpsdGetCaptureQueueBuffersLastBuffers[0] = rgpsdBuffers[0] =
            m_psdqProcCapture->GetWritePtr();
    }
    if(m_psdqDoneCapture != 0 && m_bDoneXrun == false){
        sm_rgpsdGetCaptureQueueBuffersLastBuffers[1] = rgpsdBuffers[1] =
            m_psdqDoneCapture->GetWritePtr();
    }
}

void SoundDataExchanger::RetrieveCaptureData(long  nDoubleBufferIndex,
                                             SoundData * rgpsdBuffers[2])
{
    // set fields used for testing
    m_bCalledRetrieveCaptureData = true;
    sm_nRetrieveCaptureDataLastDoubleBufferIndex = nDoubleBufferIndex;
    sm_rgpsdRetrieveCaptureDataLastBuffers[0] = rgpsdBuffers[0];
    sm_rgpsdRetrieveCaptureDataLastBuffers[1] = rgpsdBuffers[1];

    // retrieve data from sound card and write it to both receivers
    m_pcaInst->ConvertInputsToFloat(nDoubleBufferIndex, m_sdCapture.m_vvfData);
    m_sdCapture.m_bIsLast = false;
    unsigned nIdx;
    for (nIdx = 0; nIdx < 2U; ++nIdx)
    {
        if (rgpsdBuffers[nIdx] != 0)
        {
            rgpsdBuffers[nIdx]->CopyFrom(m_sdCapture);
        }
    }
}


void SoundDataExchanger::PushCaptureQueues()
{
    m_bCalledPushCaptureQueues = true;
    if(m_bProcXrun == false){
        m_psdqProcCapture->Push();
    }
   if (!m_bCaptureDoneProcessed) {
      if (m_psdqDoneCapture != 0 && m_bDoneXrun == false) {
         m_psdqDoneCapture->Push();
      }
   }
}

bool SoundDataExchanger::CheckCaptureXrun(SoundDataQueue * psdqCapture)
{
    if (psdqCapture != 0 && psdqCapture->NumEmptyBuffers() == 0U)
    {
        // Overrun detected!
        bool bIsProc = (psdqCapture == m_psdqProcCapture);
        m_pcaInst->SignalXrun(bIsProc ? XR_PROC : XR_DONE);
        return true;
    }
    return false;
}

void SoundDataExchanger::HandleCaptureData(long nDoubleBufferIndex)
{
    m_bCalledHandleCapturePrivate = true;
    m_bProcXrun = CheckCaptureXrun(m_psdqProcCapture);
    m_bDoneXrun = CheckCaptureXrun(m_psdqDoneCapture);
    SoundData * rgpsdBuffers[2] = {0,0};
    GetCaptureQueueBuffers(rgpsdBuffers);
    RetrieveCaptureData(nDoubleBufferIndex, rgpsdBuffers);
    PushCaptureQueues();
}



void SoundDataExchanger::GetPlaybackQueueBuffers(SoundData * rgpsdBuffers[2])
{
    sm_rgpsdGetPlaybackQueueBuffersLastBuffers[0] =
        sm_rgpsdGetPlaybackQueueBuffersLastBuffers[1] =
        rgpsdBuffers[0] =
        rgpsdBuffers[1] = 0;
    if(m_bProcXrun == false){
        sm_rgpsdGetPlaybackQueueBuffersLastBuffers[0] =
            rgpsdBuffers[0] =
            m_psdqProcPlayback->GetReadPtr();
    }
    if(m_psdqDonePlayback != 0 && m_bDoneXrun == false){
        sm_rgpsdGetPlaybackQueueBuffersLastBuffers[1] =
            rgpsdBuffers[1] =
            m_psdqDonePlayback->GetWritePtr();
    }
    m_bCalledGetPlaybackQueueBuffers = true;
}

void SoundDataExchanger::CopyPlaybackData(long  nDoubleBufferIndex,
                                          SoundData * rgpsdBuffers[2])
{
    // set fields used for testing this method
    sm_nCopyPlaybackDataLastDoubleBufferIndex = nDoubleBufferIndex;
    sm_rgpsdCopyPlaybackDataLastBuffers[0] = rgpsdBuffers[0];
    sm_rgpsdCopyPlaybackDataLastBuffers[1] = rgpsdBuffers[1];


    if (rgpsdBuffers[0] != 0)
    {
        m_sdPlayback.CopyFrom(*rgpsdBuffers[0]);
    }
    else
    {   // there was an xrun, data is missing
        m_sdPlayback.Clear();
    }
    m_pcaInst->OnBufferPlay(m_sdPlayback);
    if(rgpsdBuffers[1] != 0)
    {
        rgpsdBuffers[1]->CopyFrom(m_sdPlayback);
    }
    m_pcaInst->ConvertFloatToOutputs(nDoubleBufferIndex,
                                     m_sdPlayback.m_vvfData);
}

void SoundDataExchanger::ReleasePlaybackQueues()
{
    if(m_bProcXrun == false){
        m_psdqProcPlayback->Pop();
    }
    if(m_psdqDonePlayback != 0 && m_bDoneXrun == false){
        m_psdqDonePlayback->Push();
    }
}

bool SoundDataExchanger::CheckPlaybackLastFlagAndStop()
{
    if (m_sdPlayback.m_bIsLast)
    {
        if (m_pcaInst)
        {
            m_pcaInst->Stop();
        }
        return true;
    }
    return false;
}

void SoundDataExchanger::HandlePlaybackData(long nDoubleBufferIndex)
{
    m_bCalledHandlePlaybackDataPrivate = true;
    sm_nHandlePlaybackDataLastDoubleBufferIndex = nDoubleBufferIndex;

    SoundData * rgpsdBuffers[2] = {0,0};
    GetPlaybackQueueBuffers(rgpsdBuffers);
    CopyPlaybackData(nDoubleBufferIndex, rgpsdBuffers);
    ReleasePlaybackQueues();
    CheckPlaybackLastFlagAndStop();
}

void SoundDataExchanger::HandlePlaybackData()
{
    HandlePlaybackData(m_nCurrentDoubleBufferIndex);
}

bool SoundDataExchanger::IsRealTime()
{
    return m_bRealtimeProcessing;
}

bool SoundDataExchanger::HasDoneQueue()
{
    return (m_psdqDonePlayback != 0);
}

void SoundDataExchanger::GetProcBuffers(SoundData ** ppsdCapture,
                                            SoundData ** ppsdPlayback)
{
    m_psdqProcCapture->WaitForData();
    *ppsdCapture = m_psdqProcCapture->GetReadPtr();
    m_psdqProcPlayback->WaitForSpace();
    *ppsdPlayback = m_psdqProcPlayback->GetWritePtr();
}

void SoundDataExchanger::CaptureDataProcToDone()
{
   SoundData * rgpsdBuffers[2] = {0, 0};
   GetCaptureQueueBuffers(rgpsdBuffers);
   SoundData * psdCapture = 0;
   SoundData * psdPlayback = 0;
   GetProcBuffers(&psdCapture, &psdPlayback);
   rgpsdBuffers[1]->CopyFrom(*psdCapture);
}


void SoundDataExchanger::ProcPut() 
{
   if (m_bCaptureDoneProcessed) {
      CaptureDataProcToDone();
      if (m_psdqDoneCapture != 0 && m_bDoneXrun == false) {
         m_psdqDoneCapture->Push();
      }
   }
   m_psdqProcCapture->Pop();
   m_psdqProcPlayback->Push();
}

unsigned SoundDataExchanger::ProcPlaybackNumEmptyBuffers() const
{
    return m_psdqProcPlayback->NumEmptyBuffers();
}

unsigned SoundDataExchanger::ProcCaptureNumFilledBuffers() const
{
    return m_psdqProcCapture->NumFilledBuffers();
}
unsigned SoundDataExchanger::ProcNumClientBuffers() const
{
    return std::min(ProcPlaybackNumEmptyBuffers(),
                    ProcCaptureNumFilledBuffers());
}

unsigned SoundDataExchanger::DoneNumFilledBuffers() const
{
    if (m_psdqDonePlayback && m_psdqDoneCapture)
        return std::min(m_psdqDonePlayback->NumFilledBuffers(),
                        m_psdqDoneCapture->NumFilledBuffers());
    return 0;
}

void SoundDataExchanger::Prefill(const SoundData & sdPlayback)
{
    m_psdqProcPlayback->GetWritePtr()->CopyFrom(sdPlayback);
    m_psdqProcPlayback->Push();
}

void SoundDataExchanger::GetDoneBuffers(
        SoundData ** ppsdCapture, SoundData ** ppsdPlayback)
{
    m_psdqDonePlayback->WaitForData();
    m_psdqDoneCapture->WaitForData();
    *ppsdCapture = m_psdqDoneCapture->GetReadPtr();
    *ppsdPlayback = m_psdqDonePlayback->GetReadPtr();
}

void SoundDataExchanger::PopDoneBuffers()
{
    m_psdqDoneCapture->Pop();
    m_psdqDonePlayback->Pop();
}

HANDLE SoundDataExchanger::GetDoneCaptureDataEvent() const
{
    return m_psdqDoneCapture->GetDataEvent();
}

HANDLE SoundDataExchanger::GetDonePlaybackDataEvent() const
{
    return m_psdqDonePlayback->GetDataEvent();
}

HANDLE SoundDataExchanger::GetProcCaptureDataEvent() const
{
    return m_psdqProcCapture->GetDataEvent();
}

HANDLE SoundDataExchanger::GetProcPlaybackSpaceEvent() const
{
    return m_psdqProcPlayback->GetSpaceEvent();
}

void SoundDataExchanger::DonePop()
{
    m_psdqDoneCapture->Pop();
    m_psdqDonePlayback->Pop();
}

void SoundDataExchanger::ClearQueues()
{
    // Discard any contents of the processing queues
    while(m_psdqProcCapture->NumFilledBuffers() > 0)
    {
        m_psdqProcCapture->Pop();
    }
    while(m_psdqProcPlayback->NumFilledBuffers() > 0)
    {
        m_psdqProcPlayback->Pop();
    }

    // The contents of the "Done" queues is consumed by the "Done" thread.
    // If the DoneLoop is active, we can wait for the exhaustion here.
    while(m_pcaInst->IsDoneLoopActive() && (
             (m_psdqDoneCapture && m_psdqDoneCapture->NumFilledBuffers() > 0)
          || (m_psdqDonePlayback && m_psdqDonePlayback->NumFilledBuffers() > 0)
          ))
    {
        Sleep(1); // millisecond. The done thread should empty these queues
    }

    // If the DoneLoop has not been active, then we discard the contents of the
    // done queues here.
    while (m_psdqDoneCapture && m_psdqDoneCapture->NumFilledBuffers() > 0)
    {
        m_psdqDoneCapture->Pop();
        ++m_nExceptionalDoneQueueClearCount;
    }
    while(m_psdqDonePlayback && m_psdqDonePlayback->NumFilledBuffers() > 0)
    {
        m_psdqDonePlayback->Pop();
        ++m_nExceptionalDoneQueueClearCount;
    }
}

int SoundDataExchanger::sm_nObjects = 0;
long SoundDataExchanger::sm_nRetrieveCaptureDataLastDoubleBufferIndex = -1;
SoundData * SoundDataExchanger::sm_rgpsdRetrieveCaptureDataLastBuffers[2] =
{
  0,0
};
SoundData * SoundDataExchanger::sm_rgpsdGetCaptureQueueBuffersLastBuffers[2] =
{
  0,0
};
long SoundDataExchanger::sm_nHandlePlaybackDataLastDoubleBufferIndex = -1;
long SoundDataExchanger::sm_nCopyPlaybackDataLastDoubleBufferIndex = -1;
SoundData * SoundDataExchanger::sm_rgpsdGetPlaybackQueueBuffersLastBuffers[2] =
{
  0,0
};
SoundData * SoundDataExchanger::sm_rgpsdCopyPlaybackDataLastBuffers[2] =
{
  0,0
};

