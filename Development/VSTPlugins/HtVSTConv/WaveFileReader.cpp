//------------------------------------------------------------------------------
/// \file AHtVSTConv.cpp
/// \author Berg
/// \brief Implementation of class CWaveFileReader for reading normalized 32bit PCM wave files.
///
/// Project SoundMexPro
/// Module  HtVSTConv.dll
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
#pragma hdrstop

#include "WaveFileReader.h"
#include <mmsystem.h>
#include <windowsx.h>
//------------------------------------------------------------------------------
#pragma package(smart_init)
//------------------------------------------------------------------------------

//------------------------------------------------------------------------------
/// constructor
//------------------------------------------------------------------------------
CWaveFileReader::CWaveFileReader()
   : m_bLoaded(false)
{
   m_lpData[0] = NULL;
   m_lpData[1] = NULL;

}
//------------------------------------------------------------------------------

//------------------------------------------------------------------------------
/// constructor with filename, calls Load() directly
//------------------------------------------------------------------------------
CWaveFileReader::CWaveFileReader(LPSTR lpszFileName)
   : m_bLoaded(false)
{
   m_lpData[0] = NULL;
   m_lpData[1] = NULL;
   Load(lpszFileName);
}
//------------------------------------------------------------------------------

//------------------------------------------------------------------------------
/// destructor, calls Cleanup()
//------------------------------------------------------------------------------
CWaveFileReader::~CWaveFileReader()
{
   Cleanup();
}
//------------------------------------------------------------------------------

//------------------------------------------------------------------------------
/// cleans up data arrays
//------------------------------------------------------------------------------
void CWaveFileReader::Cleanup()
{
   if (m_lpData[1])
	  {
	  GlobalFreePtr(m_lpData[1]);
	  m_lpData[1] = NULL;
	  }
   if (m_lpData[0])
	  {
	  GlobalFreePtr(m_lpData[0]);
	  m_lpData[0] = NULL;
	  }
}
//------------------------------------------------------------------------------

//------------------------------------------------------------------------------
/// loads mono or stereo 32bit normalized float PCM wavfile
//------------------------------------------------------------------------------
void CWaveFileReader::Load(LPSTR lpszFileName)
{
   Cleanup();
   HMMIO hmmio          = NULL;
   LPSTR lpszFmtBuffer  = NULL;
   float *lpTmpData     = NULL;
   try
      {
      try
         {
         // open wave file
         MMCKINFO            mmParent, mmSub;
         hmmio = mmioOpen(lpszFileName,NULL,MMIO_READ);

         if (!hmmio)
            throw ("error opening file");

         // test "WAVE" chunk
         mmParent.fccType = mmioFOURCC('W','A','V','E');
         if (mmioDescend(hmmio,(LPMMCKINFO)&mmParent,NULL,MMIO_FINDRIFF))
            throw ("not a wave (WAVE missing)");

         // test "fmt " chunk
         mmSub.ckid = mmioFOURCC('f','m','t',' ');
         if (mmioDescend(hmmio,(LPMMCKINFO)&mmSub,(LPMMCKINFO)&mmParent,MMIO_FINDCHUNK))
            throw ("not a wave (fmt missing)");

         lpszFmtBuffer = (LPSTR)malloc((size_t)mmSub.cksize);
         if ( NULL==lpszFmtBuffer )
            throw ("OOM");

         if ( (LONG)mmSub.cksize!=mmioRead(hmmio, lpszFmtBuffer, (LONG)mmSub.cksize) )
            {
            throw ("error in temporary file: error reading format chunk");
            }

         // at the moment only 32bit normalized float supported
         LPPCMWAVEFORMAT lppcmwf = (LPPCMWAVEFORMAT)lpszFmtBuffer;
         if  (   mmSub.cksize<sizeof(*lppcmwf)
             || lppcmwf->wf.wFormatTag    != 3
             || lppcmwf->wBitsPerSample   != 32
             || (lppcmwf->wf.nChannels     != 2 && lppcmwf->wf.nChannels     != 1)
            )
            {
            free(lpszFmtBuffer);
            lpszFmtBuffer = NULL;
            throw ("Invalid wave format of impulse response (only normalized 32bit stereo supported)");
            }
         int iBitsPerSample   = lppcmwf->wBitsPerSample;
         int iChannels        = lppcmwf->wf.nChannels;

         // ascend up again
         if  (mmioAscend(hmmio,&mmSub,0))
            throw ("Error in mmioAscend");

         // descend to 'data' chunk
         mmSub.ckid = mmioFOURCC('d','a','t','a');
         if (mmioDescend(hmmio,(LPMMCKINFO)&mmSub,(LPMMCKINFO)&mmParent,MMIO_FINDCHUNK))
            throw ("error in temporary file: error reading data chunk");

         m_nSize = ((DWORD)mmSub.cksize / ((unsigned int)iBitsPerSample/8)) / (unsigned int)iChannels;

         // create data array for complete file
         lpTmpData = (float*)GlobalAllocPtr(GMEM_FIXED, m_nSize*sizeof(float)*(unsigned int)iChannels);
         // create channel data arrays
         m_lpData[0] = (float*)GlobalAllocPtr(GMEM_FIXED, m_nSize*sizeof(float));
         m_lpData[1] = (float*)GlobalAllocPtr(GMEM_FIXED, m_nSize*sizeof(float));
         if (!lpTmpData || !m_lpData[0] || !m_lpData[1])
            throw ("OOM");

         // read complete file
         if (mmioRead(hmmio, (char*)lpTmpData, (LONG)mmSub.cksize) != (int)mmSub.cksize)
            throw ("file reading error");

         if (iChannels == 2)
            {
            // move data to internal channel data blocks
            for (unsigned int i = 0; i < m_nSize; i++)
               {
               m_lpData[0][i] = lpTmpData[2*i];
               m_lpData[1][i] = lpTmpData[2*i+1];
               }
            }
         else
            {
            for (unsigned int i = 0; i < m_nSize; i++)
               {
               m_lpData[0][i] = lpTmpData[i];
               m_lpData[1][i] = lpTmpData[i];
               }

            }
         }
      __finally
         {
         if (hmmio)
            {
            mmioClose(hmmio, 0);
            hmmio = NULL;
            }
         if (lpszFmtBuffer)
            {
            free(lpszFmtBuffer);
            lpszFmtBuffer = NULL;
         }
         if (lpTmpData)
            {
            GlobalFreePtr(lpTmpData);
            lpTmpData = NULL;
            }
         }
      m_bLoaded = true;
      }
   catch (...)
      {
      Cleanup();
      throw;
      }
}
//------------------------------------------------------------------------------


