//-----------------------------------------------------------------------------
/// \file casio.h
/// \author Berg
/// \brief Interface of CAsio class used by Asio module
///
/// Project SoundMexPro
/// Module SoundDllPro
/// Interface of CAsio class used by Asio module. Interface to Steinbergs ASIO-SDK
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
//-----------------------------------------------------------------------------
#ifndef casioH
#define casioH

#include <vector>
#include <valarray>
#include <windows.h>

#include "casioEnums.h"
#include "casioExceptions.h"

struct ASIODriverInfo;
struct ASIOBufferInfo;
struct ASIOChannelInfo;
class AsioDrivers;
struct ASIOCallbacks;
struct ASIOClockSource;
class TCAsioView;
class Test_CAsio;

namespace Asio
{
    class SoundDataExchanger;
    class SoundData;

    /// \brief A class for interacting with ASIO drivers on system.
    ///
    /// Only one ASIO driver may be loaded at any time.  This is a restriction
    /// of Steinberg's SDK, which uses a global variable to identify the
    /// currently loaded driver.  CAsio is a Singleton class and can only be
    /// instantiated once.
    ///
    /// The CAsio class creates three threads: One thread (signal processing
    /// thread) performs the signal processing.
    ///
    /// Another thread (Callback thread) waits for events (buffer
    /// underruns, errors) to occur and invokes callbacks to notify
    /// the application of these events.  them.  The stop helper
    /// thread, finally, makes it possible that the trigger for
    /// stopping signal processing can come from inside the signal
    /// processing thread.
    ///
    /// When CAsio is instantiated, it creates an instance of class
    /// AsioDrivers (defined in Steinberg's Asio SDK) using operator
    /// new, and sets a global pointer in Steinberg's SDK to point to
    /// the new instance.  It also allocates memory for one struct
    /// ASIODriverInfo and one struct ASIOCallbacks for later usage
    /// (loading resp. preparing the asio driver).  The corresponding
    /// delete operators are called in DeallocateSDK, which is called
    /// from the destructor, or from the the constructor, if the
    /// constructor fails.
    class CAsio
    {
#ifdef UNIT_TEST_CLASS
        friend class UNIT_TEST_CLASS;
#endif
        /// This class may call private methods of CAsio.
        friend class SoundDataExchanger;
    public:
        // ASIO interface encapsulation.
        // ===================================================================

        /// \brief instance access
        ///
        /// Returns a pointer to the current instance, if there is one,
        /// else a NULL pointer is returned.
        static CAsio * Instance();

        /// \brief Constructor.
        ///
        /// Only one instance of class CAsio may be present in a single
        /// process.
        /// Allocates locks and event objects. Starts threads.
        /// \throw EWin32Error
        ///   When a windows event could not be allocated.
        ///    Triggered by InitEvents.
        /// \throw EWin32Error
        ///   a thread could not be started.
        ///   Triggered by StartThreads.
        /// \throw EMultipleInstancesError
        ///     when there already is another instance of CAsio.
        ///     Triggered by this constructor.
        CAsio();

        /// \brief Destructor.
        ///
        /// Releases Asio resources.
        /// Release Events and lock.
        virtual ~CAsio();

        /// \brief The state of the ASIO soundsystem.
        ///
        /// This method returns the current state of the ASIO system.
        /// Asio::State is an enum data type defined in file casioEnums.h.
        /// May be used to find out if a driver is currently loaded and in what
        /// state that ASIO driver is at the moment.
        /// \return The current state.
        State GetState() const;

        /// \brief Number of ASIO drivers present on system.
        ///
        /// \return the number of ASIO drivers available on the system.
        /// \throw ENegativeCountError
        ///   if the Asio SDK reports a negative number of ASIO drivers.
        ///   Triggered by this method.
        virtual size_t NumDrivers() const;

        /// \brief Human-readable name of ASIO driver.
        ///
        /// Query for a human-readable name of the ASIO driver at index nIndex.
        /// \param nIndex The index of the ASIO driver whose name is requested.
        ///        nIndex >= 0 && nIndex < numDrivers().
        /// \throw EIndexError
        ///     If nIndex is out of bounds.
        ///     Triggered by this method.
        /// \throw ENegativeCountError
        ///     Asio SDK reports a negative number of drivers.
        ///     Triggered by NumDrivers.
        /// \throw EUnexpectedError
        ///     Driver name at valid index cannot be retrieved.
        ///     Triggered by this method.
        virtual AnsiString DriverNameAtIndex(size_t nIndex) const;

        /// \brief Unload current driver.
        ///
        /// Unload the ASIO driver that is currently loaded.  Do nothing
        /// if no driver is loaded.
        ///
        /// Because this method is part of the clean-up process, errors 
        /// signalled by the driver are ignored,  except that they trigger
        /// a warning (TriggerWarning->OnWarning).
        ///
        virtual void UnloadDriver();

        /// \brief Load ASIO driver by driver index.
        ///
        /// Load the ASIO driver at the specified index into memory.
        /// Unloads the currently loaded driver first if there is one.
        /// \param nIndex The index of the ASIO driver to load.
        ///        nIndex >= 0 && nIndex < numDrivers().
        /// \param lpSysRef System Reference: Window handle
        ///        (needed for DirectX ASIO driver, maybe others).
        /// \throw EIndexError
        ///        DriverNameAtIndex is called internally. It can
        ///        cause this exception when nIndex is out of bounds.
        ///        Triggered by DriverNameAtIndex
        /// \throw ENegativeCountError
        ///        Number of drivers on system is negative.
        ///        Triggered by DriverNameAtIndex->NumDrivers.
        /// \throw ELoadError
        ///        Loading the ASIO driver has failed.
        ///        Triggered by LoadDriver.
        /// \throw EVersionError
        ///        The driver is built against a different major
        ///        version of the Asio SDK than this library.
        ///        Triggered by LoadDriver.
        /// \throw ELoadError
        ///        Initializing the ASIO driver has failed.
        ///        The driver is loaded but unusable.
        ///        Triggered by LoadDriver.
        /// \throw ENoMemoryError
        ///        not enough memory for initializing Asio driver.
        ///        Triggered by LoadDriver.
        /// \throw EUnexpectedError
        ///        Asio Driver returns unexpected error code when initialized. 
        ///        Triggered by LoadDriver.
        void LoadDriverByIndex(size_t nIndex, void * lpSysRef);

        /// \brief Load ASIO driver by name.
        ///
        /// Load the ASIO driver with the the specified name into memory.
        /// Unloads the currently loaded driver first if there is one.
        /// \param sDriverName The name of the ASIO driver to load.
        /// \param lpSysRef System Reference: Window handle
        ///        (needed for DirectX ASIO driver, maybe others).
        /// \throw ENegativeCountError
        ///        Number of drivers on system is negative.
        ///        Triggered by DriverNameAtIndex->NumDrivers.
        /// \throw ELoadError
        ///        There is no driver with this name on the system.
        ///        Triggered by this method.
        /// \throw ELoadError
        ///        Loading the ASIO driver has failed in Asio SDK.
        ///        Triggered by LoadDriver.
        /// \throw EVersionError
        ///        The driver is built against a different major
        ///        version of the Asio SDK than this library.
        ///        Triggered by LoadDriver.
        /// \throw ELoadError
        ///        Initializing the ASIO driver has failed.
        ///        The driver is loaded but unusable.
        ///        Triggered by LoadDriver.
        /// \throw ENoMemoryError
        ///        not enough memory for initializing Asio driver.
        ///        Triggered by LoadDriver.
        /// \throw EUnexpectedError
        ///        Asio Driver returns unexpected error code when initialized. 
        ///        Triggered by LoadDriver.
        void LoadDriverByName(AnsiString sDriverName, void * lpSysRef);

        /// \brief Index of the driver that is currently loaded.
        ///
        /// Index of the currently loaded driver.
        /// \throw EStateError 
        ///   When no ASIO driver is currently loaded.
        ///   Triggered by AssertInitialized.
        size_t CurrentDriverIndex() const;

        /// Method checks that the driver is initialized to at least the given
        /// asMinimumState, or raises an error.
        /// Some methods require that the ASIO driver is initialized to a
        /// certain state.  This method can be called to ensure the driver is
        /// initialized as required.
        ///
        /// Mainly called from other CAsio methods, but can also be called 
        /// from client classes.
        /// \param lpszAction
        ///        Describes the action that requires the driver initialization
        /// \param asMinimumState
        ///        The required minimum state of the driver.  Asio::State
        ///        is an enum data type defined in casioEnums.h.
        /// \throw EStateError if the driver is not initialized as required.
        void AssertInitialized(const char * lpszAction,
                               State asMinimumState = INITIALIZED) const;

        /// \brief The version of ASIO supported by the driver.
        ///
        /// The major version of the ASIO SDK supported by the
        /// currently loaded driver.
        /// \throw EStateError 
        ///   When no ASIO driver is currently loaded.
        ///   Triggered by AssertInitialized.
        long DriverInfoAsioVersion() const;

        /// \brief The version number of the ASIO driver.
        ///
        /// The version number of the currently loaded ASIO driver.
        /// The driver vendor can set this freely.
        /// \throw EStateError 
        ///   When no ASIO driver is currently loaded.
        ///   Triggered by AssertInitialized.
        long DriverInfoVersion() const;

        /// \brief The internal name of the driver.
        ///
        /// An internal name of the driver, which can differ from the public
        /// ASIO driver name as returned by #driverNameAtIndex.
        /// \return The internal name of the driver.
        /// \throw EStateError 
        ///   When no ASIO driver is currently loaded.
        ///   Triggered by AssertInitialized.
        AnsiString DriverInfoName() const;

        /// \brief Number of channels supported by the hardware.
        ///
        /// The total number of channels that can be used for input or output
        /// with the current driver.
        /// \param adDirection
        ///     Query INPUT or OUTPUT channels. 
        ///     Asio::Direction is an enum data type defined in casioEnums.h.
        /// \return
        ///     Number of channels for adDirection supported by the hardware.
        /// \throw EStateError
        ///     If the driver is not initialized.
        ///     Triggered by AssertInitialized.
        virtual long HardwareChannels(Direction adDirection) const;

