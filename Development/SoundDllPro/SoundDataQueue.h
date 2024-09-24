//------------------------------------------------------------------------------
/// \file SoundDataQueue.h
/// \author Berg
/// \brief Interface of class SoundDataQueue
///
/// Project SoundMexPro
/// Module SoundDllPro
///
/// Interface of class SoundDataQueue. Implements a FIFO for SoundData clas
/// instances
/// \sa SoundData.h
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
#ifndef SOUND_DATA_QUEUE_H
#define SOUND_DATA_QUEUE_H

#include <windows.h>
#include "SoundData.h"

class UNIT_TEST_CLASS;

namespace Asio
{
    /// A SoundDataQueue is a FIFO of sound data buffers with limited maximum
    /// capacity.
    /// It can be used to transport sound data from one thread of execution to
    /// another, and to introduce a delay.
    ///
    /// variable prefix: sdq for (S)ound(D)ata(Q)ueue
    class SoundDataQueue {
        friend class UNIT_TEST_CLASS;

        /// The maximum number of SoundData structs that can be stored in this Queue.
        const size_t m_nBufCapacity;

        /// The memory allocated to store the data.  m_nBufCapacity + 1
        /// locations are allocated: At least one location is always unused,
        /// because we have m_nBufCapacity + 1 possible fillcounts
        /// [0:m_nBufCapacity] that we need to distinguish.
        SoundData * m_rgsdBuffers;

        /// points to location where to write next
        SoundData * m_psdWrite;

        ///  points to location where to read next
        SoundData * m_psdRead;

        /// This Win32 Event is set when data has been written to the
        /// queue using \sa Push
        HANDLE m_hDataAvailable;

        /// This Win32 Event is set when data has been read from the
        /// queue using \sa Pop 
        HANDLE m_hSpaceAvailable;

        /// Flag to detect erroneous usage of this instance
        bool m_bGetWritePointerCalled;
    public:
        /// Return the number of sound data buffers currently in the queue.
        unsigned NumFilledBuffers() const;

        /// Return the number of sound data buffers that are currently empty
        unsigned NumEmptyBuffers() const;

        /// Return a pointer to the next sound buffer that can be read from the
        /// queue.
        /// \exception EXrunError if NumFilledBuffers() == 0 
        virtual SoundData * GetReadPtr() const;

        /// Erase the sound data in the current read buffer and advance the
        /// read pointer. Wake up the thread that sleeps in WaitForSpace().
        /// \exception EXrunError if NumFilledBuffers() == 0 
        virtual void Pop();

        /// Return a pointer to the next sound buffer that can be written to in
        /// the queue.
        /// \exception EXrunError if NumEmptyBuffers() == 0
        virtual SoundData * GetWritePtr();

        /// Advance the write pointer. Wake up the thread that sleeps is
        /// WaitForData().
        /// \exception EUnexpectedError If GetWritePtr() has not been called since the last push
        virtual void Push();

        /// Wait until the queue contains at least one filled buffer.
        virtual void WaitForData();

        /// Wait until the queue has space for at least one buffer.
        virtual void WaitForSpace();

        /// Returns the Win32 Event that signals data availability.
        HANDLE GetDataEvent() const;

        /// Returns the Win32 Event that signals space availability.
        HANDLE GetSpaceEvent() const;
        
        /// Create a queue of SoundData structs.
        /// \param[in] nChannels
        ///  The number of audio channels in each SoundData struct
        /// \param[in] nFrames
        ///  The number of samples stored for each channel in a single instance
        ///  of the SoundData struct.
        /// \param[in] nBuffers
        ///  The maximum capacity of the queue, i.e. how many SoundData
        ///  instances can be stored in the queue.
        SoundDataQueue(unsigned nChannels,
                       unsigned nFrames,
                       unsigned nBuffers);

        /// Deallocate the Queue
        virtual ~SoundDataQueue();

    }; // class SoundDataQueue

} // namespace Asio

#endif // #ifndef SOUND_DATA_QUEUE_H

// next comment block describes code layout for emacs.

// Local Variables:
// mode: c++
// c-file-style: "stroustrup"
// c-basic-offset: 4
// indent-tabs-mode: nil
// End:
