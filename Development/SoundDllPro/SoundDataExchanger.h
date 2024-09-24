//------------------------------------------------------------------------------
/// \file SoundDataExchanger.h
/// \author Berg
/// \brief Interface of class SoundDataExchanger
///
/// Project SoundMexPro
/// Module SoundDllPro
///
/// Interface of class SoundDataExchanger. Uses multiple SoundDataQueue instances
/// to buffer input (recording) and output (playback) data of an ASIO device
/// \sa SoundDataQueue.h
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
#ifndef SoundDataExchangerH
#define SoundDataExchangerH

#include "SoundDataQueue.h"

#ifndef CASIO_CLASS_NAME

/// The SoundDataExchanger class transports sound data from the Asio driver to
///   the CAsio class and vice versa. In this setup, the CAsio class is still
///   responsible for loading and initialising the Asio driver.
///
///   The SoundDataExchanger introduces the possibility to queue incoming and
///   outgoing sound data.
///
///   To test the queueing functionality, it is not necessary to perform these tests
///   with real Asio driver loaded. Therefore, a mock implementation of CAsio will
///   be used for the tests. For this purpose, this macro is used. It is altered
///   by the SoundDataExchanger test.

#define CASIO_CLASS_NAME CAsio
#endif

namespace Asio
{              
    class CASIO_CLASS_NAME;

    /// The SoundDataExchanger class implements the sound data buffering,
    /// since native Asio is otherwise an unbuffered realtime sound I/O API.
    /// Instances are created and deleted by CAsio.
    /// Possible class prefixes are sde or sdx.
    class SoundDataExchanger
    {
        #ifdef UNIT_TEST_CLASS
        friend class UNIT_TEST_CLASS;
        #endif

    public:
        /// Initialize sound data exchanger instance
        /// \param[in] nProcQueueBuffers
        ///  Number of buffers used to decouple processing thread from sound
        ///  card. 0 buffers means realtime processing.
        /// \param[in] nDoneQueueBuffers
        ///  Number of buffers used to decouple visualisation/disk recording
        ///  from sound card
        /// \param[in] nCaptureChannels
        ///  The number of prepared sound channels for capture
        /// \param[in] nPlaybackChannels
        ///  The number of prepared sound channels for playback
        /// \param[in] nFrames
        ///  The number of sound samples per channel and buffer */
        SoundDataExchanger(unsigned nProcQueueBuffers,
                           unsigned nDoneQueueBuffers,
                           unsigned nCaptureChannels,
                           unsigned nPlaybackChannels,
                           unsigned nFrames);

        /// Deallocates the sound data queues.
        virtual ~SoundDataExchanger();

        // true, if processed capture data to be passed to done-queuecallback
        // added to allow processed capture data passed to done-queue
        // rather than raw data from driver
        bool m_bCaptureDoneProcessed;

        /// Get pointers to the current client sound data buffers in the
        /// processing queues: The next readable buffer in the capture
        /// queue, and the next writable buffer in the playback queue.
        /// Waits for data/space if necessary.
        /// \param[out] ppsdCapture
        ///  Assign address of the capture queue's next readable
        ///  Asio::SoundData to this pointer.
        /// \param[out] ppsdPlayback
        ///  Assign address of the playback queue's next writable
        ///  Asio::SoundData to this pointer.
        void GetProcBuffers(SoundData ** ppsdCapture, SoundData ** ppsdPlayback);

        /// Mark in the processing queues that the processing is finished:
        /// Old capture data can be discarded, new playback data is now valid.
        void ProcPut();

        /// Returns the number of empty buffers in the processing playback queue
        unsigned ProcPlaybackNumEmptyBuffers() const;

        /// Returns the number of filled buffers in the processing capture queue
        unsigned ProcCaptureNumFilledBuffers() const;

        /// Returns the minimum of #ProcPlaybackNumEmptyBuffers()
        /// and #ProcCaptureNumFilledBuffers(). This is the number of
        /// buffers available for processing right now, without waiting for
        /// further soundcard interupts.
        unsigned ProcNumClientBuffers() const;

        ///  Returns the number of filled buffers in the "done" queues.
        /// \retval The minimum of the number of filled buffers in both
        /// "done" queues.
        unsigned DoneNumFilledBuffers() const;

        ///  Prefills queues by copying and pushing data from sdPlayback
        /// \param[in] sdPlayback reference to SoundData to copy
        void Prefill(const SoundData & sdPlayback);