        /// \brief Latency.
        ///
        /// The input or output latency in samples as reported by the ASIO
        /// driver.
        /// \param adDirection
        ///     Query INPUT or OUTPUT latency.
        ///     Asio::Direction is an enum data type defined in casioEnums.h.
        /// \return Latency in samples.
        /// \throw EStateError 
        ///   If no driver is currently initialized.
        ///   Triggered by AssertInitialized.
        /// \throw EUnavailableError
        ///   If "no input/output is present".  
        ///   Triggered by this method.
        /// \throw EUnexpectedError 
        ///   If the driver returns unexpected error code. 
        ///   Triggered by this method.
        long Latency(Direction adDirection) const;

        /// \brief ASIO driver prefers this sound buffer size.
        ///
        /// The ASIO driver's currently "preferred" sound buffer size in
        /// samples per channel.  Applications should use this sound buffer
        /// size unless they have a very good reason to use something else.
        /// \return
        ///     Preferred sound buffer size in samples per channel.
        /// \throw EStateError
        ///     if no driver is currently initialized.
        ///     Triggered by AsioBufsizeQuery->AssertInitialized.
        /// \throw EUnavailableError
        ///     if "no input/output is present".
        ///     Triggered by AsioBufsizeQuery.
        /// \throw EUnexpectedError
        ///     if the buffer size parameters returned by the driver
        ///     are inconsistent.
        ///     Triggered by AsioBufsizeQuery.
        /// \throw EUnexpectedError
        ///     if the buffer size parameters returned by the driver
        ///     are inconsistent.
        ///     Triggered by AsioBufsizeQuery->CheckGranularity.
        /// \throw EUnexpectedError
        ///     if the driver returned unexpected error code.
        ///     Triggered by AsioBufsizeQuery.
        long BufsizeBest() const;

        /// \brief Return the current sound buffer size
        ///
        /// This method returns the current sound buffer size if the driver is
        /// in state PREPARED or RUNNING.
        /// If the driver is in state INITIALIZED, then this method returns
        /// the driver's preferred buffer size.
        /// If the driver is in state LOADED or FREE, throw an Exception.
        /// \throw EStateError
        ///     if no driver is currently initialized.
        ///     Triggered by BufsizeBest->AsioBufsizeQuery->AssertInitialized.
        /// \throw EUnavailableError
        ///     if "no input/output is present".
        ///     Triggered by BufsizeBest->AsioBufsizeQuery.
        ///     Triggered by BufsizeBest->AsioBufsizeQuery.
        /// \throw EUnexpectedError
        ///     if the buffer size parameters returned by the driver
        ///     are inconsistent.
        ///     Triggered by BufsizeBest->AsioBufsizeQuery->CheckBufsizeInterval        /// \throw EUnexpectedError
        ///     if the driver returned unexpected error code.
        ///     Triggered byBufsizeBest-> AsioBufsizeQuery.
        virtual long BufsizeCurrent() const;

        /// \brief Return a vector containing all supported buffer sizes.
        ///
        /// Returns a sorted vector containing all supported buffer sizes.
        /// \throw EStateError
        ///     if no driver is currently initialized.
        ///     Triggered by AsioBufsizeQuery->AssertInitialized.
        /// \throw EUnavailableError
        ///     if "no input/output is present".
        ///     Triggered by AsioBufsizeQuery.
        /// \throw EUnexpectedError
        ///     if the buffer size parameters returned by the driver
        ///     are inconsistent.
        ///     Triggered by AsioBufsizeQuery.
        /// \throw EUnexpectedError
        ///     if the buffer size parameters returned by the driver
        ///     are inconsistent.
        ///     Triggered by AsioBufsizeQuery->CheckGranularity.
        /// \throw EUnexpectedError
        ///     if the driver returned unexpected error code.
        ///     Triggered by AsioBufsizeQuery.
        std::vector<long> BufferSizes() const;

        /// \brief Query wether dSampleRate is supported.
        ///
        /// Check if the named sample rate is supported.
        /// \param dSampleRate
        ///     The sample rate to check for.
        /// \throw EStateError
        ///     if no driver is currently initialized.
        ///     Triggered by AssertInitialized.
        /// \throw EUnavailableError
        ///     if "no input/output is present".
        ///     Triggered by this method.
        /// \throw EUnexpectedError
        ///     if the driver returned unexpected error code.
        ///     Triggered by this method.
        bool CanSampleRate(double dSampleRate) const;

        /// \brief The current sampling rate.
        ///
        /// \return The sampling rate in Hz.
        /// \throw EStateError
        ///     if no driver is currently initialized.
        ///     Triggered by AssertInitialized.
        /// \throw EUnavailableError
        ///     if "no input/output is present".
        ///     Triggered by this method.
        /// \throw ENoClockError
        ///     if current sample rate is unknown.
        ///     Triggered by this method.
        /// \throw EUnexpectedError
        ///     if the driver returned unexpected error code.
        ///     Triggered by this method.
        virtual double SampleRate() const;

        /// \brief Set the sampling rate
        /// \param dSampleRate
        ///     The new sample rate.
        /// \throw EStateError
        ///     if no driver is currently initialized.
        ///     Triggered by AssertInitialized.
        /// \throw EUnavailableError
        ///     if "no input/output is present".
        ///     Triggered by this method.
        /// \throw ENoClockError
        ///     if the sample rate is unsupported.
        ///     Triggered by this method.
        /// \throw EModeError
        ///     if the current clock is external, and sampleRate is != 0.
        ///     Triggered by this method.
        /// \throw EUnexpectedError
        ///     if the driver returned unexpected error code.
        ///     Triggered by this method.
        void SampleRate(double dSampleRate) const;

        /// \brief Get number of possible clock sources.
        ///
        /// Sound cards may be able to adjust their sampling rate to different
        /// external clock sources.  Asio has API for investigating and 
        /// changing the clock source in use.  Asio SDK reference states that
        /// each sound card must have at least one clock, the internal clock.
        ///
        /// Most Asio drivers take a simplistic aproach of providing only one
        /// clock source to the Asio API, name it "Settings" or similar,
        /// and provide clock source selection support to the user through a
        /// different interface.
        ///
        /// \throw EStateError
        ///     if no driver is currently initialized.
        ///     Triggered by GetClockSources->AssertInitialized.
        /// \throw EUnavailableError
        ///     if "no input/output is present".
        ///     Triggered by GetClockSources.
        /// \throw EUnexpectedError
        ///     if the driver returned unexpected error code.
        ///     Triggered by GetClockSources.
        /// \throw ENegativeCountError
        ///     If the number of clock sources reported by the driver is 
        ///     zero or less.
        ///     Triggered by GetClockSources.
        /// \throw ELimitExceededError
        ///     If the number of clock sources reported by the driver
        ///     is greater than MAX_CLOCKS (256).  All known driver at
        ///     this time report less than 20 clock sources, most
        ///     support only 1.  This limit protects CAsio from
        ///     excessive memory allocation.
        ///     Triggered by GetClockSources.
        /// \throw EUnexpectedError
        ///     If the numbering of clock sources is inconsistent.
        ///     Triggered by GetClockSources.
        size_t NumClockSources() const;

        /// \brief Get human readable name of clock source at index.
        /// \param nIndex
        ///     The index of the investigated clock source.
        ///     nIndex >= 0 && nIndex < #numClockSources
        /// \throw EStateError
        ///     if no driver is currently initialized.
        ///     Triggered by GetClockSources->AssertInitialized.
        /// \throw EUnavailableError
        ///     if "no input/output is present".
        ///     Triggered by GetClockSources.
        /// \throw EUnexpectedError
        ///     if the driver returned unexpected error code.
        ///     Triggered by GetClockSources.
        /// \throw ENegativeCountError
        ///     If the number of clock sources reported by the driver is 
        ///     zero or less.
        ///     Triggered by GetClockSources.
        /// \throw ELimitExceededError
        ///     If the number of clock sources reported by the driver
        ///     is greater than MAX_CLOCKS (256).  All known driver at
        ///     this time report less than 20 clock sources, most
        ///     support only 1.  This limit protects CAsio from
        ///     excessive memory allocation.
        ///     Triggered by GetClockSources.
        /// \throw EUnexpectedError
        ///     If the numbering of clock sources is inconsistent.
        ///     Triggered by GetClockSources.
        /// \throw EIndexError
        ///     the index is out of range.
        ///     Triggered by this method.
        AnsiString ClockSourceName(size_t nIndex) const;

        /// \brief Index of clock source's associated channel group.
        ///
        /// Asio clock sources can be somehow associated with groups of 
        /// channels. E.g. if the clock signal of incoming ADAT channels can
        /// be selected as a clock source, then these ADAT channels would be
        /// part of this clock source's associated channel group.
        /// 
        /// The channel group of each sound channel can be queried using the
        /// ChannelGroup method.
        ///
        /// \param nIndex
        ///    Index of the investigated clock source.
        /// \return
        ///    Index of associated channel group.
        ///    Steinberg has not specified return value for cases when the 
        ///    clock source is not associated with any audio channels.
        /// 
        /// \throw EStateError
        ///     if no driver is currently initialized.
        ///     Triggered by GetClockSources->AssertInitialized.
        /// \throw EUnavailableError
        ///     if "no input/output is present".
        ///     Triggered by GetClockSources.
        /// \throw EUnexpectedError
        ///     if the driver returned unexpected error code.
        ///     Triggered by GetClockSources.
        /// \throw ENegativeCountError
        ///     If the number of clock sources reported by the driver is 
        ///     zero or less.
        ///     Triggered by GetClockSources.
        /// \throw ELimitExceededError
        ///     If the number of clock sources reported by the driver
        ///     is greater than MAX_CLOCKS (256).  All known driver at
        ///     this time report less than 20 clock sources, most
        ///     support only 1.  This limit protects CAsio from
        ///     excessive memory allocation.
        ///     Triggered by GetClockSources.
        /// \throw EUnexpectedError
        ///     If the numbering of clock sources is inconsistent.
        ///     Triggered by GetClockSources.
        /// \throw EIndexError
        ///     the index is out of range.
        ///     Triggered by this method.
        long ClockSourceGroup(size_t nIndex) const;

