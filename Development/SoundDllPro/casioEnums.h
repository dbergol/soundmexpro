//------------------------------------------------------------------------------
/// \file casioEnums.h
/// \author Berg
/// \brief Definition of string arrays and enums used by class Casio
///
/// Project SoundMexPro
/// Module SoundDllPro
/// Definition of string arrays and enums used by class Casio
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
#ifndef casioEnumsH
#define casioEnumsH

namespace Asio {
    /// The major version of the ASIO SDK supported by this library.
    const long SUPPORTED_ASIO_VERSION = 2;

    /// \brief Data type for tracking state of asio driver.
    ///
    /// Variable prefix: "as" for Asio State.
    ///
    /// This types extends the states from ASIO SDK 2.1 documentation by
    /// the additional state FREE, which means no ASIO driver has been
    /// loaded yet, or the ASIO driver has been unloaded.
    enum State
    {
        /// No ASIO driver is loaded
        FREE = 0,
        /// Skipped in this class: The ASIO driver is initialized
        /// immediately after loading.
        LOADED = 1,
        /// Driver is initialized and can be queried for its capabilities.
        INITIALIZED = 2,
        /// Sound buffers are allocated for sound data exchange.
        PREPARED = 3,
        /// Sound processing is running.
        RUNNING = 4
    };

    /// \brief Direction of sound transport.
    ///
    /// Variable prefix: ad for Asio Direction.
    /// Specifies the direction of sound data transfer.
    enum Direction
    {
        /// Sound data transfer from audio hardware to application.
        INPUT = 0,

        /// Sound data transfer from application to audio hardware.
        OUTPUT = 1
    };
    
    /// location of xrun, used as parameter to CAsio::OnXrun method.
    enum XrunType
    {
        /// An xrun was detected in the processing queues that decouple
        /// sound processing/generation from the sound hardware.
        XR_PROC = 0,
        
        /// An xrun was detected in the "done" queues that decouple
        /// visualization / disk recording from the sound hardware.
        XR_DONE = 1,
        
        /// An xrun was detected in realtime processing mode
        XR_RT = 2
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