        ///  Get pointers to the current sound data buffers that are
        /// readable next in the "done" queue. Waits for data if there is no
        /// data available in both done queues yet.
        /// \param[out] ppsdCapture
        ///  Assign address of the capture queue's current Asio::SoundData
        ///  to this pointer.
        /// \param[out] ppsdPlayback
        ///  Assign address of the playback queue's current Asio::SoundData
        ///  to this pointer.
        void GetDoneBuffers(SoundData ** ppsdCapture, SoundData ** ppsdPlayback);

        /// Pops the buffers used for GetDoneBuffers.
        void PopDoneBuffers();

        /// Returns the Event that signals that the "done" capture queue has
        /// data available.
        HANDLE GetDoneCaptureDataEvent() const;

        /// Returns the Event that signals that the "done" playback queue has
        /// data available.
        HANDLE GetDonePlaybackDataEvent() const;

        /// Returns the Event that signals that the processing capture queue has
        /// data available.
        HANDLE GetProcCaptureDataEvent() const;

        ///  Returns the Event that signals that the processing playback queue
        /// has data available.
        HANDLE GetProcPlaybackSpaceEvent() const;

        /// This public method without argument delegates to the private
        /// method #HandlePlaybackData(long nDoubleBufferIndex).
        /// The double buffer index used is the one stored in
        /// #m_nCurrentDoubleBufferIndex.
        /// This public method is used by CAsio when sound I/O is processed
        /// in realtime mode (i.e. unbuffered).
        /// In this mode, it is called after output data has been generated
        /// by the CAsio::Process method, to transfer the newly created
        /// output data to the sound card immediately.
        void HandlePlaybackData();

        /// Returns true if realtime processing is selected.
        bool IsRealTime();

        /// Returns true if the SoundDataExchanger has non-zero "Done" queues.
        bool HasDoneQueue();

        /// Mark in the processing queues that the "done" thread has finished:
        /// Old data is discarded.
        void DonePop();

        /// Waits for the done thread to finish processing and then discards
        /// remaining entries from the processing queues.
        void ClearQueues();

        /// Audio processing callback, called by the ASIO driver.
        /// This static method will relay the bufferSwitch call to the
        /// instance method #bufferSwitch.  It locates the current CAsio
        /// instance using the static method CAsio::instance().
        /// \param[in] nDoubleBufferIndex
        ///     0 or 1.  Indicates which of (the 2 buffers allocated for each
        ///     channel) to use for reading/writing sound data.
        /// \param[in] bProcessNow
        ///     Indicates wether it is safe to process the sound data within
        ///     this thread, or if the application should return as soon as
        ///     possible and process the sound data in a different thread.
        ///     This class always assumes the worst case bProcessNow == false,
        ///     and always processes the signal in a different thread,
        ///     regardless of the value of this flag. This also enables XRun
        ///     detection.
        static void StaticBufferSwitch(long nDoubleBufferIndex,
                                       long bProcessNow);
    private:
        /// Number of objects allocated, used for testing.
        static int sm_nObjects;

        /// storage used only for testing data flow,
        /// method RetrieveCaptureData stores its 1st parameter here.
        static long sm_nRetrieveCaptureDataLastDoubleBufferIndex;

        /// storage used only for testing data flow,
        /// method RetrieveCaptureData stores its 2nd Prameter here.
        static SoundData * sm_rgpsdRetrieveCaptureDataLastBuffers[2];

        /// storage used only for testing data flow,
        /// method GetCaptureQueueBuffers stores its result here for unit tests.
        static SoundData * sm_rgpsdGetCaptureQueueBuffersLastBuffers[2];

        /// storage used only for testing data flow,
        /// method HandlePlaybackData stores its parameter here.
        static long sm_nHandlePlaybackDataLastDoubleBufferIndex;

        /// storage used only for testing data flow,
        /// method CopyPlaybackData stores its 1st parameter here.
        static long sm_nCopyPlaybackDataLastDoubleBufferIndex;

        /// storage used only for testing data flow,
        /// method CopyPlaybackData stores its 2nd parameter here.
        static SoundData * sm_rgpsdCopyPlaybackDataLastBuffers[2];

        /// storage used only for testing data flow,
        /// method GetPlaybackQueueBuffers stores its result here for unit tests
        static SoundData * sm_rgpsdGetPlaybackQueueBuffersLastBuffers[2];

        /// The sound data queue for transmitting the captured sound data
        /// from the sound card to the processing.
        SoundDataQueue * m_psdqProcCapture;

        /// The sound data queue for transmitting the generated output sound
        /// data from the processing to the sound card.
        SoundDataQueue * m_psdqProcPlayback;

        /// The sound data queue that transmits the current capture sound data
        /// from the sound card to the visualization/disk recording thread.
        SoundDataQueue * m_psdqDoneCapture;