        /// \brief Index of first channel of clock source's associated channel 
        /// group.
        ///
        /// \param nIndex
        ///    Index of the investigated clock source.
        /// \return
        ///    Index of first channel of channel group associated with 
        ///    clock source nIndex associated channel group.
        ///    Steinberg has not specified return value for cases when the 
        ///    clock source is not associated with any audio channels.
        ///
        /// \throw EStateError
        ///     if no driver is currently initialized.
        ///     Triggered by GetClockSources->AssertInitialized.
        /// \throw EUnavailableError
        ///     if "no input/output is present".
        ///     Triggered by GetClockSources.
        /// \throw EUnexpectedError
        ///     if the driver returned unexpected error code.
        ///     Triggered by GetClockSources.
        /// \throw ENegativeCountError
        ///     If the number of clock sources reported by the driver is 
        ///     zero or less.
        ///     Triggered by GetClockSources.
        /// \throw ELimitExceededError
        ///     If the number of clock sources reported by the driver
        ///     is greater than MAX_CLOCKS (256).  All known driver at
        ///     this time report less than 20 clock sources, most
        ///     support only 1.  This limit protects CAsio from
        ///     excessive memory allocation.
        ///     Triggered by GetClockSources.
        /// \throw EUnexpectedError
        ///     If the numbering of clock sources is inconsistent.
        ///     Triggered by GetClockSources.
        /// \throw EIndexError
        ///     the index is out of range.
        ///     Triggered by this method.
        long ClockSourceChannel(size_t nIndex) const;

        /// \brief Index of currently used clock source.
        /// \throw EStateError
        ///     if no driver is currently initialized.
        ///     Triggered by GetClockSources->AssertInitialized.
        /// \throw EUnavailableError
        ///     if "no input/output is present".
        ///     Triggered by GetClockSources.
        /// \throw EUnexpectedError
        ///     if the driver returned unexpected error code.
        ///     Triggered by GetClockSources.
        /// \throw ENegativeCountError
        ///     If the number of clock sources reported by the driver is 
        ///     zero or less.
        ///     Triggered by GetClockSources.
        /// \throw ELimitExceededError
        ///     If the number of clock sources reported by the driver
        ///     is greater than MAX_CLOCKS (256).  All known driver at
        ///     this time report less than 20 clock sources, most
        ///     support only 1.  This limit protects CAsio from
        ///     excessive memory allocation.
        ///     Triggered by GetClockSources.
        /// \throw EUnexpectedError
        ///     If the numbering of clock sources is inconsistent.
        ///     Triggered by GetClockSources.
        /// \throw EUnexpectedError
        ///     If no clock source is currently marked active.
        ///     Triggered by this method.
        size_t ClockSourceCurrent() const;

        /// \brief Set clock source to use.
        /// \param nIndex
        ///     The index of the clock source to use.
        ///     nIndex >= 0 && nIndex < #numClockSources
        /// \throw EStateError
        ///     if no driver is currently initialized.
        ///     Triggered by NumClockSources->GetClockSources->AssertInitialized.
        /// \throw EUnavailableError
        ///     if "no input/output is present".
        ///     Triggered by NumClockSources->GetClockSources.
        /// \throw EUnexpectedError
        ///     if the driver returned unexpected error code.
        ///     Triggered by NumClockSources->GetClockSources.
        /// \throw ENegativeCountError
        ///     If the number of clock sources reported by the driver is 
        ///     zero or less.
        ///     Triggered by NumClockSources->GetClockSources.
        /// \throw ELimitExceededError
        ///     If the number of clock sources reported by the driver
        ///     is greater than MAX_CLOCKS (256).  All known driver at
        ///     this time report less than 20 clock sources, most
        ///     support only 1.  This limit protects CAsio from
        ///     excessive memory allocation.
        ///     Triggered by NumClockSources->GetClockSources.
        /// \throw EUnexpectedError
        ///     If the numbering of clock sources is inconsistent.
        ///     Triggered by NumClockSources->GetClockSources.
        /// \throw EModeError
        ///     if switching clock is impossible because of incoming clock on
        ///     active input channel (depending on hardware).
        ///     Triggered by this method.
        /// \throw EUnavailableError
        ///     if "no input/output is present".
        ///     Triggered by this method.
        /// \throw EUnexpectedError
        ///     if the driver returned unexpected error code.
        ///     Triggered by this method.
        /// \throw EIndexError
        ///     the index is out of range.
        ///     Triggered by this method.
        void ClockSourceSet(size_t nIndex);

        /// \brief Associated channel group for channel.
        ///
        /// ASIO drivers may divide the channels into logical groups.  This
        /// method returns the channel group associated with the specified
        /// channel.
        /// \param iChannelIndex The index of the hardware channel.
        /// \param adDirection 
        ///     Query INPUT or OUTPUT channel.
        ///     Asio::Direction is an enum data type defined in casioEnums.h.
        /// \return The corresponding group index for the hardware channel.
        /// \throw EStateError
        ///     if the driver is not initialized.
        ///     Triggered GetChannelInfo->AssertInitialized.
        /// \throw ENegativeCountError
        ///     if the number of channels in the soundcard reported by the
        ///     driver is negative.
        ///     Triggered by GetChannelInfo->HardwareChannels
        /// \throw EIndexError
        ///     if the iChannelIndex is out of bounds for this driver and
        ///     adDirection.
        ///     Triggered by GetChannelInfo.
        /// \throw EUnavailableError
        ///     if "no input/output is present".
        ///     Triggered by GetChannelInfo.
        /// \throw EUnexpectedError
        ///     if the driver returned unexpected error code.
        ///     Triggered by GetChannelInfo.
        long ChannelGroup(long iChannelIndex, Direction adDirection) const;

        /// \brief Human-readable name for the specified hardware channel
        /// \param iChannelIndex The index of the hardware channel.
        /// \param adDirection 
        ///     Query INPUT or OUTPUT channel.
        ///     Asio::Direction is an enum data type defined in casioEnums.h.
        /// \return The name for the hardware channel.
        /// \throw EStateError
        ///     if the driver is not initialized.
        ///     Triggered GetChannelInfo->AssertInitialized.
        /// \throw ENegativeCountError
        ///     if the number of channels in the soundcard reported by the
        ///     driver is negative.
        ///     Triggered by GetChannelInfo->HardwareChannels
        /// \throw EIndexError
        ///     if the iChannelIndex is out of bounds for this driver and
        ///     adDirection.
        ///     Triggered by GetChannelInfo.
        /// \throw EUnavailableError
        ///     if "no input/output is present".
        ///     Triggered by GetChannelInfo.
        /// \throw EUnexpectedError
        ///     if the driver returned unexpected error code.
        ///     Triggered by GetChannelInfo.
        virtual AnsiString ChannelName(long iChannelIndex,
                                       Direction adDirection) const;

        /// \brief return the channel data type specifier.
        ///
        /// Soundcards and even differernt channels can specify different
        /// data types that they use for sound data transfer.
        /// \param iChannelIndex
        ///    channel index for channel identification.
        /// \param adDirection
        ///    INPUT or OUTPUT channel.
        ///    Asio::Direction is an enum data type defined in casioEnums.h.
        /// \return 
        ///    A specifyer for the data type used to represent sound
        ///    samples in this channel. 
        ///
        ///    A list of possible return values appears in Steinberg's Asio SDK
        ///    2.1 reference documentation, page 31-32.  (The Sony DSD formats
        ///    from page 33 are not supported by this implementation.)
        /// 
        /// \throw EStateError
        ///     if the driver is not initialized.
        ///     Triggered GetChannelInfo->AssertInitialized.
        /// \throw ENegativeCountError
        ///     if the number of channels in the soundcard reported by the
        ///     driver is negative.
        ///     Triggered by GetChannelInfo->HardwareChannels
        /// \throw EIndexError
        ///     if the iChannelIndex is out of bounds for this driver and
        ///     adDirection.
        ///     Triggered by GetChannelInfo.
        /// \throw EUnavailableError
        ///     if "no input/output is present".
        ///     Triggered by GetChannelInfo.
        /// \throw EUnexpectedError
        ///     if the driver returned unexpected error code.
        ///     Triggered by GetChannelInfo.
        long ChannelDataType(long iChannelIndex, Direction adDirection) const;

        /// \brief Show the ASIO driver's settings dialog.
        ///
        /// Depending on the driver, this function can block until the
        /// dialogue is closed again, or return immediately if the settings
        /// dialogue executes asynchronously.
        ///
        /// ShowControlPanel creates another GUI windows and cannot be tested
        /// automatically. It is therefore tested manually.
        /// \throw EStateError
        ///     if no driver is currently initialized.
        ///     Triggered AssertInitialized.
        /// \throw EUnavailableError
        ///     if no input/output is present.
        ///     Triggered by this method.
        /// \throw EUnexpectedError
        ///     if the ASIO driver returns unexpected error code.
        ///     Triggered by this method.
        void ShowControlPanel();

        /// \brief Allocate sound data buffers.
        ///
        /// Create sound data buffers and change state from INITIALIZED to
        /// PREPARED
        /// \param vbChannelsIn
        ///     A vector of channel-activation flags, one for each hardware
        ///     input channel.
        /// \param vbChannelsOut
        ///     A vector containing one flag per hardware output channel.  The
        ///     flags determine wether the corresponding hardware channel is
        ///     active during sound i/o or not.
        /// \param iBufferSize
        ///     The buffer size to use.  This is the number of samples per
        ///     channel that is transferred from the soundcard to the
        ///     application or vice versa for each processing callback.
        /// \param iProcQueueBuffers
        ///     The number of sound buffers to use for buffered I/O.
        ///     0 means unbuffered IO.
        /// \param iDoneQueueBuffers
        ///     The number of sound buffers used to communicate the signal to
        ///     the "Done" callback (in the "Done" thread).
        /// \throw EStateError
        ///     If the driver is not exactly in state INITTIALIZED.
        ///     Triggered by this method.
        /// \throw EModeError
        ///     If the request includes active sound channels that the
        ///     sound card does not provide.  E.g, on a stereo-out
        ///     sound card, the application may request channel 0, or
        ///     channel 1, or both of them, or none.  If it requests
        ///     channel 2 or higher, then this Exception is thrown.
        ///     Triggered by this method.
        /// \throw EModeError
        ///     If the requested buffer size is not supported by the driver.
        ///     Triggered by this method.
        /// \throw EInconsistentStateError
        ///     If the state of this CAsio instance is inconsistent. This would
        ///     most likely be a bug in CAsio.
        ///     Triggered by this method.
        /// \throw ENoMemoryError
        ///     When the driver cannot alloacate enough memory.
        ///     Triggered by this method.
        /// \throw EUnavailableError
        ///     When "no input/output is present".
        ///     Triggered by this method.
        /// \throw EUnexpectedError
        ///     When the ASIO driver returns "InvalidMode" error code.
        ///     This method checks the parameters before calling into
        ///     Asio, and triggers EModeError itself if the requested
        ///     settings are not supported by the driver.  So, if the
        ///     driver still returns "InvalidMode", then it behaves
        ///     unexpectedly, therefore this Exception is of class
        ///     EUÖnexpectedError.
        ///     Triggerd by this method.
        /// \throw EUnexpectedError
        ///     The driver returns unexpected error code.
        ///     Triggerd by this method.
        void CreateBuffers(const std::vector<bool> & vbChannelsIn,
                           const std::vector<bool> & vbChannelsOut,
                           long iBufferSize,
                           long iProcQueueBuffers,
                           long iDoneQueueBuffers);

