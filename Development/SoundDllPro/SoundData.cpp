//------------------------------------------------------------------------------
/// \file SoundData.cpp
/// \author Berg
/// \brief Implementation of class SoundData (Multi-channel sound data buffer)
///
/// Project SoundMexPro
/// Module SoundDllPro
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
#include "SoundData.h"
#include "casioExceptions.h"
using namespace Asio;

/// Change: error message changed, multiple 'using' statements avoided
SoundData * SoundData::CreateArray(size_t nNumBuffers,
                                   size_t nNumChannels,
                                   size_t nNumFrames)
{
    if (nNumBuffers == 0 || nNumFrames == 0)
        throw EUnsupportedParamsError("SoundData::CreateArray",
                                      "Number of buffers and number of"
                                      " frames must not be 0");
    SoundData * rgsdBuffers = new SoundData[nNumBuffers];

    // Initialize allocated sound data structures
    // Check assignment to m_vvfData
    std::vector<std::valarray<float> > vvfInit(nNumChannels,
                                       std::valarray<float>(0.0f, nNumFrames));
    size_t nBuf;
    for (nBuf = 0; nBuf < nNumBuffers; ++nBuf)
    {
        rgsdBuffers[nBuf].m_vvfData = vvfInit;
        rgsdBuffers[nBuf].m_bIsLast = false;
    }
    return rgsdBuffers;
}

void SoundData::DestroyArray(SoundData * rgsdArray)
{
    if (rgsdArray != 0)
    {
        delete [] rgsdArray;
        sm_nArraysDeleted++; // Only for testing, no synchronization required.
    }
}

void SoundData::Clear()
{
    unsigned nCh;
    for (nCh = 0; nCh < m_vvfData.size(); ++nCh)
    {
        m_vvfData[nCh] = 0.0f;
    }
    m_bIsLast = false;
}

/// Change: range check for nNumFrames added
void SoundData::Reinitialize(size_t nNumChannels, size_t nNumFrames)
{
    if (nNumFrames == 0)
        throw EUnsupportedParamsError("SoundData::Reinitialize",
                                      "Number of frames must not be 0");

    m_vvfData.clear();
    m_vvfData = std::vector<std::valarray<float> >(
            nNumChannels,
            std::valarray<float>(0.0f, nNumFrames));
    m_bIsLast = false;
}

void SoundData::CopyFrom(const SoundData & sdSource)
{
    if (sdSource.m_vvfData.size() != m_vvfData.size())
    {
        throw EUnexpectedError(__FILE__, __LINE__, "SoundData::CopyFrom",
                               "Channel count mismatch", this, 0);
    }
    size_t nChannel;
    for (nChannel = 0; nChannel < m_vvfData.size(); ++nChannel)
    {
        if (m_vvfData[nChannel].size() != sdSource.m_vvfData[nChannel].size())
        {
            throw EUnexpectedError(__FILE__, __LINE__, "SoundData::CopyFrom",
                                   "Frames count mismatch", this, nChannel);
        }
    }
    for (nChannel = 0; nChannel < m_vvfData.size(); ++nChannel)
    {
        m_vvfData[nChannel] = sdSource.m_vvfData[nChannel];
    }
    m_bIsLast = sdSource.m_bIsLast;
}

/// initialize static member
size_t SoundData::sm_nArraysDeleted = 0;