        /// The sound data queue that transmits the current playback data to
        /// the visualization/disk recording thread.  Sound buffers enter this
        /// queue when they are actually delivered to the Asio driver, i.e.
        /// after they have tunneled through the m_sdqProcPlayback queue.
        SoundDataQueue * m_psdqDonePlayback;

        /// true if the desired processing queues size is 0
        bool m_bRealtimeProcessing;

        /// Address of the CAsio instance
        CASIO_CLASS_NAME * m_pcaInst;

        /// XRun in processing detected
        bool m_bProcXrun;

        /// XRun in Visualization detected
        bool m_bDoneXrun;

        /// Current capture data is stored here during bufferSwitch handling
        SoundData m_sdCapture;

        /// Current playback data is stored here during bufferSwitch handling
        /// This is temporary memory used during the CopyPlaybackData method,
        /// but it is also used to store the state of the latest
        /// SoundData::m_bIsLast flag for the CheckPlaybackLastFlagAndStop
        /// method.
        SoundData m_sdPlayback;

        /// Current index within double buffer set by \sa OnBufferSwitch
        long m_nCurrentDoubleBufferIndex;

        /// Flag for testing, set to true when GetPlaybackQueueBuffers executes
        bool m_bCalledGetPlaybackQueueBuffers;

        /// Flag for testing, set to true when GetCaptureQueueBuffers executes
        bool m_bCalledGetCaptureQueueBuffers;

        /// Flag for testing, set to true when RetrieveCaptureData executes
        bool m_bCalledRetrieveCaptureData;

        /// Flag for testing, set to true when PushCaptureQueues executes
        bool m_bCalledPushCaptureQueues;

        /// Flag for testing, set to true when the private HandlePlaybackData
        /// method executes
        bool m_bCalledHandlePlaybackDataPrivate;

        /// Flag for testing, set to true when the private HandleCaptureData
        /// method executes
        bool m_bCalledHandleCapturePrivate;

        /// Initialize the sound data queues for the processing thread.
        /// param[in] nProcQueueBuffers
        ///  The capacity of the sound data queues, in buffers.
        ///  nProcQueueBuffers > 0
        /// param[in] nCaptureChannels
        ///  The number of audio channels prepared for capture.
        /// param[in] nPlaybackChannels
        ///  The number of audio channels prepared for playback.
        /// param[in] nFrames
        ///  The number of sound samples per channels and buffer.
        void InitProcQueues(unsigned  nProcQueueBuffers,
                            unsigned  nCaptureChannels,
                            unsigned  nPlaybackChannels,
                            unsigned  nFrames);
        
        /// Initialize the sound data queues for the visualization /
        /// disk recording thread.
        /// param[in] nDoneQueueBuffers
        ///  The capacity of the sound data queues, in buffers.
        ///  nProcQueueBuffers > 0
        /// param[in] nCaptureChannels
        ///  The number of audio channels prepared for capture.
        /// param[in] nPlaybackChannels
        ///  The number of audio channels prepared for playback.
        /// param[in] nFrames
        ///  The number of sound samples per channels and buffer.
        void InitDoneQueues(unsigned  nDoneQueueBuffers,
                            unsigned  nCaptureChannels,
                            unsigned  nPlaybackChannels,
                            unsigned  nFrames);
        
        /// Initialize the temporary buffers #m_sdCapture and #m_sdPlayback
        /// param[in] nCaptureChannels
        ///  The number of audio channels prepared for capture.
        /// param[in] nPlaybackChannels
        ///  The number of audio channels prepared for playback.
        /// param[in] nFrames
        ///  The number of sound samples per channels and buffer.
        void InitTmpBuffers(unsigned nCaptureChannels, 
                            unsigned nPlaybackChannels,
                            unsigned nFrames);

        /// Instance method invoked by static method StaticBufferSwitch
        /// whenever the sound card has or needs new data.
        /// Checks if the device is currently stopping or if there is
        /// an Xrun in realtime processing mode. If not, handles
        /// the captured sound data, and, in non-realtime processing mode,
        /// also the playback data.
        /// \param[in] nDoubleBufferIndex
        ///   asio driver buffer index
        void OnBufferSwitch(long nDoubleBufferIndex);

        /// Reads sound data from the driver buffer,
        /// converts them to float and writes the sound data to the
        /// capture queues
        /// \param nDoubleBufferIndex
        ///   The Asio driver's buffer index.
        void HandleCaptureData(long nDoubleBufferIndex);