        /// \brief Discard sound data buffers.
        ///
        /// If CAsio state is RUNNING, then this method stops sound
        /// processing and waits for the stop process to finish (see
        /// Stop method).
        ///
        /// Discards sound data buffers and changes state to
        /// CAsio::INITIALIZED.
        ///
        /// Because this method is part of the clean-up process, errors
        /// signalled by the driver are ignored,  except that they trigger
        /// a warning (TriggerWarning->OnWarning).
        ///
        /// \throw EStateError
        ///     if the current #state is not CAsio::RUNNING or CAsio::PREPARED.
        ///     Triggered by AssertInitialized.
        void DisposeBuffers();

        /// \brief Return number of active channels for the given direction.
        ///
        /// \param adDirection
        ///     Query for INPUT or OUTPUT channels.
        ///     Asio::Direction is an enum data type defined in casioEnums.h.
        /// \return
        ///     Number of active channels for the given direction.
        ///     If the ASIO driver is not loaded or not in prepared for sound
        ///     i/o, return 0.
        virtual size_t ActiveChannels(Direction adDirection) const;

        /// \brief Return the sample data type used by the given active channel
        ///
        /// \param adDirection
        ///     Query for INPUT or OUTPUT channels.
        ///     Asio::Direction is an enum data type defined in casioEnums.h
        /// \param nChannel
        ///     Index of channel in the list of active channels.
        /// \return
        ///     Number of active channels for the given direction.
        ///     If the ASIO driver is not loaded or not in prepared for sound
        ///     i/o, return 0.
        long ActiveChannelDataType(Direction adDirection,
                                   size_t nChannel) const;
        
        /// \brief Asynchronously start sound i/o.
        ///
        /// Start sound i/o.  This method returns after the playback sound
        /// buffers have been filled.  Sound
        /// is processed in the signal processing thread that has been
        /// created during #CreateBuffers.
        ///
        /// If buffered I/O is used, then the sound card is not started
        /// started until the Queue of sound buffers waiting for playback has
        /// been filled.
        ///
        /// The ASIO driver needs to be in state PREPARED when this
        /// method is called.  After successful execution, the ASIO
        /// driver is in state RUNNING.
        /// \throw EStateError
        ///     if the sound driver is not in state PREPARED.
        ///     Triggered by this method.
        /// \throw EUnavailableError
        ///     if no input/output is present, or if the hardware is
        ///     malfunctioning.
        ///     Triggered by this method.
        /// \throw EUnexpectedError
        ///     if the ASIO driver returns unknown error code.
        ///     Triggered by this method.
        void Start();

        /// Buffers of the Playback processing queue are prefilled before
        /// the device is actually started.
        void PrefillPlaybackBuffers();
        
        /// \brief Asynchroneous stopping of sound I/O.
        ///
        /// Stopping Sound I/O is an asynchroneous process.  This
        /// method signals the stop request to the actor thread and
        /// returns.
        /// The actor thread will wait for another #m_nStopSwitches
        /// buffer switches, during which silence is written to the
        /// active output buffers, and then transmit the stop request
        /// to the asio driver.
        /// This way, the output buffers are cleared, and the stop request
        /// can come from any thread, including the signal processing
        /// thread.
        /// The ASIO driver needs to be in state RUNNING when this
        /// method is called.  If the Asio driver is not in state
        /// RUNNING, then this method does nothing.  The CAsio instance
        /// will change its state to PREPARED only after the stop
        /// procedure just described terminates. At this point,
        /// the manual-reset event m_hStoppedEvent is activated.
        virtual void Stop();

        /// \brief Synchroneous stopping of sound I/O
        ///
        /// Calls Stop() and waits until the stopping process terminates.
        /// This method must not be called from the signal processing
        /// thread.
        void StopAndWait();


        // Callbacks to be implemented in derived classes.
        // ===============================================

        /// \brief Signal processing main callback.
        /// Clients need to overwrite this abstract method, and do their signal
        /// input/output here.  This base class implementation touches neither
        /// inputs nor outputs.
        ///
        /// The implementation of this method should be real-time safe, i.e.
        /// Disk I/O (this includes memory allocation/deallocation) must
        /// not be done here.
        ///
        /// The client class must handle all exceptions, and not propagate
        /// exceptions to the caller.
        ///
        /// This base class implementation does nothing, just sets 4 counters
        /// that are only used in the unit tests.  Derived implementations
        /// do not need to call this base class implementation.
        ///
        /// \param sdBuffersIn
        ///     Input signal as float samples in the range [-1:+1].
        ///     Each buffer corresponds to one of the prepared input channels.
        ///
        ///     Non-active input channels are not represented.
        /// \param sdBuffersOut
        ///     Output signal as float samples.
        ///     Each buffer corresponds to one of the prepared input channels.
        ///
        ///     Non-active output channels have no representation here.
        ///
        ///     The output buffers are initialized with zeros before this
        ///     method is called. The m_bIsLast flag is initialized to false.
        ///
        ///     The range of approximately [-1:1] can be output by the
        ///     soundcard, therefore,
        ///     the sound samples produced by the client should be in this
        ///     range unless the client does something about it in the
        ///     #OnBufferPlay method, or unless hard clipping is considered ok
        ///     (usually it is not).
        ///     The exact permitted range of sound sample values for each
        ///     output channel can be retrieved from
        ///     m_vvfAciveChMinValues[OUTPUT], m_vvfAciveChMaxValues[OUTPUT],
        ///     which can differ slightly from [-1:1] due to asymmetric
        ///     positive/negative integer representation.
        /// \param nBuffersInWaiting
        ///     The number of buffers waiting in the input queue for
        ///     processing, including the current buffer sdBuffersIn.
        /// \param bPreloading
        ///     A flag indicating wether this Process callback has been
        ///     triggered while the Queue of output buffers is being preloaded.
        virtual void
        Process(SoundData & sdBuffersIn,
                SoundData & sdBuffersOut,
                unsigned nBuffersInWaiting,
                bool bPreloading);

        /// \brief Signal Visualization / Disk Writing callback.
        /// Clients may overwrite this method, and visualize and
        /// or save the signal to disk here.
        ///
        /// The client class must handle all exceptions, and not propagate
        /// exceptions to the caller.
        ///
        /// The parameter signal buffers here are taken from queues that are
        /// filled with the latest input signal from the sound card, and output
        /// signal that is currently transferred to the sound card driver.
        ///
        /// The default implementation does nothing.
        ///
        /// \param sdBuffersIn
        ///     reference to SoundData containing the input signal
        ///
        ///     Non-active input channels are not represented.
        /// \param sdBuffersOut
        ///     reference to SoundData containing the output signal
        ///
        ///     Non-active output channels have no representation here.
        ///
        /// \param nBuffersWaiting
        ///     The backlog, number of buffers waiting in the queues for
        ///     this callback.
        virtual void
        OnBufferDone(SoundData & sdBuffersIn,
                     SoundData & sdBuffersOut,
                     long nBuffersWaiting);

        /// Callback invoked after the last OnBufferDone has been called.
        virtual void OnDoneLoopStopped()
        {
            // This method sets a testing flag that is checked in test_DoneMain
            m_lpcszTestingLastCallback = "OnDoneLoopStopped";
        }
        
        /// \brief Buffer play callback.
        /// Clients may overwrite this method and have the chance to modify
        /// the output signal just before it reaches the soundcard.
        ///
        /// The client class must handle all exceptions, and not propagate
        /// exceptions to the caller.
        ///
        /// The client class must not perform costly computations in this
        /// callback. XRuns caused by the processing in this callback cannot
        /// be detected! This callback executes in the Asio driver thread.
        ///
        /// The default implementation does nothing.
        ///
        /// \param sdBuffersOut
        ///     reference to SoundData containing the output signal
        ///
        ///     Non-active output channels have no representation here.
        ///
        #pragma argsused
        virtual void
        OnBufferPlay(SoundData & sdBuffersOut)
        { // Base class implementation does nothing and does not need a test
        }

        /// \brief The sampling rate changed.
        ///
        /// The Asio driver has notified this application that the
        /// sampling rate has changed.
        ///
        /// The cause of the sampling rate change can be either a
        /// change in the external clock used for synchronization, or
        /// another Asio application changing the internal clock rate
        /// of the same sound card.
        ///
        /// Be aware that most drivers do not bother to notify the
        /// application of a change in sampling rate.
        ///
        /// The default implementation calls Stop.
        #pragma argsused
        virtual void OnRateChange(double dSrate)
        {
            Stop();
        }

        /// \brief The buffer size changed.
        ///
        /// The Asio driver has notified this application that the
        /// buffer size has changed.
        ///
        /// The default implementation calls Stop.
        virtual void OnBufferSizeChangeChange()
        {
            Stop();
        }

        /// \brief A reset request was sent by driver.
        ///
        /// The Asio driver has notified this application that the
        /// everything to be resetted. Not supported, thus we stop.
        ///
        /// The default implementation calls Stop.
        virtual void OnResetRequest()
        {
            Stop();
        }
    

        /// Clients overwrite this callback to be notified when the state of
        /// the Asio driver changes. Since the state is sometimes changed in
        /// situations that have to succeed (stopping sound output, unloading
        /// Asio driver), exceptions raised by the callback implementation are
        /// ignored. The callback implementation should not raise exceptions.
        /// The default implementation does nothing important.
        /// \param asState
        ///    The new state of the asio driver.  Asio::State is an enum data
        ///    type defined in asioEnums.h.
        #pragma argsused
        virtual void OnStateChange(State asState)
        {
            m_lpcszTestingLastCallback = "OnStateChange";
        }

