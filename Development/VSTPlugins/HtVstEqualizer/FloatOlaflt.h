//---------------------------------------------------------------------------

#ifndef floatolafltH
#define floatolafltH

#include <MMFFT.hpp>	// Pascal unit

#include <SysUtils.hpp>	// Pascal unit
#include <Windows.hpp>	// Pascal unit
#include <SysInit.hpp>	// Pascal unit
#include <System.hpp>	// Pascal unit

#define MAX_FLOATOLA_FFTLEN 8192
#define MAX_FLOATOLA_CHANNELS 2


int ConvertToPowerOfTwo(int n, int* pnOrder = NULL);



//intenal outputdebugstring routine
void OutputDebugStringTag(int Tag, AnsiString s);

//typedef for callback function


typedef void __fastcall (__closure *TMMSpectrumWaveReady)(  Mmfft::TfCplx *lpSpectrum,
                                                            DWORD dwBins,
                                                            float* lpWaveData,
                                                            DWORD dwSamples,
                                                            bool  &bUseWave
                                                            );

typedef void __fastcall (__closure *TMMWaveReadyPreSpec) ( float* lpWaveData , DWORD dwSamples);
typedef void __fastcall (__closure *TMMWaveReadyPostSpec)( float* lpWaveData , DWORD dwSamples);

namespace Mmfloatolaflt
{

#pragma option push -b-
typedef enum  { wffHalf = 0,
                wffQuarter,
                wffEighth}TMMFloatOlaWindowFeed;
#pragma option pop

struct TFloatOlaFlt;
typedef TFloatOlaFlt *PFloatOlaFlt;
#pragma pack(push, 1)
struct TFloatOlaFlt
   {
   float *FFT_Buf;
   float *Overlap_Buf;
   int Tag;
   int iNumRealChannels;
   int iBuffersProcessed;
   int TotalOverlapAddSamples;
   int WindowLength;
   int WindowShift;
   int TotalLeadingZeros;
   int FFTLen;
   int TotalFFTLen;
   int FFTLen_2;
   Mmfft::TFFTCplx *pfft;
   Mmfft::TfCplx *pfxSortedSpectrum;
   Mmfft::TfCplx *pfxLeft;
   Mmfft::TfCplx *pfxRight;
   TMMSpectrumWaveReady FSpectrumWaveCallBack;
   TMMWaveReadyPreSpec  FWavePreSpecCallBack;
   TMMWaveReadyPostSpec FWavePostSpecCallBack;
   float *FPreWinBuf;
   float *FPostWinBuf;
   char *BufIn;
   int BufIn_Bytes;
   char *BufOut;
   int BufOut_Bytes;
   int IOBufferSize;
   char *WaveCallback_Buf;
   float f4IFFTScalingFactor;
   _RTL_CRITICAL_SECTION DataSection;
   bool FFFTEnabled;
   };
#pragma pack(pop)


extern PACKAGE PFloatOlaFlt __fastcall InitFloatOlaFlt(  unsigned short nChannels,
                                                         int FFTLength,
                                                         int WindowLength,
                                                         TMMFloatOlaWindowFeed WindowFeed,
                                                         int MaxBufSize,
                                                         int Tag
                                                         );

extern PACKAGE void __fastcall DoneFloatOlaFlt(PFloatOlaFlt &pflt);
extern PACKAGE void __fastcall SetFloatOlaFltWindow(PFloatOlaFlt pflt, int Window);
extern PACKAGE void __fastcall ResetFloatOlaFlt(PFloatOlaFlt pflt);
extern PACKAGE void __fastcall DoFloatOlaFlt(PFloatOlaFlt pflt, char * pIn, DWORD *lpdwLength);
extern PACKAGE void __fastcall GetSignal(PFloatOlaFlt pflt, char *pIn, float *pOut);
extern PACKAGE void __fastcall SetSignal(PFloatOlaFlt pflt, float *pIn, char *pOut);
extern PACKAGE void __fastcall SetFloatOlaSpectrumWaveReady(PFloatOlaFlt pflt, TMMSpectrumWaveReady aValue);
extern PACKAGE void __fastcall SetFloatOlaWaveReadyPreSpec(PFloatOlaFlt pflt, TMMWaveReadyPreSpec aValue);
extern PACKAGE void __fastcall SetFloatOlaWaveReadyPostSpec(PFloatOlaFlt pflt, TMMWaveReadyPostSpec aValue);
extern PACKAGE void __fastcall SetFloatOlaFFTEnabled(PFloatOlaFlt pflt, bool bFFTEnabled);
} ;

using namespace Mmfloatolaflt;


//---------------------------------------------------------------------------
#endif