        /// Checks for an Overrun of the given capture queue,
        /// signals the xrun if there is one,
        /// and returns the xrun condition
        /// \param psdqCapture
        ///   Pointer to the queue to check. If an xrun has to be signalled,
        ///   then the type of the queue (Proc or Done) is determined by
        ///   comparing the address with #m_psdqProcCapture.
        /// \return
        ///   A flag indicating wether an Xrun was detected or not.
        bool CheckCaptureXrun(SoundDataQueue * psdqCapture);

        /// Retrieves addresses from next write buffers of the capture queues
        /// and stores them in the array given as parameter.
        /// \param rgpsdBuffers Addresses of next write buffers of capture
        ///        queues are stored here.
        ///        The address of the next buffer in the processing capture
        ///        queue is stored at index 0. The address of the next buffer
        ///        in the processing capture queue is stored at index 1.
        ///        If one queue is not present or is currently in overrun
        ///        condition, then NULL is stored in its place.
        void GetCaptureQueueBuffers(SoundData * rgpsdBuffers[2]);

        /// Fills given buffers with sound data from the sound card.
        /// \param nDoubleBufferIndex
        ///    The current asio driver buffer index
        /// \param rgpsdBuffers
        ///    Addresses of the next write buffers in the capture queues.
        ///    The addresses for queues that are currently in xrun condition
        ///    may be replaced with NULL.
        void RetrieveCaptureData(long  nDoubleBufferIndex,
                                 SoundData * rgpsdBuffers[2]);

        /// Copies current proc capture data to current done capture data
        void CaptureDataProcToDone();

        /// Call Push on each capture queue that exists and is not
        /// currently in xrun condition (uses the #m_bProcXrun,
        /// #m_bDoneXrun flags to determine the xrun condition)
        void PushCaptureQueues();

        /// Handles the current playback data. Calls helper method to achieve
        /// that the playback data is retrieved from the processing playback
        /// queue, copied to the sound card as well as the "done" playback
        /// queue, and initiates stopping if the SoundData::m_bIsLast flag was
        /// set.
        /// \param nDoubleBufferIndex
        ///   The Asio driver's buffer index.
        void HandlePlaybackData(long nDoubleBufferIndex);

        /// Helper method used by #HandlePlaybackData.
        /// Retrieves addresses from next read buffer of the processing
        /// playback queue and of the next write buffer of the done playback
        /// queue and stores them in the array given as parameter.
        /// \param rgpsdBuffers
        ///   Addresses of next buffers of playback queues are stored here.
        ///   At index 0, the address of the next read buffer of the
        ///   processing playback queue is stored.  At index 1, the address
        ///   of next write buffer of the "done" playback queue is stored.
        ///   If one of the queues is not present or is currently in xrun
        ///   condition, then 0 is stored in place of that address.
        void GetPlaybackQueueBuffers(SoundData * rgpsdBuffers[2]);

        /// Helper method used by #HandlePlaybackData.
        /// Reads sound data from the processing playback queue,
        /// calls the OnBufferPlay callback, copies the data to the "done"
        /// queue, and in native format to the driver buffer.
        /// An xrun that has been detected during #HandleCaptureData is
        /// taken care of.
        /// \param nDoubleBufferIndex
        ///    The current asio driver buffer index
        /// \param rgpsdBuffers
        ///    Addresses of the next buffers in the Playback queues.
        ///    Index 0: Next read buffer in processing playback queue.
        ///    Index 1: Next write buffer in "done" playback queue
        void CopyPlaybackData(long  nDoubleBufferIndex,
                              SoundData * rgpsdBuffers[2]);

        /// Helper method used by #HandlePlaybackData.
        /// Call SoundDataQueue::Pop on the processing playback queue and
        /// SoundDataQueue::Push on the "done" playback queue if they exist and
        /// the respective queue is not currently in xrun condition (uses the
        /// #m_bProcXrun, #m_bDoneXrun flags to determine the xrun condition)
        void ReleasePlaybackQueues();

        /// Checks whether the last playback buffer handled in the bufferswitch
        /// had its m_bIsLast flag set. If so, then CAsio::Stop() is called on
        /// the current CAsio instance.
        /// \retval the value of the m_bIsLast flag of the last playback buffer
        bool CheckPlaybackLastFlagAndStop();

        /// Counter used in unit test, incremented for each buffer that the
        /// ClearQueues had to delete exceptionally because the CAsio::DoneLoop
        /// had terminated prematurely. This should not happen in production,
        /// and this counter is used only to check that destruction would
        /// succeed even in this unlikely case.
        unsigned m_nExceptionalDoneQueueClearCount;
  };
} // Namespace Asio

#endif