        /// Some error return values of the Asio driver are ignored
        /// during cleanup of the Asio driver. They trigger this warning
        /// callback.
        /// The default implementation sets the static member
        /// m_lpcszLastWarning.
        /// While this method can be overwritten in derived classes,
        /// be aware that only the base class implementation executes
        /// when OnWarning is called from the destructor or while the
        /// destructor executes in a different thread.
        virtual void OnWarning(const char * lpcszWarning)
        {
            sm_lpcszLastWarning = lpcszWarning;
        }

        /// Clients have to implement this method to be notified about XRuns.
        /// This method executes in the callback thread.
        /// The default implementation does nothing important.
        /// \param xtXrunType Detail on where the xrun occurred.
        virtual void OnXrun(XrunType xtXrunType)
        {
            m_xtTestingLastXrunType = xtXrunType;
            m_lpcszTestingLastCallback = "OnXrun";
        }


        /// Called from the callback thread when the signal processing thread
        /// detects an error in the client's signal processing. The signal
        /// processing code has called stop() prior to causing this callback.
        /// The default implementation does nothing important.
        virtual void OnFatalError()
        {
            m_lpcszTestingLastCallback = "OnFatalError";
        }

        /// Called from the callback thread when state is RUNNING, but
        /// the bufferSwitch Callback has not been called by the
        /// driver for some time (m_nWatchdogTimeout milliseconds).
        /// The default implementation does nothing important.
        virtual void OnHang()
        {
            m_lpcszTestingLastCallback = "OnHang";
        }

    private:
        // Internal methods
        // ================

        // Thread-related internal methods

        /// Called by the ASIO driver thread:  Signal an XRun to the actor
        /// thread.
        /// \param xtXrunType
        ///   true if the Xrun occurred in the processing queues,
        ///   false if the Xrun occurred in the "done" queues.
        virtual void SignalXrun(XrunType xtXrunType);

        /// Called by the ASIO driver thread when realtime processing is 
        /// selected.
        bool CheckForRealtimeXrun();
        
        /// Undo allocations and global pointer settings done in the
        /// constructor.  Called from destructor and from exception
        /// handler in constructor.
        void DeallocateSDK();

        /// Initialize Win32 Events needed for thread synchronization.
        /// To be called from constructor.  To be called from
        /// constructor.
        /// \throw EWin32Error
        ///     If creation of some event fails.  Caller is
        ///     responsible for calling DestroyEvents() to clean up
        ///     events that have already been created.
        ///     Triggerecd by this method.
        void InitEvents();

        /// Destroy allocated Win32 Event objects.  To be called 
        /// from destructor, or from InitEvents if creation of
        /// some event fails.  Silently ignores events that still
        /// have value NULL.
        void DestroyEvents();
        
        /// Create and start signal processing thread, callback
        /// thread, and stop helper thread.  To be called from
        /// constructor.
        /// \throw EWin32Error
        ///     If thread creation fails.  Threads that have already been
        ///     created are terminated before the exception is thrown.
        ///     Triggerecd by this method.
        void StartThreads();

        /// Stop and close signal processing thread, callback thread,
        /// and stop helper thread.  Silently ignores requests to stop
        /// the threads when the threads are already stopped.  To be
        /// called from destructor, or from StartThreads if creation of
        /// some threads fails.
        void StopThreads();

        // Callback thread startup function
        static DWORD WINAPI CbStartFunction(void * pvCAsio);

        /// Main loop for the Callback thread.
        virtual void CbMain();

        // Stop helper thread startup function
        static DWORD WINAPI StopStartFunction(void * pvCAsio);

        /// Main loop for the Stop helper thread.
        virtual void StopMain();

        /// signal processing thread startup function
        static DWORD WINAPI ProcStartFunction(void * pvCAsio);

        /// Main function for the signal processing thread created by this
        /// object.
        virtual void ProcMain();

        /// inner loop of processing thread.
        virtual void ProcLoop();

        /// Called in each iteration of the #ProcLoop. Signals that the ProcLoop
        /// should terminate if the STOP of QUIT event is set.
        /// \return true if one of the terminating events is set.
        bool ShouldProcLoopTerminate();

        /// Called in each iteration of the #ProcLoop. Returns the number of
        /// waiting buffers. If there is currently no waiting buffer, then
        /// it calls #WaitForProc and returns that return value.
        /// \return The number of buffers available for the processing thread.
        unsigned BuffersWaitingForProcLoop();

        /// Called in each iteration of the #ProcLoop. Locates buffers in the
        /// processing queues to read from and write to, and calls Process
        /// with them.
        /// \param nBuffersWaiting Number of waiting buffers, to relay to
        //                         #Process
        void HandleProcLoopData(unsigned nBuffersWaiting);

        /// Called from #ProcLoop when no processing can be performed.
        /// Waits until either (1) data becomes available in the processing
        /// capture queue AND space becomes available in the processing
        /// playback queue, or (2) until m_phProcEvents[QUIT] or
        /// m_phProcEvents[PROC_STOP] become set.
        /// @return The number of buffers present in the processing
        ///         capture queue at return time.
        virtual unsigned WaitForProc();

        /// startup function for the "done" thread
        static DWORD WINAPI DoneStartFunction(void * pvCAsio);

        /// Main function for the "done" thread
        virtual void DoneMain();

        /// Inner loop of the "done" thread, entered when Asio processing
        /// starts.  This loop waits for sound data buffers in the "done"
        /// queues of the SoundDataExchanger, and calls the #OnBufferDone
        /// callback with this sound data for visualization and disk
        /// recording.
        /// Method may only be called by DoneMain, except for testing.
        /// In every iteration, this method checks if the
        /// m_phDoneEvents[QUIT] event is set, and returns if it is.
        /// When the done queues are empty, #WaitForDoneData is called,
        /// but only as long as #m_bDoneLoopWaitsWhenDoneQueuesEmpty flag is
        /// true. (It is set to true when this method is entered. It is set to
        /// false when WaitForDoneData detects the DONE_STOP event.)
        /// This way it is ensured that all "done" data that were produced are
        /// handled, and that this loop exits when no more data produced.
        virtual void DoneLoop();

        /// Returns true while #DoneLoop() executes in the "Done" thread.
        bool IsDoneLoopActive();

        /// Called from #DoneLoop when no "done" buffers are present.
        /// Waits until either data becomes available in the "done" queues,
        /// or until m_phDoneEvents[QUIT] or m_phDoneEvents[DONE_STOP] become
        /// set.
        /// Sets #m_bDoneLoopWaitsWhenDoneQueuesEmpty to false when the
        /// m_phDoneEvents[DONE_STOP] event is detected.
        /// @return The number of buffers present in the "done" queues at
        ///         return time.
        virtual unsigned WaitForDoneData();

        // ASIO related internal methods

        /// ComputeStopTimeout computes the time theoretically needed for one
        /// buffer switch, rounds it up to the next full millisecond,
        /// and adds nTolerance milliseconds to the result.
        /// Used by CreateBuffers.
        /// \param nBufsize
        ///   Number of samples per channel in ASIO buffers per bufferswitch
        /// \param dSrate
        ///   sample rate / Hz. Must be at least 1000 Hz.
        /// \param nTolerance
        ///   The resulting timeout time exceeds the buffer switch period
        ///   by this number of milliseconds.
        /// \return
        ///   The timeout, in milliseconds, computed by this result.
        /// \throw EUnsupportedParamsError
        ///     If dSrate < 1000 or if parameters cause unsigned int overflow.
        ///     Triggered by this method.
        static unsigned int ComputeStopTimeout(unsigned int nBufsize,
                                               double dSrate,
                                               unsigned int nTolerance = 1);

        /// Fill output buffers with zeros when called.
        void SilenceOutputs(long nDoubleBufferIndex);

        /// To be called from BufferSwitch handler. Checks for stopping:
        /// short-cut processing thread if sound i/o is about to stop
        /// Calls #SilenceOutputs and advances the stop progress.
        /// \return true if we are currently stopping
        bool SilenceOutputsIfStopping(long nDoubleBufferIndex);

        /// \brief loads driver with specified name and index.
        ///
        /// Load the ASIO driver with the the specified name into memory.
        /// Unloads the currently loaded driver first if there is one.
        /// \param sDriverName The name of the ASIO driver to load.
        /// \param nIndex The index of the driver to load.
        /// \param lpSysRef System Reference: Window handle
        ///        (needed for DirectX ASIO driver, maybe others).
        /// \throw EUnexpectedError
        ///        Asio Driver returns unexpected error code when it
        ///        is loaded or initialized. 
        ///        Triggered by this method.
        /// \throw ENoMemoryError
        ///        not enough memory for initializing Asio driver.
        ///        Triggered by this method.
        /// \throw ELoadError
        ///        Loading the ASIO driver has failed.
        ///        The driver is not loaded.  
        ///        Triggered by this method.
        /// \throw ELoadError
        ///        Initializing the ASIO driver has failed.
        ///        The driver is loaded but unusable.  
        ///        Triggered by this method.
        /// \throw EVersionError
        ///        The driver is built against a different major
        ///        version of the Asio SDK than this library.
        ///        Triggered by this method.
        void LoadDriver(AnsiString sDriverName, 
                        size_t nIndex, 
                        void * lpSysRef);

        /// \brief Number of channels supported by the hardware.
        ///
        /// Determines the total number of channels that can be used for input
        /// or output with the current driver and writes it to members.
        /// \throw EStateError
        ///     If the driver is not initialized.
        ///     Triggered by AssertInitialized.
        /// \throw EUnexpectedError
        ///     If driver returns negative number of channels.
        ///     Triggered by this method.
         virtual void HardwareChannelsInternal();


