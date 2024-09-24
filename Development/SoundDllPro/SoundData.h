//------------------------------------------------------------------------------
/// \file SoundData.h
/// \author Berg
/// \brief Interface of class SoundData (Multi-channel sound data buffer)
///
/// Project SoundMexPro
/// Module SoundDllPro
///
/// Interface of class SoundData (Multi-channel sound data buffer)
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
#ifndef SoundDataH
#define SoundDataH

#include <vector>
#include <valarray>

class UNIT_TEST_CLASS;

namespace Asio
{
    /// Multi-channel sound data buffer
    class SoundData
    {
    public:
        // Access to private data for testing.
        friend class UNIT_TEST_CLASS;

        /// Each float if a sample in a signal block and channel.
        /// First index specifies the channel, second index specifies
        /// the time index within the block.
        std::vector<std::valarray<float> > m_vvfData;

        /// A flag indicating that this multi-channel block is the last in a
        /// sequence of multiple instances. 
        bool m_bIsLast;

        /// Create an Array of SoundData structures, all initialized to the
        /// same channels and frames count.  The frames of the array members
        /// are filled with silence (0.0f), and the m_bIsLast member is set to
        /// false for all array members.  The Array can be deallocated with
        /// \sa DestroyArray.
        /// \param[in] nNumBuffers
        ///   The number of SoundData structure instances to create
        /// \param[in] nNumChannels
        ///   The number of sound channels to allocate for each of the array
        ///   members
        /// \param[in] nNumFrames
        ///   The number of sound frames in each channel
        /// \retval
        ///   A pointer to the first element in the newly created array
        static SoundData * CreateArray(size_t nNumBuffers,
                                       size_t nNumChannels,
                                       size_t nNumFrames);
        /// Destroy and deallocate an Array of sound data structures created
        /// with \sa CreateArray.
        /// \param[in] rgsdArray A pointer to the first element in the array,
        /// as returned by \sa CreateArray.
        static void DestroyArray(SoundData * rgsdArray);

        /// Overwrite all sound samples with zeros and clear the #m_bIsLast flag
        void Clear();

        /// Change the number of channels and frames of an instance.
        void Reinitialize(size_t nNumChannels, size_t nNumFrames);

        /// Copy sound data and the flag from given source.
        /// \param[in] sdSource
        /// Source for copying. Number of channels and frames have to match
        /// with this 
        void CopyFrom(const SoundData & sdSource); 
    private:
        /// field for testing: How many sound data arrays have been deleted by
        /// the DestroyArray static method. 
        static size_t sm_nArraysDeleted;
    };

} // namespace Asio

#endif // #ifndef SOUND_DATA_H

// Next comment block describes code layout for emacs editor:

// Local Variables:
// mode: c++
// c-file-style: "stroustrup"
// c-basic-offset: 4
// indent-tabs-mode: nil
// End:
