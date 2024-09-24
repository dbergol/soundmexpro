//---------------------------------------------------------------------------
//---------------------------------------------------------------------------
//       Main Routines for MMTools-Overlapped-Add-Component
//
//                         by Daniel Berg
//
//
//---------------------------------------------------------------------------


#pragma hdrstop
#include <math.h>
#include <limits.h>
#include "floatolaflt.h"

//---------------------------------------------------------------------------

int ConvertToPowerOfTwo(int n, int* pnOrder)
{
   int nOrder = 0;
   while (n > 1)
      {
      n = n >> 1;
      nOrder++;
      }
   if (nOrder > 0)
      n = n << nOrder;
   if (pnOrder)
      *pnOrder = nOrder;
   return n;
}

//---------------------------------------------------------------------------
// SOME DEBUG ROUTINES
//---------------------------------------------------------------------------
void OutputDebugStringTag(int Tag, AnsiString s)
{
OutputDebugStringW((IntToStr(Tag) + ": " + s).c_str());
}
//---------------------------------------------------------------------------
void OutputDebugStringFilterTag(PFloatOlaFlt &pflt, AnsiString s)
{
int i = -1;
if (pflt != NULL)
   i = pflt->Tag;
OutputDebugStringTag(i,s);
}
//---------------------------------------------------------------------------
void OutputDebugStringFilterTagBufferNum(PFloatOlaFlt &pflt, AnsiString s)
{
if (pflt != NULL)
   s = "Buf" + IntToStr(pflt->iBuffersProcessed) + ": " + s;

OutputDebugStringFilterTag(pflt,s);
}
//---------------------------------------------------------------------------

//---------------------------------------------------------------------------
// inteface to ClcWindowFunc from MMTOOLS
//---------------------------------------------------------------------------
void GenerateWindow(float* pTable, int Window, int Len)
{
   for (int i = 0; i < Len; i++)
      *pTable++ = CalcWindowFunc(Window,i,Len);
}
//---------------------------------------------------------------------------
//---------------------------------------------------------------------------
// inteface to ClcWindowFunc from MMTOOLS
//---------------------------------------------------------------------------
void GenerateWindow(float* pTable, int Window, int Len, float f4Scale)
{
   for (int i = 0; i < Len; i++)
      *pTable++ = CalcWindowFunc(Window,i,Len)*f4Scale;
}
//---------------------------------------------------------------------------

//---------------------------------------------------------------------------
//Main Initialization routine for overlapped add filter Mmfloatolaflt
//---------------------------------------------------------------------------
PFloatOlaFlt __fastcall Mmfloatolaflt::InitFloatOlaFlt(  unsigned short nChannels,
                                                         int FFTLength,
                                                         int WindowLength,
                                                         TMMFloatOlaWindowFeed WindowFeed,
                                                         int MaxBufSize,
                                                         int Tag
                                                         )
{
PFloatOlaFlt pflt;

pflt = (PFloatOlaFlt)GlobalAllocMem(sizeof(TFloatOlaFlt));
if (pflt != NULL)
   {
   // Convert FFTLen to a power of 2 }
   int nOrder;
   FFTLength = ConvertToPowerOfTwo(FFTLength, &nOrder);

   pflt->iNumRealChannels     = nChannels;
   pflt->Tag                  = Tag;
   pflt->FSpectrumWaveCallBack = NULL;
   pflt->FWavePreSpecCallBack = NULL;
   pflt->FWavePostSpecCallBack= NULL;
   pflt->FFTLen            = FFTLength;
   pflt->TotalFFTLen       = 2 * FFTLength;
   pflt->FFTLen_2          = FFTLength / 2;
   pflt->WindowLength      = WindowLength;
   pflt->FFFTEnabled       = true;

   //determine Numner of samples we have to shift after each frame
   int iWindowFeed;
   switch (WindowFeed)
      {
      case wffHalf:      iWindowFeed = 2; break;
      case wffQuarter:   iWindowFeed = 4; break;
      case wffEighth:    iWindowFeed = 8; break;
      }
   pflt->WindowShift       = pflt->WindowLength / iWindowFeed;


   pflt->TotalLeadingZeros     = pflt->FFTLen - pflt->WindowLength;

   //we need 2 buffers for input overlap...
   pflt->IOBufferSize     = 2*MaxBufSize;
   //we need 2 buffers for input overlap...
   pflt->BufIn    = (char*)GlobalAllocMem(pflt->IOBufferSize*sizeof(char));
   //... and buffers for output overlap
   pflt->BufOut   = (char*)GlobalAllocMem(pflt->IOBufferSize*sizeof(char));

   ZeroMemory(&pflt->DataSection, sizeof(pflt->DataSection));
   InitializeCriticalSection(&pflt->DataSection);


   //calculate the scalingfactor for the spectrum:
   // - respect to zero-padding: scale intensity by wndlen/fftlen
   // - windowing: sum(W^2)/wndlen = 0.375 for hanning window
   // - adjusting to power density rather than power: scale intensity by fftlen
   // - scale by sqrt(N) for FFT
   // - another sqrt(2) to keep intensity rather than amplitude
   // this leads to a total factor of
   //     sqrt(0.375*wndlen*fftlen)
   float f4SpectrumScaling = sqrt(0.375*0.5*(float)pflt->WindowLength*(float)pflt->FFTLen);

   //for FFT we apply this factor by deviding the window function by this factor!
   // 1. allocate buffer for window function for Pre-Windowing: only WindowLength samples
   pflt->FPreWinBuf  = (float*)GlobalAllocMem(WindowLength*sizeof(float));
   // 2. generate hanning window and apply scaling factor
   GenerateWindow(   pflt->FPreWinBuf,
                     fwHanning,
                     WindowLength,
                     1.0/f4SpectrumScaling);

   // for IFFT we have to scale back _and_ we have to take care for WindowFeed's other than
   // wfHalf. The Hanning window will keep energy the same for wfHalf but for higher WindowFeeds we
   // have to devide the wave data by WindowFeeds/2 !!! So we calculate the correction value here
   // WindowFeed is
   //   pflt->WindowFeed       = pflt->WindowLength / pflt->WindowShift;
   // so the total IFFT factor is
   pflt->f4IFFTScalingFactor = 2.0/(float)(pflt->WindowLength / pflt->WindowShift) * f4SpectrumScaling;


   //buffer for window function for Post-Windowing: we need only
   //the real window for the patched zeros!!
   if (pflt->TotalLeadingZeros != 0)
      {
      pflt->FPostWinBuf = (float*)GlobalAllocMem(pflt->TotalLeadingZeros*sizeof(float));
      GenerateWindow(pflt->FPostWinBuf,fwHanning ,pflt->TotalLeadingZeros);
      }


   pflt->FFT_Buf = (float*)GlobalAllocMem(pflt->FFTLen*sizeof(float)*2);
   //Really overlapping are Samples
   pflt->TotalOverlapAddSamples  = 2*(pflt->TotalLeadingZeros + pflt->WindowLength - pflt->WindowShift);
   //alloc always for 2 channels!!!
   pflt->Overlap_Buf = (float*)GlobalAllocMem(pflt->TotalOverlapAddSamples * sizeof(float));
   pflt->WaveCallback_Buf = (char*)GlobalAllocMem(pflt->WindowShift*pflt->iNumRealChannels * sizeof(float));

   pflt->pfft   = InitCplxFFT(nOrder);

   pflt->pfxSortedSpectrum = (TfCplx*)GlobalAllocMem(pflt->FFTLen_2*2* sizeof(TfCplx));
   pflt->pfxLeft  = &pflt->pfxSortedSpectrum[0];
   pflt->pfxRight = &pflt->pfxSortedSpectrum[pflt->FFTLen_2];

   ResetFloatOlaFlt(pflt);
   }
return pflt;

}
//---------------------------------------------------------------------------

//---------------------------------------------------------------------------
//---------------------------------------------------------------------------
void __fastcall Mmfloatolaflt::ResetFloatOlaFlt(PFloatOlaFlt pflt)
{
   pflt->BufIn_Bytes       = 0;
   // IMPORTANT NOTE: preload output buffers with zeros with the expected total delay, i.e.
   // WinowLength + half of padded zeros!
   pflt->BufOut_Bytes =    (pflt->WindowLength + pflt->TotalLeadingZeros/2)*2*sizeof(float);

   pflt->iBuffersProcessed = 0;
   ZeroMemory(pflt->FFT_Buf, pflt->FFTLen*sizeof(float)*2);
   ZeroMemory(pflt->Overlap_Buf, pflt->TotalOverlapAddSamples * sizeof(float));
   ZeroMemory(&pflt->pfxSortedSpectrum[0], pflt->FFTLen_2*2* sizeof(TfCplx));

   ZeroMemory(pflt->BufIn, pflt->IOBufferSize*sizeof(char));
   ZeroMemory(pflt->BufOut, pflt->IOBufferSize*sizeof(char));

}
//---------------------------------------------------------------------------


//---------------------------------------------------------------------------
//---------------------------------------------------------------------------
void __fastcall Mmfloatolaflt::DoneFloatOlaFlt(PFloatOlaFlt &pflt)
{
if (pflt != NULL)
   {

   DeleteCriticalSection(&pflt->DataSection);

   if (pflt->pfft != NULL)
      DoneCplxFFT(pflt->pfft);

   GlobalFreeMem((void*)pflt->BufIn);
   GlobalFreeMem((void*)pflt->BufOut);
   GlobalFreeMem((void*)pflt->FPreWinBuf);
   GlobalFreeMem((void*)pflt->FPostWinBuf);
   GlobalFreeMem((void*)pflt->FFT_Buf);
   GlobalFreeMem((void*)pflt->Overlap_Buf);
   GlobalFreeMem((void*)pflt->WaveCallback_Buf);
   GlobalFreeMem((void*)pflt->pfxSortedSpectrum);
   GlobalFreeMem((void*)pflt);
   }
}
//---------------------------------------------------------------------------


//---------------------------------------------------------------------------
//---------------------------------------------------------------------------
void __fastcall Mmfloatolaflt::GetSignal(PFloatOlaFlt pflt, char *pIn, float *pOut)
{
//access window
float *lpf4Window = pflt->FPreWinBuf;
float * lpf4 = (float*)pIn;


//here we fill mono signals into the left channel of the internal
//FFT-Buffer and write zeroes to the right channel

if (pflt->iNumRealChannels == 1)
   {
   for (int i = 0; i < pflt->WindowLength; i++)
      {
      //write one value multiplicated with the window function
      //left
      *pOut++ = (float)*lpf4++ * *lpf4Window++;
      //and right channel ZERO
      *pOut++ = 0.0;
      }
   }
else
   {
   for (int i = 0; i < pflt->WindowLength; i++)
      {
      //write one value multiplicated with the window function
      //left
      *pOut++ = (float)*lpf4++ * *lpf4Window;
      //and right channel
      *pOut++ = (float)*lpf4++ * *lpf4Window;
      //proceed window after writing _both_ channels!
      lpf4Window++;
      }
   }
}
//---------------------------------------------------------------------------

//---------------------------------------------------------------------------
//---------------------------------------------------------------------------
void __fastcall Mmfloatolaflt::SetSignal(PFloatOlaFlt pflt, float *pIn, char *pOut)
{
   float *lpf4Out = (float*)&pOut[0];
   float *lpf4In = (float*)&pIn[0];

   for (int i = 0; i < 2*pflt->WindowShift; i++)
      {
      *lpf4Out++ = *lpf4In++*pflt->f4IFFTScalingFactor;


      if (pflt->iNumRealChannels == 1)
         lpf4In++;
      }
}
//---------------------------------------------------------------------------


//---------------------------------------------------------------------------
//---------------------------------------------------------------------------
void __fastcall Mmfloatolaflt::DoFloatOlaFlt(PFloatOlaFlt pflt, char * pIn, DWORD *lpdwLength)
{

#ifdef DEBUG_CHECKBUFFERLENGTH
if (pflt->BufIn_Bytes + *lpdwLength > (UINT)pflt->IOBufferSize)
   {
   OutputDebugStringFilterTagBufferNum(pflt, "Try to write behind outpuffer end!");
   return;
   throw Exception("Try to write behind outpuffer end of FloatOLA!");
   }
#endif

if (pflt != NULL)
   {
   //copy complete input data to internal buffer at actual position
   GlobalMoveMem(pIn,&(pflt->BufIn[pflt->BufIn_Bytes]),*lpdwLength);
   ZeroMemory(pIn, *lpdwLength);

   //adjust number of bytes in inbuffer
   pflt->BufIn_Bytes += *lpdwLength;

   //number of total samples in inbuffer
   int iTmp = pflt->BufIn_Bytes / (pflt->iNumRealChannels*sizeof(float));
   //determine number of fft intervals in the inbuffer
   int iNumInputFFTs = 0;
   if (iTmp >= pflt->WindowLength) //??
      iNumInputFFTs = (iTmp-pflt->WindowLength) / pflt->WindowShift + 1;

   //----------------------------------------------------------------------------------------------------------------------------------------
   // iteration through buffers!
   // - write zeros and data*windowfunction as right and left samples alternating to FFT_Buf
   // - hand this buffer as ComplexArry (re, im, re, im...) to ComplexFFT
   // - extract spectra from complex fft and hand them to spectrum callback
   // - rebuild complex spectrum and hand it to complex IFFT
   // - apply window to zeros and do OLA
   // - hand wave data to wave callback
   // - write reconstructed data back
   //----------------------------------------------------------------------------------------------------------------------------------------

   for (int iNumIter = 0; iNumIter < iNumInputFFTs; iNumIter++)
      {
      //******************** CALLBACK 1: WAVE BEFORE FFT*************************


      //call callback funtion: we are allowed to process one buffer!
      if (pflt->FWavePreSpecCallBack != NULL)
         {
         // hand the last 'WindowShift' samples to the Callback
         pflt->FWavePreSpecCallBack(
            (float*)&pflt->BufIn[(pflt->WindowShift*(iNumIter-1) + pflt->WindowLength)*sizeof(float)*pflt->iNumRealChannels ],
            pflt->WindowShift*pflt->iNumRealChannels
            );
         }
      //******************** END CALLBACK 1: WAVE BEFORE FFT*********************


      //prepare data: fill zeros in the beginning of the buffer
      ZeroMemory(pflt->FFT_Buf,pflt->TotalLeadingZeros*sizeof(float));
      //HALLO
      GlobalMoveMem(&pflt->BufIn[(pflt->WindowShift*(iNumIter-1) + pflt->WindowLength)*sizeof(float)*pflt->iNumRealChannels ],
                     &pflt->WaveCallback_Buf[0],
                     pflt->WindowShift*pflt->iNumRealChannels * sizeof(float)
                     );
      //only do FFT if we need to!
      if (pflt->FFFTEnabled)
         {
         //write signal data behind the zeros
         GetSignal(  pflt,
                     //we access the data as char...
                     (char*)&pflt->BufIn[pflt->WindowShift*iNumIter*sizeof(float)*pflt->iNumRealChannels],
                     //and write them as float!!
                     &pflt->FFT_Buf[pflt->TotalLeadingZeros]
                  );

         //prepare data: fill zeros at the end of the buffer
         ZeroMemory(&pflt->FFT_Buf[pflt->TotalLeadingZeros + 2*pflt->WindowLength], pflt->TotalLeadingZeros*sizeof(float));

         //here we have to do the main thing...
         EnterCriticalSection(&pflt->DataSection);

         //************************************************************
         DoCplxFFTb(pflt->pfft,(TfCplxArray*)pflt->FFT_Buf,1);

         //write channel 0
         pflt->pfxLeft[0].re   = pflt->FFT_Buf[0];
         pflt->pfxLeft[0].im   = 0;
         pflt->pfxRight[0].re  = pflt->FFT_Buf[1];
         pflt->pfxRight[0].im  = 0;

         //extract left and right spectrum
         for (int i = 1; i < pflt->FFTLen_2; i++)
            {
            //Left.Re            = (Re(Y(f))           + Re(Y(-f)))                         / 2
            pflt->pfxLeft[i].re   = (pflt->FFT_Buf[2*i] + pflt->FFT_Buf[pflt->TotalFFTLen - 2*i]) / 2;
            //Left.Im            = (Im(Y(f))               - Im(Y(-f)))                             / 2
            pflt->pfxLeft[i].im   = (pflt->FFT_Buf[2*i + 1] - pflt->FFT_Buf[pflt->TotalFFTLen - 2*i + 1]) / 2;
            //Right.Re           = (Im(Y(f))               + Im(Y(-f)))                             / 2
            pflt->pfxRight[i].re  = (pflt->FFT_Buf[2*i + 1] + pflt->FFT_Buf[pflt->TotalFFTLen - 2*i + 1]) / 2;
            //Right.Im           = (Re(Y(-f))                         - Re(Y(f)))           / 2
            pflt->pfxRight[i].im  = (- pflt->FFT_Buf[2*i] + pflt->FFT_Buf[pflt->TotalFFTLen - 2*i]) / 2;
            }

         ZeroMemory(pflt->FFT_Buf, pflt->TotalFFTLen*sizeof(float));
      } //FFTEnabled


      bool bUseWave = false;
      if (pflt->FSpectrumWaveCallBack != NULL)
         {
         pflt->FSpectrumWaveCallBack(  &pflt->pfxSortedSpectrum[0],
                                       pflt->FFTLen_2 * pflt->FFFTEnabled *pflt->iNumRealChannels,
                                       (float*)&pflt->WaveCallback_Buf[0],
                                       pflt->WindowShift*pflt->iNumRealChannels,
                                       bUseWave
                                       );

         }

      if (pflt->FFFTEnabled)
         {
         //write channel 0
         pflt->FFT_Buf[0]                       = pflt->pfxLeft[0].re - pflt->pfxRight[0].im;
         pflt->FFT_Buf[1]                       = pflt->pfxLeft[0].im + pflt->pfxRight[0].re;

         //rebuild complex spectrum
         for (int i = 1; i < pflt->FFTLen_2; i++)
            {
            //Re(Z(f))                             = Left.re            - Right.im
            pflt->FFT_Buf[2*i]                     = pflt->pfxLeft[i].re - pflt->pfxRight[i].im;
            //Re(Z(-f))                            = Left.re            + Right.im
            pflt->FFT_Buf[pflt->TotalFFTLen - 2*i] = pflt->pfxLeft[i].re + pflt->pfxRight[i].im;
            //Im(Z(f))                             = Left.im            + Right.re
            pflt->FFT_Buf[2*i + 1]                 = pflt->pfxLeft[i].im + pflt->pfxRight[i].re;
            //Im(Z(-f))                            = Right.re           - Left.im
            pflt->FFT_Buf[pflt->TotalFFTLen - 2*i + 1]  = pflt->pfxRight[i].re - pflt->pfxLeft[i].im;
            }


         //IFFT
         DoCplxFFTb(pflt->pfft,(TfCplxArray*)pflt->FFT_Buf,-1);
         //************************************************************

         LeaveCriticalSection(&pflt->DataSection);


         //no we apply the back window for the zero patched areas
         if (pflt->TotalLeadingZeros != 0)
            {
            float *fWin          = (float*)pflt->FPostWinBuf;
            float *fDataStart    = (float*)pflt->FFT_Buf;
            float *fDataStop     = (float*)&pflt->FFT_Buf[pflt->TotalFFTLen-1];
            for (int i = 0; i < pflt->TotalLeadingZeros/2; i++)
               {
               //apply window to the beginning...
               *fDataStart++ *= *fWin;
               // _and_ the end of the data!!
               *fDataStop-- *= *fWin;

               //and now the other channel!
               //apply window to the beginning...
               *fDataStart++ *= *fWin;
               // _and_ the end of the data!!
               *fDataStop-- *= *fWin;

               //proceed window after writing _both_ channels!
               fWin++;
               }
            }

         //here we do the overlapped add!!
         for (int i = 0; i < pflt->TotalOverlapAddSamples; i++)
            pflt->FFT_Buf[i] += pflt->Overlap_Buf[i];

         //shift the old buffer one back (only the pflt->OverlapAddSamples number of samples)!
         GlobalMoveMem( &pflt->FFT_Buf[pflt->TotalFFTLen - pflt->TotalOverlapAddSamples],
                        pflt->Overlap_Buf,
                        pflt->TotalOverlapAddSamples * sizeof(float));

         #ifdef DEBUG_CHECKBUFFERLENGTH
         if (pflt->BufOut_Bytes +  pflt->WindowShift*pflt->iNumRealChannels * sizeof(float) > (UINT)pflt->IOBufferSize)
            {
            OutputDebugStringFilterTagBufferNum(pflt, "Try to write behind outpuffer end!");
            throw Exception("Try to write behind outpuffer end of FloatOLA!");
            }
         #endif

         //here we decide if the wave data from the reconstruction (IFFT)
         //or the directly modified time signal is written back.
         if (!bUseWave)
            {
            //and finally we write the new buffer to main out buffer
            SetSignal(  pflt,
                        //we read the data as float...
                        pflt->FFT_Buf,
                        //and write them as float!!
                        &pflt->BufOut[pflt->BufOut_Bytes + pflt->WindowShift*iNumIter*sizeof(float)*pflt->iNumRealChannels]
                     );
            }
         }//FFFTEnabled
         if (bUseWave || !pflt->FFFTEnabled)
            {
            //we have handed the pointer to BufIn do SpecWaveCallback. If
            //we are instructed to write the wave back, then we copy the modified data to
            //bufout!!
            GlobalMoveMem(
                           &pflt->WaveCallback_Buf[0],
                           &pflt->BufOut[pflt->BufOut_Bytes + pflt->WindowShift*iNumIter*sizeof(float)*pflt->iNumRealChannels],
                           pflt->WindowShift*pflt->iNumRealChannels * sizeof(float)
                           );
            }


      //call callback function: we are allowed to process one buffer!
      if (pflt->FWavePostSpecCallBack != NULL)
         pflt->FWavePostSpecCallBack((float*)&pflt->BufOut[pflt->BufOut_Bytes + pflt->WindowShift*iNumIter*sizeof(float)*pflt->iNumRealChannels],
                                       pflt->WindowShift*pflt->iNumRealChannels
                                       );
      }
//----------------------------------------------------------------------------------------------------------------------------------------


   //determine number of processed bytes
   int iBytesDone = iNumInputFFTs * sizeof(float)*pflt->iNumRealChannels*pflt->WindowShift;

   //adjust filled buffer sizes
   if (iBytesDone > 0)
      {
      pflt->BufOut_Bytes   += iBytesDone;
      pflt->BufIn_Bytes    -= iBytesDone;
      }

   //now set the really processed (==output) bytes
   //int nBytes = Min(*lpdwLength,pflt->BufOut_Bytes);
   //*lpdwLength = nBytes;

   if (*lpdwLength > (DWORD)pflt->BufOut_Bytes)
      *lpdwLength = pflt->BufOut_Bytes;

   GlobalMoveMem(pflt->BufOut, pIn ,*lpdwLength);

   //move remaining bytes to the beginning of the inbuffer
   if (iBytesDone > 0)
      {
      GlobalMoveMem(&pflt->BufIn[iBytesDone], pflt->BufIn ,pflt->BufIn_Bytes);
      }

   //move remaining output values to the beginning of the buffer
   pflt->BufOut_Bytes -= *lpdwLength;
   GlobalMoveMem(&pflt->BufOut[*lpdwLength],pflt->BufOut,pflt->BufOut_Bytes);

   }
pflt->iBuffersProcessed++;
}
//---------------------------------------------------------------------------



//---------------------------------------------------------------------------
void __fastcall Mmfloatolaflt::SetFloatOlaSpectrumWaveReady(PFloatOlaFlt pflt, TMMSpectrumWaveReady aValue)
{
pflt->FSpectrumWaveCallBack = aValue;
}
//---------------------------------------------------------------------------


//---------------------------------------------------------------------------
void __fastcall Mmfloatolaflt::SetFloatOlaWaveReadyPreSpec(PFloatOlaFlt pflt, TMMWaveReadyPreSpec aValue)
{
   pflt->FWavePreSpecCallBack = aValue;
}
//---------------------------------------------------------------------------

//---------------------------------------------------------------------------
void __fastcall Mmfloatolaflt::SetFloatOlaWaveReadyPostSpec(PFloatOlaFlt pflt, TMMWaveReadyPostSpec aValue)
{
   pflt->FWavePostSpecCallBack = aValue;
}
//---------------------------------------------------------------------------

void __fastcall Mmfloatolaflt::SetFloatOlaFFTEnabled(PFloatOlaFlt pflt, bool bFFTEnabled)
{
   pflt->FFFTEnabled = bFFTEnabled;
}

//---------------------------------------------------------------------------
//---------------------------------------------------------------------------
#pragma argsused
void __fastcall Mmfloatolaflt::SetFloatOlaFltWindow(PFloatOlaFlt pflt, int Window)
{

}
//---------------------------------------------------------------------------

//---------------------------------------------------------------------------
//---------------------------------------------------------------------------
#pragma package(smart_init)