        /// This method queries the buffer size parameters from the driver
        /// and checks them for consistency.
        /// \return the driver's preferred buffer size
        /// \param piMin
        ///     if != 0, the method will store the minimum buffer size here.
        /// \param piMax
        ///     if != 0, the method will store the maximum buffer size here.
        /// \param piGranularity
        ///     if != 0, store the buffer size granularity here.
        ///     When buffer size granularity is > 0, then valid buffer sizes
        ///     are {*piMin, *piMin + *piGranularity, 
        ///     *piMin + 2 * *piGranularity, ..., *piMax}. When buffer size
        ///     granularity is -1, then valid buffer sizes are {*piMin,
        ///     *piMin * 2, *piMin * 4, *piMin * 8, ..., *piMax}.
        ///     The granularity does not matter if *piMin == *piMax, and some
        ///     asio drivers are known to report *piGranularity=0 in this case.
        /// \throw EStateError
        ///     if no driver is currently initialized.
        ///     Triggered by AssertInitialized.
        /// \throw EUnavailableError
        ///     if "no input/output is present".
        ///     Triggered by this method.
        /// \throw EUnexpectedError
        ///     if the buffer size parameters returned by the driver
        ///     are inconsistent.
        ///     Triggered by this method.
        /// \throw EUnexpectedError
        ///     if the buffer size parameters returned by the driver
        ///     are inconsistent.
        ///     Triggered by CheckGranularity.
        /// \throw EUnexpectedError
        ///     if the driver returned unexpected error code.
        ///     Triggered by this method.
        long AsioBufsizeQuery(long * piMin = 0,
                              long * piMax = 0,
                              long * piGranularity = 0) const;


        /// Validity check on buffer size parameters reported by the alsa
        /// driver.  Driver reports 3 buffersizes (minimum, preferred, and
        /// maximum), and a "granularity" value.  0 < minimum <= preferred <=
        /// maximum has to be true, granularity, if positive, is the
        /// difference between valid buffer sizes.  If granularity is -1, then
        /// valid buffer sizes differ by a factor 2.  This method checks if
        /// the granularity is consistent with the given interval sizes. The
        /// granularity can be checked int the two passes, in intervals
        /// [minimum, preferred] and [preferred, maximum].  the given buffer
        /// size interval borders.
        /// \param iLower
        ///    minimum buffer size or preferred buffer size
        /// \param iUpper
        ///    preferred buffer size or maximum buffer size
        /// \param iGran
        ///    buffer size granularity.
        /// \throw EUnexpectedError
        ///    if the parameters are inconsistent.
        #pragma clang diagnostic push
        #pragma clang diagnostic ignored "-Wmicrosoft-extra-qualification"
        static void CAsio::CheckBufsizeInterval(long iLower,
                                                long iUpper,
                                                long iGran);
        #pragma clang diagnostic pop        
        /// \brief returns true if parameter is a power of 2
        static bool IsPowerOf2(unsigned long iValue);
        
        /// Fill a ASIOChannelInfo structure with values.
        /// \param iChannelIndex
        ///     The channel's index.
        ///     0 <= iChannelIndex < #hardwareChannels (adDirection)
        /// \param adDirection
        ///     Direction of sound data transfer (INPUT or OUTPUT)
        ///     Asio::Direction is an enum data type defined in casioEnums.h.
        /// \param paciInfo
        ///     The ASIOChannelInfo content will be stored here.
        /// \throw EStateError
        ///     if the driver is not initialized.
        ///     Triggered by AssertInitialized.
        /// \throw UnexpectedError
        ///     if the number of channels in the soundcard reported by the
        ///     driver is negative.
        ///     Triggered by HardwareChannels
        /// \throw EIndexError
        ///     if the iChannelIndex is out of bounds for this driver and
        ///     adDirection.
        ///     Triggered by this method.
        /// \throw EUnavailableError
        ///     if "no input/output is present".
        ///     Triggered by this method.
        /// \throw EUnexpectedError
        ///     if the driver returned unexpected error code.
        ///     Triggered by this method.
        void GetChannelInfo(long iChannelIndex,
                            Direction adDirection,
                            ASIOChannelInfo * paciInfo) const;

        /// Fill in a vector of clock source descriptions.
        /// \param vacsClocks
        ///     The clock sources will be stored here.
        /// \throw EStateError
        ///     if no driver is currently initialized.
        ///     Triggered by AssertInitialized.
        /// \throw EUnavailableError
        ///     if "no input/output is present".
        ///     Triggered by this method.
        /// \throw EUnexpectedError
        ///     if the driver returned unexpected error code.
        ///     Triggered by this method.
        /// \throw EUnexpectedError
        ///     If the number of clock sources reported by the driver is
        ///     zero or less.
        ///     Triggered by this method.
        /// \throw EUnexpectedError
        ///     If the numbering of clock sources is inconsistent.
        ///     Triggered by this method.
        void GetClockSources(std::vector<ASIOClockSource> & vacsClocks) const;

        /// Method sets the state, calls OnStateChange,
        /// and ignores all Exceptions that may result from the OnStateChange
        /// callback.
        /// \param asState
        ///     The new state.  Asio::State is an enum data type defined in
        ///     casioEnums.h.
        void SetState(State asState);

        /// Calls OnWarning, ignoring all exceptions that might arise there.
        void TriggerWarning(const char * lpcszWarning);

        /// Read all input channels from the driver buffer and write them as
        /// floats into the given float buffer.
        /// \param nDoubleBufferIndex
        ///   Which of the two driver buffers is the one to use.
        /// \param vvfBuf
        ///   Converted float values are stored in this buffer.
        ///   The number of channels (1st index) and the nuber of frames in
        ///   each channel have to be accurate, no reallocation is performed
        ///   in this method.
        void ConvertInputsToFloat(long nDoubleBufferIndex,
                                  std::vector<std::valarray<float> > & vvfBuf);

        /// Write all output channels to the driver buffer, converting them
        /// from the given float buffer.
        /// \param nDoubleBufferIndex
        ///   Which of the two driver buffers is the one to use.
        /// \param vvfBuf
        ///   Source float values are stored in this buffer.
        ///   The number of channels (1st index) and the nuber of frames in
        ///   each channel have to match the current prepared asio buffers.
        void ConvertFloatToOutputs(long nDoubleBufferIndex,
                                   std::vector<std::valarray<float> > & vvfBuf);
     protected:
        /// Retrieve the current SoundDataExchanger, or NULL if there is none.
        SoundDataExchanger * GetSoundDataExchanger();

     private:
        /// Driver should call this method when the sample rate changes.
        /// (Some don't.)
        static void StaticSampleRateDidChange(double dSrate);

        /// Driver calls this method to notify application of certain events.
        /// For a description of the parameters, see the Asio SDK reference
        /// documentation, pages 27-30.
        /// This class implements only the minimum required selectors
        /// (kAsioSelectorSupported and kAsioEngineVersion).
        static long StaticAsioMessage(long iSelector,
                                      long iValue,
                                      void* pvMessage,
                                      double* pdOpt);


        // internal methods for data manipulation and checks (helper methods)

        /// Resize a bit vector to a target length.  Added bits are set to
        /// "false".  Truncated bits have to be false.
        /// \param vbIn The bit vector to adjust
        /// \param nSize Target length.
        /// \return a resized copy of vbIn
        /// \throw EModeError
        ///    When at least 1 bit that would have to be truncated was "true".
        static std::vector<bool>
        ResizeBitVector(const std::vector<bool> & vbIn, size_t nSize);

        /// Converts buffer with sound data as received from ASIO to samples of
        /// type float.
        /// \param pvSrc
        ///     The buffer pointer from the ASIO driver
        /// \param iDataType
        ///     The ASIO sample type for this input channel.
        /// \param vfDest
        ///     Ouputs the float samples here.
        static void Asio2Float(const void * pvSrc, long iDataType,
                               std::valarray<float> & vfDest);

        /// Clips floating point samples to [-1:1] and converts them to the
        /// data type needed by the ASIO driver.
        /// \param vfSrc
        ///     float samples for soundcard output
        /// \param pvDest
        ///     The buffer pointer from the ASIO driver
        /// \param iDataType
        ///     The ASIO sample type for this output channel.
        static void Float2Asio(const std::valarray<float> & vfSrc,
                               void * pvDest, long iDataType);

        /// The maximum possible floating-point sample value if the underlying
        /// Asio data type has the given type code
        /// \param iDataType
        ///     The ASIO sample type for this output channel.
        /// \return The maximum possible floating-point sample value for
        ///         the given asio sample data type.
        static float MaxFloatSample(long iDataType);
        
        /// The minimum possible floating-point sample value if the underlying
        /// Asio data type has the given type code
        /// \param iDataType
        ///     The ASIO sample type for this output channel.
        /// \return The minimum possible floating-point sample value for
        ///         the given asio sample data type.
        static float MinFloatSample(long iDataType);
        
        /// Returns the round-trip float value for a given float sample value
        /// and an asio sample data type. The float value is rounded and
        /// clipped to the next representable value in the asio data type.
        #pragma clang diagnostic push
        #pragma clang diagnostic ignored "-Wmicrosoft-extra-qualification"
        static float CAsio::RoundTripFloatSample(long iDataType,
                                                 float fSampleValue);
        #pragma clang diagnostic pop        
        // ====================================================================
        // Data members of this class follow.

        // Thread and Win32 Event handling
        // ====================================================================

      protected:
        /// Synchronization lock.  The thread executing the bufferSwitch
        /// callback can detect XRun through not being able to enter this
        /// critical section.
        CRITICAL_SECTION m_csLock;
      private:
        /// Named indices for threads created by the CAsio instance
        enum ThreadIndex {
            CB_THREAD, STOP_THREAD, PROC_THREAD, DONE_THREAD, CASIO_THREADS
        };

        /// Named event indices for events the callback thread responds to.
        #pragma clang diagnostic push
        #pragma clang diagnostic ignored "-Wduplicate-enum"
        enum CbEventsIndex {
            QUIT,
            PROC_ERROR,
            XRUN, XRUN_PROC = XRUN + XR_PROC,
            XRUN_DONE = XRUN + XR_DONE,
            XRUN_RT = XRUN + XR_RT,
            OBSERVE, DONE_ERROR, CB_EVENTS
        };
        #pragma clang diagnostic pop

        /// Named event indices for events the stop helper thread responds to.
        enum StopEventsIndex {
            QUIT_STOP = QUIT, STOP_BEGIN, STOP_CONTINUE, STOP_EVENTS
        };

        /// Named event indices for events the signal processing thread
        /// responds to
        enum ProcEventsIndex {
            QUIT_PROC = QUIT, PROC_START, PROC_STOP,
            PROC_EVENTS_WITHOUT_STOP = PROC_STOP, PROC_EVENTS
        };

        enum DoneEventsIndex {
            QUIT_DONE = QUIT, DONE_START, DONE_STOP,
            DONE_EVENTS_WITHOUT_STOP = DONE_STOP, DONE_EVENTS
        };
        /// Array containing all threads' Win32 Event objects
        HANDLE m_rghEvents[CB_EVENTS + STOP_EVENTS + PROC_EVENTS + DONE_EVENTS];

