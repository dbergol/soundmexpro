//------------------------------------------------------------------------------
/// \file SoundDataQueue.cpp
/// \author Berg
/// \brief Implementation of class SoundDataQueue
///
/// Project SoundMexPro
/// Module SoundDllPro
///
/// Implementation of class SoundDataQueue. Implements a FIFO for SoundData clas
/// instances
/// \sa SoundData.cpp
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
#include <windows.h>
#include <algorithm>
#include "SoundDataQueue.h"
#include "casioExceptions.h"

namespace Asio
{
    SoundDataQueue::SoundDataQueue(unsigned nChannels,
                                   unsigned nFrames,
                                   unsigned nBuffers)
        : m_nBufCapacity(nBuffers)
    {
        if (nBuffers == 0)
            throw EUnsupportedParamsError("SoundDataQueue::SoundDataQueue",
                                          "Cannot create queue with capacity"
                                          " for 0 buffers.");
        m_hDataAvailable = m_hSpaceAvailable = NULL;
        m_hDataAvailable = CreateEvent(NULL, FALSE, FALSE, NULL);
        if (m_hDataAvailable == NULL)
            throw EWin32Error("SoundDataQueue::SoundDataQueue",
                              "CreateEvent failed",
                              (int)GetLastError());
        m_hSpaceAvailable = CreateEvent(NULL, FALSE, TRUE, NULL);
        if (m_hSpaceAvailable == NULL)
            throw EWin32Error("SoundDataQueue::SoundDataQueue",
                              "CreateEvent failed",
                              (int)GetLastError());
        m_psdRead = m_psdWrite = m_rgsdBuffers =
            SoundData::CreateArray(nBuffers+1, nChannels, nFrames);
        m_bGetWritePointerCalled = false;
    }

    SoundDataQueue::~SoundDataQueue()
    {
        // secure clearing (try..catch around each action)
        try
        {
            SoundData::DestroyArray(m_rgsdBuffers);
        }
        catch (...)
        {
        }
        try
        {
            if (m_hDataAvailable)
            {
                CloseHandle(m_hDataAvailable);
                m_hDataAvailable = NULL;
            }
        }
        catch (...)
        {
        }
        try
        {
            if (m_hSpaceAvailable)
            {
                CloseHandle(m_hSpaceAvailable);
                m_hSpaceAvailable = NULL;
            }
        }
        catch (...)
        {
        }
        m_psdRead = m_psdWrite = m_rgsdBuffers = 0;
    }

    unsigned SoundDataQueue::NumFilledBuffers() const
    {
        // The extra "+ max_fill_count + 1" might look unnecessary, but is
        // required when the write pointer has already wrapped to the beginning
        // of the buffer, while the read pointer has not.
        return ( (unsigned int)(m_psdWrite + m_nBufCapacity + 1 - m_psdRead)
                 % (m_nBufCapacity + 1) );
    }

    unsigned SoundDataQueue::NumEmptyBuffers() const
    {
        return (unsigned)(m_nBufCapacity - NumFilledBuffers());
    }

    SoundData * SoundDataQueue::GetReadPtr() const
    {
        if (NumFilledBuffers() == 0)
        {
            bool bUnderrun = true;
            throw EXrunError("SoundDataQueue::GetReadPtr", bUnderrun);
        }
        return m_psdRead;
    }

    void SoundDataQueue::Pop()
    {
        if (NumFilledBuffers() == 0)
        {
            bool bUnderrun = true;
            throw EXrunError("SoundDataQueue::GetReadPtr", bUnderrun);
        }
        m_psdRead->Clear();
        m_psdRead =
            (unsigned int)(m_psdRead+1 - m_rgsdBuffers) % (m_nBufCapacity+1) + m_rgsdBuffers;
        SetEvent(m_hSpaceAvailable);
    }

    SoundData * SoundDataQueue::GetWritePtr()
    {
        if (NumEmptyBuffers() == 0)
        {
            throw EXrunError("SoundDataQueue::GetWritePtr", false); // overrun
        }
        m_bGetWritePointerCalled = true;
        return m_psdWrite;
    }
    void SoundDataQueue::Push()
    {
        if (m_bGetWritePointerCalled == false)
        {
            throw EUnexpectedError(__FILE__, __LINE__,
                                   "SoundDataQueue::Push",
                                   "SoundDataQueue::GetWritePtr has to be"
                                   " called prior to this method, otherwise,"
                                   " no data can be in the current buffer",
                                   this);
        }
        m_psdWrite =
            (unsigned int)(m_psdWrite+1 - m_rgsdBuffers) % (m_nBufCapacity+1) + m_rgsdBuffers;
        m_bGetWritePointerCalled = false; // reset
        SetEvent(m_hDataAvailable);
    }

    void SoundDataQueue::WaitForData()
    {
        while (NumFilledBuffers() == 0)
        {
            WaitForSingleObject(m_hDataAvailable, INFINITE);
        }
    }

    void SoundDataQueue::WaitForSpace()
    {
        while (NumEmptyBuffers() == 0)
        {
            WaitForSingleObject(m_hSpaceAvailable, INFINITE);
        }
    }

    HANDLE SoundDataQueue::GetDataEvent() const
    {
        return m_hDataAvailable;
    }
    HANDLE SoundDataQueue::GetSpaceEvent() const
    {
        return m_hSpaceAvailable;
    }
} // namespace Asio

// Local Variables:
// mode: c++
// c-file-style: "stroustrup"
// c-basic-offset: 4
// indent-tabs-mode: nil
// End:
