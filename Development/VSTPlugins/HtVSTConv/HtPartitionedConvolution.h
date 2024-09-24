//------------------------------------------------------------------------------
/// \file HtPartitionedConvolution.h
///
/// \author Berg
/// \brief Implementation of class CHtPartitionedConvolution for partitioned
/// convolution
///
/// Project SoundMexPro
/// Module  SoundDllPro.dll
/// Implementation of class CHtPartitionedConvolution for partitioned
/// convolution
///
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
#ifndef HtPartitionedConvolutionH
#define HtPartitionedConvolutionH
//--------------------------------------------------------------------------
#include "HtFFT3.h"
#include "HtTransferFunction.h"

   #define FFTERROR_STR_INT_INT(s, t1, t2)\
      {\
      AnsiString str;\
      str.printf(s, t1, t2);\
      throw Exception(str);\
      }


//--------------------------------------------------------------------------
/// structure for internal bookkeeping of channel indices and delays of
/// filter partitions, prefix fi
//--------------------------------------------------------------------------
struct FilterIndex
{
   unsigned int m_nChannelIndex;    /// channel index of a
   unsigned int m_nDelay;           /// delay (in blocks)
};
//--------------------------------------------------------------------------

//--------------------------------------------------------------------------
/// Class for multichannel partitioned convolution, prefix pc
//--------------------------------------------------------------------------
class CHtPartitionedConvolution
{
   public:
      CHtPartitionedConvolution( unsigned int nFragSize,
                                 unsigned int nChannels,
                                 const CHtTransferFunctions & tfs);
      ~CHtPartitionedConvolution();

      // different types of processing routines
      void Process(  float * const *   ppfSignalIn,
                     float * const *   ppfSignalOut,
                     unsigned int      nChannels,
                     unsigned int      nFrames);
      void Process(vvaf & vvafSignalIn, vvaf & vvafSignalOut);
      // inline processing
      void Process(vvaf & vvafSignal);
   private:

      unsigned int m_nFragSize;           /// Audio fragment size, always equal to partition size.
      unsigned int m_nChannels;           /// Number of audio channels.
      unsigned int m_nOutputPartitions;   /// The maximum number of partitions in any of the impulse responses.
                                          /// Determines the size if the delay line.
      unsigned int m_nFilterPartitions;   /// The total number of non-zero impulse response partitions.
      unsigned int m_nWaveInBufferHalfCurrentIndex;   /// A counter modulo 2. Indicates the buffer half in input
                                                      /// signal wave into which to copy the current input signal.

      vvaf   m_vvafWaveIn;                /// Buffer for input signal. Has m_nChannels channels and m_nFragSize*2 frames


      vvac m_vvacSpecIn;                  /// Buffer for FFT transformed input signal. Has m_nChannels channels
                                          /// and m_nFragSize+1 frames (fft bins).

      vvac   m_vvacFrequencyResponse;     /// Buffers for frequency response spectra of impulse response
                                          /// partitions.  Each "channel" contains another partition of
                                          /// some impulse response.  The array m_fiBookKeeping is used to
                                          /// keep track what to do with these frequency responses.
                                          /// This container has m_nFilterPartitions channels and m_nFragSize+1
                                          /// frames (fft bins).

      std::vector<FilterIndex> m_fiBookKeeping; /// Keeps track of channel index, and delay. The index into
                                                /// this array is the same as the "channel" index into the
                                                /// m_vvacFrequencyResponse array. Array has m_nFilterPartitions
                                                /// entries.

      std::vector<vvac> m_vvacSpecOut;    /// Buffers for FFT transformed output signal. For each array
                                          /// member, the number of channels is equal to m_nChannels,
                                          /// number of frames (fft bins) is equal to m_nFragSize+1.
                                          /// Array size is equal to m_nOutputPartitions.
      unsigned int m_nOutputPartitionCurrentIndex;    /// A counter modulo output_partitions, indexing the
                                                      /// "current" output partition.

      vvaf   m_vvafWaveOut;               /// Buffer for the wave output signal. Number of channels is equal
                                          /// to m_nChannels, number of frames is equal to m_nFragSize


      CHtFFT* m_pfft;                     /// CHtFFT instance

      // private processing routine
      void DoProcess();
};
//--------------------------------------------------------------------------
#endif // #ifndef HtPartitionedConvolutionH