        /// Pointer into the part of array m_rghEvents containing
        /// Win32 Event Objects for the callback thread.
        ///
        /// The event at index QUIT signals the callback thread to terminate.
        /// This event object is created in manual-reset mode.
        ///
        /// The event at index PROC_ERROR signals the actor thread that an
        /// error occured in the signal processing thread, most likely an
        /// exception from the client's processing methods reached the level
        /// where the processing methods are called.  The actor thread will
        /// call the OnFatalError callback.  This is an auto-reset event.
        ///
        /// The events at indices XRUN_PROC, XRUN_DONE, XRUN_RT signal Xruns
        /// at different locations. These are created in
        /// auto-reset mode.
        ///
        /// The event at index OBSERVE signals the callback thread
        /// that is should start or end acting as a watchdog for sound
        /// i/o.  This is an auto-reset event.
        HANDLE * m_phCbEvents;

        /// Pointer into the part of array m_rghEvents containing
        /// Win32 Event Objects for the stop helper thread.
        ///
        /// The event at index QUIT signals the callback thread to terminate.
        /// This event object is created in manual-reset mode.
        ///
        /// The event at index STOP_BEGIN signals the stop helper to
        /// invoke the stop procedure, if it is not already in
        /// progress.  The stop procedure includes: outputting buffers
        /// of silence (number determined by m_nStopSwitches), but
        /// with limited wait time (determined by m_nStopTimeout) for
        /// each buffer switch to avoid infinite hangs.  Calling
        /// ASIOStop thereafter.  This event is created in auto reset
        /// mode.
        ///
        /// The event at index STOP_CONTINUE signals progress in the
        /// stop procedure: Another buffer switch has occurred and the
        /// output buffers have been filled with silence.  The stop
        /// helper thread counts the number of buffer switches and
        /// calls ASIOStop when the designated number of silence
        /// output buffers has been reached.  This event is created in
        /// auto reset mode.
        HANDLE * m_phStopEvents;

        /// Pointer into the part of array m_rghEvents containing
        /// Win32 Event Objects for the signal processing thread.
        ///
        /// The event at index QUIT signals the signal processing thread to
        /// exit from its event loop.
        /// This event object is created in manual-reset mode.
        ///
        /// The event at index PROC_START signals that the signal processing
        /// thread should start its sound I/O loop.  The event at index
        /// PROC_STOP signals that the signal processing thread should stop
        /// performing sound I/O. Both are created in
        /// auto-reset mode.
        HANDLE * m_phProcEvents;

        /// Pointer into the part of array m_rghEvents containing
        /// Win32 Event Objects for the "done" thread, which handles
        /// disk recording and visualization.
        ///
        /// The event at index QUIT signals the "done" thread should
        /// exit.
        /// This event object is created in manual-reset mode.
        ///
        /// The event at index DONE_START signals that the signal processing
        /// is about to start, and that the "done" thread should enter the
        /// #DoneLoop() method to wait for data and call #OnBufferDone
        /// repeatedly.
        /// The event at index DONE_STOP signals that the stop procedure
        /// has been initiated, and that the "done" thread does not need to
        /// wait for more data when all current data is exhausted.
        /// The last two are created in auto-reset mode.
        HANDLE * m_phDoneEvents;

    protected:
        /// A Win32 Event that is set when the Asio driver is not
        /// currently running.  Can be used to detect stop procedure
        /// completion.  Not associated with a particular thread.  The
        /// destructor and UnloadDriver() make use of this event.
        HANDLE m_hStoppedEvent;
    private:

        /// number of hardware channels for input
        long m_nHardwareChannelsIn;
        /// number of hardware channels for output
        long m_nHardwareChannelsOut;

        /// Callback thread handle.  This thread enables the CAsio
        /// instance to imform the application of errors and underruns
        /// asynchronously, a must for reporting errors from the
        /// processing thread.
        HANDLE m_hCbThread;

        /// Stop helper thread handle.  This thread initiates and
        /// monitors the asynchronous stop procedure
        HANDLE m_hStopThread;

        /// The thread handle for the signal processing thread.
        HANDLE m_hProcThread;

        /// The thread handle for the visualization & disk recording thread.
        HANDLE m_hDoneThread;

        // Asio driver data

        /// Global pointer to the single permitted instance of this class.
        static CAsio * sm_pcaInstance;

        /// Copy of the AsioDrivers pointer in the SDK.  Allocated by the
        /// constructor and deallocated by the destructor.
        AsioDrivers * m_padAsioDrivers;

        /// The state of the currently loaded driver or FREE, if there is no
        /// driver currently loaded.  Asio::State is an enum data type defined
        /// in casioEnums.h.
        State m_asState;

        /// The index of the currently loaded ASIO driver as enumerated by the
        /// ASIO SDK, or -1 if there is no driver currently loaded.
        long m_nCurrentDriverIndex;

        /// The ASIODriverInfo structure to fill out by the ASIO driver
        /// upon loading, allocated by the constructor and deallocated by the
        /// destructor.
        ASIODriverInfo * m_padiDriverInfo;


        // Data needed while State == PREPARED or RUNNING

        /// The number of prepared INPUT channels.
        size_t m_nActiveChannelsIn;

        /// Number of prepared OUTPUT channels
        size_t m_nActiveChannelsOut;

        /// Array of buffer info structures for prepared ASIO channels.
        /// INPUT channels come first, OUTPUT channels last.
        /// Allocated by #createBuffers, deallocated by #disposeBuffers.
        ASIOBufferInfo * m_rgabiBufferInfos;

    protected:
        /// Allocated buffer size.  Set by #createBuffers, reset to 0 by
        /// #disposeBuffers.
        /// note: this is needed to determine buffer size when total number
        /// of active channels is 0.
        long m_iPreparedBuffersize;

        /// The masks (INPUT and OUTPUT) of prepared Asio channels.
        /// First index: INPUT or OUTPUT (defined as values of enum data type
        /// Asio::Direction in casioEnums.h).
        /// Second index: Asio channel index
        /// Stored: A bool indicating wether this channel is prepared for
        /// sound I/O
        std::vector<std::vector<bool> > m_vvbActiveChMasks;

        /// The indices of prepared Asio channels.
        /// First index: INPUT or OUTPUT (defined as values of enum data type
        /// Asio::Direction in casioEnums.h).
        /// Second index: nth prepared channel for that direction
        /// Stored: The Asio channel index
        std::vector<std::vector<long> > m_vviActiveChIndices;

        /// The names of prepared Asio channels.
        /// First index: INPUT or OUTPUT (defined as values of enum data type
        /// Asio::Direction in casioEnums.h).
        /// Second index: nth prepared channel for that direction
        /// Stored: The Asio channel name
        std::vector<std::vector<AnsiString> > m_vvsActiveChNames;

        /// The data types of prepared Asio channels.
        /// First index: INPUT or OUTPUT (defined as values of enum data type
        /// Asio::Direction in casioEnums.h).
        /// Second index: nth prepared channel for that direction
        /// Stored: The Asio channel's data type code
        std::vector<std::vector<long> > m_vviActiveChDataTypes;

        /// The maximum sample values for the data types of the prepared
        /// asio channels.
        /// First index: INPUT or OUTPUT (defined as values of enum data type
        /// Asio::Direction in casioEnums.h).
        /// Second index: nth prepared channel for that direction
        /// Stored: The maximum possible float sample value for the underlying
        /// asio data type (usually a bit lower than +1.0)
        /// The contents of this array is only valid when in state
        /// PREPARED or RUNNING.
        std::vector<std::vector<float> > m_vvfAciveChMaxValues;

        /// The minimum sample values for the data types of the prepared
        /// asio channels.
        /// First index: INPUT or OUTPUT (defined as values of enum data type
        /// Asio::Direction in casioEnums.h).
        /// Second index: nth prepared channel for that direction
        /// Stored: The minimum possible float sample value for the underlying
        /// asio data type (usually exactly -1.0)
        /// The contents of this array is only valid when in state
        /// PREPARED or RUNNING.
        std::vector<std::vector<float> > m_vvfAciveChMinValues;
    private:
        /// Structure with function pointers into this host.  Allocated by the
        /// constructor and deallocated by the destructor.  Filled out by
        /// #createBuffers.
        ASIOCallbacks * m_pacbCallbacks;

    protected:
        /// Watchdog timeout, in milliseconds.  While sound i/o is
        /// active, the callback thread wakes up every
        /// m_nWatchdogTimeout milliseconds and checks if the
        /// m_nProcBufferSwitches counter has changed its value.  If
        /// that counter has not changed its value, then the asio
        /// driver has ceased to trigger the bufferSwitch callback.
        /// Callback OnHang is executed to notify the application.
        unsigned int m_nWatchdogTimeout;

        /// Flag for #processBuffer(long) method.
        bool m_bStopping;

        /// When stopping, and playing buffers of silence:  How many
        /// milliseconds to wait for each bufferswitch.
        /// CreateBuffers sets this value based on the prepared buffer size
        /// and the current sampling rate.
        unsigned int m_nStopTimeout;

        /// When stopping, how many times has the stopper thread woken
        /// up because of the timeout, and not because of the buffer
        /// switch callback?
        size_t m_nStopTimeoutsWaited;

        /// How many buffswitches (or timeouts, if the bufferswitch does not
        /// occur) should the CAsio class play back silence upon stopping?
        size_t m_nStopSwitches;

        /// When stopping, how many StopSwitches have already occurred?
        size_t m_nStopSwitchesWaited;

        /// Watchdog counter. Incremented in the Asio driver thread during 
        /// every buffer switch callback. This member is incremented
        /// from SoundDataExchanger::StaticBufferSwitch
        size_t m_nDrvBufferSwitches;

        /// Watchdog counter.  Incremented in the processing thread
        /// for every wakeup.  Due to stop procedure or xruns, the
        /// update rate of this counter is not as high as
        /// m_nDrvBufferSwitches
        size_t m_nProcBufferSwitches;

        /// Watchdog counter. For Checking Watchdog activity.
        size_t m_nWatchdogWakeups;

        /// Sound data exchanger object, allocated in #CreateBuffers,
        /// deallocated in #ReleaseBuffers.
        SoundDataExchanger * m_psxSoundDataExchanger;

        /// True when the processing queue has logical size 0 
        /// (realtime processing), AND the processing thread is
        /// currently busy processing the data.
        bool m_bDataNeedsRealtimeProcessing;

        /// Last warning message pointer
        static const char * sm_lpcszLastWarning;

        /// For testing only: Base class implementation of Process sets this
        /// to the number of input sound channels
        size_t m_nTestingProcessChannelsIn;

        /// For testing only: Base class implementation of Process sets this
        /// to the number of sound samples per input channel, or 0 if there
        /// are no input channels.
        size_t m_nTestingProcessFramesIn;

        /// For testing only: Base class implementation of Process sets this
        /// to the number of output sound channels
        size_t m_nTestingProcessChannelsOut;

        /// For testing only: Base class implementation of Process sets this
        /// to the number of sound samples per output channel, or 0 if there
        /// are no output channels.
        size_t m_nTestingProcessFramesOut;

        /// For testing only: Base class implementation of Process sets
        /// this to the number of waiting sound buffers
        size_t m_nTestingProcessBuffersInWaiting;

        /// For testing only: Base class implementation of Process sets
        /// this to the flag that indicates wether CAsio is currently prefilling
        /// or if the soundcard has started for real.
        bool m_bTestingProcessPreloading;

        /// Testing field used in OnBufferDone: Number input channels
        size_t m_nTestingBufferDoneChannelsIn;

        /// Testing field used in OnBufferDone: Number output channels
        size_t m_nTestingBufferDoneChannelsOut;

        /// Testing field used in OnBufferDone: Number of buffers in done queue
        size_t m_nTestingDoneBuffersWaiting;

        /// Testing field used in OnBufferDone: 1st sample in 1st input channel
        float m_fTestingBufferDoneCaptureSample;

        /// Testing field used in OnBufferDone: 1st sample in 1st output channel
        float m_fTestingBufferDonePlaybackSample;

        /// Testing flag set when #ShouldProcLoopTerminate executes
        bool m_bShouldProcLoopTerminateCalled;

        /// Testing flag set when #BuffersWaitingForProcLoop executes
        bool m_bBuffersWaitingForProcLoopCalled;

        /// Testing flag set when #HandleProcLoopData executes
        bool m_bHandleProcLoopDataCalled;

        /// For testing: Base class implementations of callbacks sets
        /// this to point to a static string that indicates which callback
        /// executed.
        const char * m_lpcszTestingLastCallback;

        /// For testing: Last xrun type
        XrunType m_xtTestingLastXrunType;

        /// Testing flag for testing BufferSizes (to force particular value to be
        /// returned by AsioBufsizeQuery)
        size_t m_nTestAsioBufsizeQuery;

    private:
        /// Result of calling (AsioOutputReady()==ASE_OK) during driver
        /// initialization is stored here. If it is true, then it means that
        /// the driver can save 1 block delay if the application calls
        /// ASIOOutputReady() again after every filling of the output buffers.
        /// See ASIO SDK documentation on ASIOOutputReady() for details.
        bool m_bPostOutput;

        /// Set to true by #DoneMain() immediately before it calls #DoneLoop().
        /// Set to false by #DoneMain() immediately after #DoneLoop() finishes
        /// execution. Used as result by the #IsDoneLoopActive() method. Used
        /// by SoundDataExchanger::ClearQueues to ensure that the DoneLoop
        /// is active, so waiting for its effect will terminate eventually.
        bool m_bDoneLoopActive;

        /// Set to true when the DoneLoop is entered. The DoneLoop will call
        /// WaitForDoneData if (1) there is currently no data in the "done"
        /// queues of the associated SoundDataExchanger, AND (2) this flag is
        /// true.
        /// WaitForDoneData will set this to false when it detects the DONE_STOP
        /// event. This event is set just before the end of the stop procedure,
        /// when no more new data for the "done" thread will be produced.
        /// Instead of calling WaitForData, DoneLoop will exit if there is
        /// no more data in the "done" queues and this flag is false.
        bool m_bDoneLoopWaitsWhenDoneQueuesEmpty;

        /// Set to true when WaitForDoneData is entered, and to false when it
        /// returns. Used only for testing.
        bool m_bWaitForDoneDataActive;

        // ====================================================================
        // global (static member variables) counters for testing.
        // These counters are incremented and decremented in
        // production code, but are never checked in production code.
        // access to the counters is not thread-safe, since
        // thread-safe access is not needed for the unit tests.

        /// Number of objects allocated.
        static int sm_nObjects;

        /// Number of Win32 Events allocated.
        static int sm_nEvents;

        /// Number of Win32 Critical sections allocated.
        static int sm_nCriticalSections;

        /// Number of threads created.
        static int sm_nThreads;

        /// LONG array for testing: These signal which of the thread
        /// start functions are currently executing.
        static ::LONG sm_anThreadStartFunctionFlags[CASIO_THREADS];

        /// LONG array for testing: These bits signal which of the the
        /// thread main functions are currently executing.
        static ::LONG sm_anThreadMainFunctionFlags[CASIO_THREADS];



        /// global enum for testing. Determines that an exception should be
        /// thrown without cause.
        /// During normal operation, this should always be set to
        /// NORMAL_OPERATION.
        /// This default value is set when this module loads.
        static enum ExceptionTrigger {
            /// No exceptions raised for testing purposes
            NORMAL_OPERATION,
            /// Constructor throws EWin32Error (Checking resource freeing)
            CONSTRUCTOR_THROWS_WIN32_ERROR,
            /// Method NumDrivers raises ENegativeCountError
            NUM_DRIVERS_THROWS_NEGATIVE_COUNT_ERROR,
            /// Method DriverNameAtIndex throws EUnexpectedError
            DRIVER_NAME_AT_INDEX_THROWS_UNEXPECTED_ERROR,
            /// Method LoadDriver throws ELoadError instead of loading driver
            LOAD_DRIVER_THROWS_LOAD_ERROR_LD,
            /// Method LoadDriver throws EVersionError after loading driver
            LOAD_DRIVER_THROWS_VERSION_ERROR,
            /// LoadDriver throws ELoadError as if load caused ASE_NotPresent
            LOAD_DRIVER_THROWS_LOAD_ERROR_NP,
            /// LoadDriver throws ELoadError as if load caused ASE_HWMalfunc
            LOAD_DRIVER_THROWS_LOAD_ERROR_HW,
            /// LoadDriver throws ENoMemoryError
            LOAD_DRIVER_THROWS_NO_MEMORY_ERROR,
            /// LoadDriver throws EUnexpectedError
            LOAD_DRIVER_THROWS_UNEXPECTED_ERROR,
            /// HardwareChannels throws EUnecpectedError
            HARDWARE_CHANNELS_THROWS_NEGATIVE_COUNT_ERROR,
            LATENCY_THROWS_UNAVAILABLE_ERROR,
            LATENCY_THROWS_UNEXPECTED_ERROR,
            CAN_SAMPLE_RATE_THROWS_UNAVAILABLE_ERROR,
            CAN_SAMPLE_RATE_THROWS_UNEXPECTED_ERROR,
            SAMPLE_RATE_THROWS_UNAVAILABLE_ERROR,
            SAMPLE_RATE_THROWS_UNEXPECTED_ERROR,
            SAMPLE_RATE_THROWS_NO_CLOCK_ERROR,
            SAMPLE_RATE_THROWS_MODE_ERROR,
            GET_CLOCK_SOURCES_THROWS_UNAVAILABLE_ERROR,
            GET_CLOCK_SOURCES_THROWS_UNEXPECTED_ERROR_CODE,
            GET_CLOCK_SOURCES_THROWS_NEGATICE_COUNT_ERROR,
            GET_CLOCK_SOURCES_THROWS_LIMIT_EXCEEDED_ERROR,
            GET_CLOCK_SOURCES_THROWS_UNEXPECTED_ERROR_NUMBERING,
            CLOCK_SOURCE_CURRENT_THROWS_UNEXPECTED_ERROR,
            CLOCK_SOURCE_SET_TRIGGERS_MODE_ERROR,
            CLOCK_SOURCE_SET_TRIGGERS_UNAVAILABLE_ERROR,
            CLOCK_SOURCE_SET_TRIGGERS_UNEXPECTED_ERROR,
            GET_CHANNEL_INFO_THROWS_UNAVAILABLE_ERROR,
            GET_CHANNEL_INFO_THROWS_UNEXPECTED_ERROR,
            CREATE_BUFFERS_THROWS_INCONSISTENT_STATE_ERROR,
            CREATE_BUFFERS_THROWS_NO_MEMORY_ERROR,
            CREATE_BUFFERS_THROWS_UNAVAILABLE_ERROR,
            /// In CreateBuffers, Asio driver returns "invalid mode" although
            /// parameters are checked and ok.
            CREATE_BUFFERS_THROWS_UNEXPECTED_ERROR_MODE,
            CREATE_BUFFERS_THROWS_UNEXPECTED_ERROR,
            /// DisposeBuffers triggers only warnings, not exceptions.
            DISPOSE_BUFFERS_TRIGGERS_UNAVAILABLE_WARNING,
            DISPOSE_BUFFERS_TRIGGERS_MODE_WARNING,
            DISPOSE_BUFFERS_TRIGGERS_UNEXPECTED_WARNING,
            START_THROWS_UNAVAILABLE_ERROR,
            /// simulate ASIOStart returned ASE_HWMalfunction
            START_THROWS_UNAVAILABLE_ERROR_HW,
            START_THROWS_UNEXPECTED_ERROR,
            /// simulate event creation failure at first event
            INIT_EVENTS_THROWS_WIN32_ERROR_IN_LOOP,
            /// simulate event creation failure at last event
            INIT_EVENTS_THROWS_WIN32_ERROR_AFTER_LOOP,
            START_THREADS_THROWS_WIN32_ERROR,
            ASIO_BUFSIZE_QUERY_THROWS_UNAVAILABLE_ERROR,
            ASIO_BUFSIZE_QUERY_THROWS_UNEXPECTED_ERROR,
            SIGNAL_BUFFER_SWITCH_SIMULATES_FAILURE_TO_AQUIRE_CRITICAL_SECTION
        } sm_etExceptionTrigger;
    };
}
#endif


// Next comment block tells emacs editor how to format code in this file.

// Local Variables:
// mode: c++
// c-file-style: "stroustrup"
// c-basic-offset: 4
// indent-tabs-mode: nil
// End:
