//-----------------------------------------------------------------------------
/// \file casioExceptions.h
/// \author Berg
/// \brief Interface of exceptions used by CAsio class
///
/// Project SoundMexPro
/// Module SoundDllPro
/// Interface of exceptions used by CAsio class
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
#ifndef casioExceptionsH
#define casioExceptionsH

#include <windows.h>

#include "casioEnums.h"

// AnsiString is the only Borland-specific dependency of CAsio.
// Therefore, CAsio may be used with other compilers, if a sufficient
// AnsiString implementation is provided.
#ifdef __BORLANDC__
#include <vcl.h>
#else
// AnsiString needs to implement AnsiString(), AnsiString(const char*), 
// cat_sprintf(const char *, ...).
#include "AnsiString.h"
#endif

//---------------------------------------------------------------------------
/// NOTE: The exceptions defined in this file are not derived from Borland VCL
/// exception class Exception, because they are used in CAsio class, that implements
/// real time signal processing strategies that should not use/depend on VCL
/// threading methods/strategies
//---------------------------------------------------------------------------
/// NOTE: All exception constructors do _not_ copy passed error messsages ot
/// method names. Therefore use them only with string literals!!!
//---------------------------------------------------------------------------
namespace Asio {
    class EAsioError
    {
    public:
        /// Name of the method that raised this exception.
        const char * m_lpszMethod;

        /// Static description of the error condition.
        const char * m_lpszMsg;

        /// Create Exception object.
        /// \param lpszMethod
        ///   pointer to static string holding the name of the method that
        ///   raised the exception.  This has to be a static string since this
        ///   constructor will only copy the address, not the string itself.
        /// \param lpszMsg
        ///   pointer to static string holding a description of the error
        ///   condition that caused the exception.  This has to be a static
        ///   string since this constructor will only copy the address, not the
        ///   string itself.
        EAsioError(const char * lpszMethod, const char * lpszMsg)
          : m_lpszMethod(lpszMethod),
            m_lpszMsg(lpszMsg)
          {}
        virtual ~EAsioError() {}
    };

    /// Error class for signalling errors from the win32 API
    class EWin32Error : public EAsioError {
    public:
        /// The system error number.
        int m_iErrno;

        /// Create Win32 Error Exception Object.
        /// \param lpszMethod
        ///   pointer to static string holding the name of the method that
        ///   raised the exception.  This has to be a static string since this
        ///   constructor will only copy the address, not the string itself.
        /// \param lpszMsg
        ///   pointer to static string holding a description of the error
        ///   condition that caused the exception.  This has to be a static
        ///   string since this constructor will only copy the address, not the
        ///   string itself.
        /// \param iErrno
        ///   The win32 Error number, typically from GetLastError().
        EWin32Error(const char * lpszMethod, const char * lpszMsg, int iErrno)
            : EAsioError(lpszMethod, lpszMsg),
              m_iErrno(iErrno)
        {}
    };

    /// Exception class used for signalling a violation of the rule that an
    /// Asio application using the Asio SDK may only use a single Asio driver.
    class EMultipleInstancesError : public EAsioError {
    public:
        /// Create Exception Object.
        /// \param lpszMethod
        ///   pointer to static string holding the name of the method that
        ///   raised the exception.  This has to be a static string since this
        ///   constructor will only copy the address, not the string itself.
        /// \param lpszMsg
        ///   pointer to static string holding a description of the error
        ///   condition that caused the exception.  This has to be a static
        ///   string since this constructor will only copy the address, not the
        ///   string itself.
        EMultipleInstancesError(const char * lpszMethod, const char * lpszMsg)
            : EAsioError(lpszMethod, lpszMsg)
        {}
    };

    /// Exception class for Errors that "cannot happen". Examples:
    /// some Asio function returns unexpected error code, or the
    /// internal state of an object is inconsistent.
    class EUnexpectedError : public EAsioError {
    public:
        /// The name of the source file that raised this exception.
        const char * m_lpszFile;

        /// The line number where the exception was raised.
        unsigned long m_nLine;

        /// A pointer to the instance that raised the exception, if applicable.
        const void * m_pvInst;

        /// The unexpected error code returned by the Asio driver, if applicable
        double m_dErrorCode;

        /// Create exception object.
        /// \param lpszFile
        ///   The name of the source file that raised this exception.
        /// \param nLine
        ///   Line number where the exception was raised.
        /// \param lpszMethod
        ///   pointer to static string holding the name of the method that
        ///   raised the exception.  This has to be a static string since this
        ///   constructor will only copy the address, not the string itself.
        /// \param lpszMsg
        ///   pointer to static string holding a description of the error
        ///   condition that caused the exception.  This has to be a static
        ///   string since this constructor will only copy the address, not the
        ///   string itself.
        /// \param pvInst
        ///   A pointer to the instance that raised the exception, if applicable
        /// \param dErrorCode
        ///   The unexpected error code, if applicable.
        EUnexpectedError(const char * lpszFile,
                         unsigned long nLine,
                         const char * lpszMethod,
                         const char * lpszMsg,
                         const void * pvInst = 0,
                         double dErrorCode = -0.5)
            : EAsioError(lpszMethod, lpszMsg),
              m_lpszFile(lpszFile),
              m_nLine(nLine),
              m_pvInst(pvInst),
              m_dErrorCode(dErrorCode)
            {}
    };

    /// Exception class used when the number of (1) drivers on system, or (2)
    /// the number of channels on a sound card, or (3) the number of possible
    /// clock sources is reported <0 by Asio. 
    class ENegativeCountError : public EUnexpectedError {
    public:
        /// Create exception object.
        /// \param lpszFile
        ///   The name of the source file that raised this exception.
        /// \param nLine
        ///   Line number where the exception was raised.
        /// \param lpszMethod
        ///   pointer to static string holding the name of the method that
        ///   raised the exception.  This has to be a static string since this
        ///   constructor will only copy the address, not the string itself.
        /// \param lpszMsg
        ///   pointer to static string holding a description of the error
        ///   condition that caused the exception.  This has to be a static
        ///   string since this constructor will only copy the address, not the
        ///   string itself.
        /// \param pvInst
        ///   A pointer to the instance that raised the exception
        /// \param nNegativeCount
        ///   The unexpected negative count. Stored in m_dErrorCode.
        ENegativeCountError(const char * lpszFile,
                            unsigned long nLine,
                            const char * lpszMethod,
                            const char * lpszMsg,
                            const void * pvInst,
                            signed long nNegativeCount)
            : EUnexpectedError(lpszFile, nLine, lpszMethod, lpszMsg, pvInst,
                               nNegativeCount)
            {}
    };

    /// Exception class used when a value reported by the asio driver
    /// exceeds the handling capacity of CAsio.  Used to limit the
    /// number of supported clock sources.
    /// Inherits from EUnexpectedError since the limits are chosen high enough
    /// that the driver exceeding them is extremely unlikely.
    class ELimitExceededError : public EUnexpectedError {
    public:
        /// The limit that was exceeded.
        double m_dLimit;

        /// Create exception object.
        /// \param lpszFile
        ///   The name of the source file that raised this exception.
        /// \param nLine
        ///   Line number where the exception was raised.
        /// \param lpszMethod
        ///   pointer to static string holding the name of the method that
        ///   raised the exception.  This has to be a static string since this
        ///   constructor will only copy the address, not the string itself.
        /// \param lpszMsg
        ///   pointer to static string holding a description of the error
        ///   condition that caused the exception.  This has to be a static
        ///   string since this constructor will only copy the address, not the
        ///   string itself.
        /// \param pvInst
        ///   A pointer to the instance that raised the exception
        /// \param dLimit
        ///   The limit that was exceeded
        /// \param dValue
        ///   The value that exceeds the limit. Stored in m_dErrorCode.
        ELimitExceededError(const char * lpszFile,
                            unsigned long nLine,
                            const char * lpszMethod,
                            const char * lpszMsg,
                            const void * pvInst,
                            double dLimit,
                            double dValue)
            : EUnexpectedError(lpszFile, nLine, lpszMethod, lpszMsg, pvInst,
                               dValue),
              m_dLimit(dLimit)
            {}
    };

    /// Exception class used when the internal state of the CAsio instance is
    /// inconsistent
    class EInconsistentStateError : public EUnexpectedError {
    public:
        /// Create exception object.
        /// \param lpszFile
        ///   The name of the source file that raised this exception.
        /// \param nLine
        ///   Line number where the exception was raised.
        /// \param lpszMethod
        ///   pointer to static string holding the name of the method that
        ///   raised the exception.  This has to be a static string since this
        ///   constructor will only copy the address, not the string itself.
        /// \param lpszMsg
        ///   pointer to static string holding the name of the
        ///   instance variable that has a value that caused the
        ///   exception.  This has to be a static string since this
        ///   constructor will only copy the address, not the string
        ///   itself.
        /// \param pvInst
        ///   A pointer to the instance that raised the exception
        /// \param dValue
        ///   The value of the deviating instance variable.  Stored in
        ///   m_dErrorCode.
        EInconsistentStateError(const char * lpszFile,
                                unsigned long nLine,
                                const char * lpszMethod,
                                const char * lpszMsg,
                                const void * pvInst,
                                double dValue)
            : EUnexpectedError(lpszFile, nLine, lpszMethod, lpszMsg, pvInst,
                               dValue)
            {}
    };

    /// Exception raised when loading or initialization of the Asio driver fails
    class ELoadError : public EAsioError {
    public:
        /// Create load error object.
        /// \param lpszMethod
        ///   pointer to static string holding the name of the method that
        ///   raised the exception.  This has to be a static string since this
        ///   constructor will only copy the address, not the string itself.
        /// \param lpszMsg
        ///   pointer to static string holding a description of the error
        ///   condition that caused the exception.  This has to be a static
        ///   string since this constructor will only copy the address, not the
        ///   string itself.
        ELoadError(const char * lpszMethod,
                   const char * lpszMsg)
            : EAsioError(lpszMethod, lpszMsg)
        {}
    };

    /// Exception raised when the Asio driver is not in the needed state for
    /// the requested operation.
    class EStateError : public EAsioError {
    public:
        /// The minimum state required.
        State m_asMinState;

        /// The maximum acceptable state.
        State m_asMaxState;

        /// The actual state.
        State m_asActualState;

        /// Create state error object.
        /// \param lpszMethod
        ///   pointer to static string holding the name of the method that
        ///   raised the exception.  This has to be a static string since this
        ///   constructor will only copy the address, not the string itself.
        /// \param lpszMsg
        ///   pointer to static string holding a description of the error
        ///   condition that caused the exception.  This has to be a static
        ///   string since this constructor will only copy the address, not the
        ///   string itself.
        /// \param asMinState
        ///   Precondition minimum state for the requested operation.
        /// \param asMaxState
        ///   Maximum acceptable state for the requested operation.
        /// \param asActualState
        ///   The current state of the Asio driver outside the range [iMinState,
        ///   iMaxState].
        EStateError(const char * lpszMethod,
                    const char * lpszMsg,
                    State asMinState,
                    State asMaxState,
                    State asActualState)
            : EAsioError(lpszMethod, lpszMsg),
              m_asMinState(asMinState),
              m_asMaxState(asMaxState),
              m_asActualState(asActualState)
        {}
    };

    /// Exception raised when the driver supports the wrong Asio Version.
    class EVersionError : public EAsioError {
    public:
        /// The application supports only this Asio version.
        long m_iRequiredVersion;

        /// The driver supports only this Asio version.
        long m_iActualVersion;

        /// Create exception instance when the driver supports bad AISO version.
        /// \param lpszMethod
        ///   pointer to static string holding the name of the method that
        ///   raised the exception.  This has to be a static string since this
        ///   constructor will only copy the address, not the string itself.
        /// \param lpszMsg
        ///   pointer to static string holding a description of the error
        ///   condition that caused the exception.  This has to be a static
        ///   string since this constructor will only copy the address, not the
        ///   string itself.
        /// \param iRequiredVersion
        ///   Asio Version supported by the application.
        /// \param iActualVersion
        ///   The actual Asio version supported by the Asio driver.
        EVersionError(const char * lpszMethod,
                      const char * lpszMsg,
                      long iRequiredVersion,
                      long iActualVersion)
            : EAsioError(lpszMethod, lpszMsg),
              m_iRequiredVersion(iRequiredVersion),
              m_iActualVersion(iActualVersion)
        {}
    };

    /// Exception thrown when the driver, some channels, the sound card, etc, 
    /// are not available to the application at the moment.
    class EUnavailableError : public EAsioError {
    public:
        EUnavailableError(const char * lpszMethod,
                          const char * lpszMsg)
            : EAsioError(lpszMethod, lpszMsg)
        {}
    };

    /// Exception thrown when an Index, e.g. for a channel, is out of bounds.
    class EIndexError : public EAsioError {
    public:
        /// Minimum permitted Index.
        long m_iMinIndex;

        /// Maximum acceptable Index.
        long m_iMaxIndex;

        /// The actual Index.
        long m_iActualIndex;

        /// Create Index Error object.
        /// \param lpszMethod
        ///   pointer to static string holding the name of the method that
        ///   raised the exception.  This has to be a static string since this
        ///   constructor will only copy the address, not the string itself.
        /// \param lpszMsg
        ///   pointer to static string holding a description of the error
        ///   condition that caused the exception.  This has to be a static
        ///   string since this constructor will only copy the address, not the
        ///   string itself.
        /// \param iMinIndex
        ///   Minimum permitted Index.
        /// \param iMaxIndex
        ///   Maximum acceptable Index.
        /// \param iActualIndex
        ///   The requested index outside the permitted range [iMinIndex,
        ///   iMaxIndex].
        EIndexError(const char * lpszMethod,
                    const char * lpszMsg,
                    long iMinIndex,
                    long iMaxIndex,
                    long iActualIndex)
            : EAsioError(lpszMethod, lpszMsg),
              m_iMinIndex(iMinIndex),
              m_iMaxIndex(iMaxIndex),
              m_iActualIndex(iActualIndex)
        {}
    };

    /// Class for exceptions thrown when some parameters to an Asio call are
    /// outside the range of values that the Asio driver supports.
    class EUnsupportedParamsError : public EAsioError {
    public:
        /// Create Exception object.
        /// \param lpszMethod
        ///   pointer to static string holding the name of the method that
        ///   raised the exception.  This has to be a static string since this
        ///   constructor will only copy the address, not the string itself.
        /// \param lpszMsg
        ///   pointer to static string holding a description of the error
        ///   condition that caused the exception.  This has to be a static
        ///   string since this constructor will only copy the address, not the
        ///   string itself.
        EUnsupportedParamsError(const char * lpszMethod,
                                const char * lpszMsg)
            : EAsioError(lpszMethod, lpszMsg)
        {}
    };

    /// Class for Exceptions raised when the Asio driver dows not support a
    /// requested sample rate, of if the current sample rate is unknown.
    class ENoClockError : public EUnsupportedParamsError {
    public:
        /// The sample rate that is not supported, or -1.0 if not applicable.
        double m_dSrate;

        /// Create Exception object.
        /// \param lpszMethod
        ///   pointer to static string holding the name of the method that
        ///   raised the exception.  This has to be a static string since this
        ///   constructor will only copy the address, not the string itself.
        /// \param lpszMsg
        ///   pointer to static string holding a description of the error
        ///   condition that caused the exception.  This has to be a static
        ///   string since this constructor will only copy the address, not the
        ///   string itself.
        /// \param dSrate
        ///   The sample rate in Hz refused by the driver, or -1.0 if not
        ///   applicable.
        ENoClockError(const char * lpszMethod,
                      const char * lpszMsg,
                      double dSrate = -1.0)
            : EUnsupportedParamsError(lpszMethod, lpszMsg),
              m_dSrate(dSrate)
        {}
    };

    /// Class for exceptions thrown when a sampling rate is requested when the
    /// soundcard runs on external sync, or the application requests invalid
    /// settings in the createBuffers method.
    class EModeError : public EUnsupportedParamsError {
    public:
        /// Create Exception object.
        /// \param lpszMethod
        ///   pointer to static string holding the name of the method that
        ///   raised the exception.  This has to be a static string since this
        ///   constructor will only copy the address, not the string itself.
        /// \param lpszMsg
        ///   pointer to static string holding a description of the error
        ///   condition that caused the exception.  This has to be a static
        ///   string since this constructor will only copy the address, not the
        ///   string itself.
        EModeError(const char * lpszMethod, const char * lpszMsg)
            : EUnsupportedParamsError(lpszMethod, lpszMsg)
        {}
    };

    /// Class for Exceptions thrown when memory allocation fails.
    class ENoMemoryError : public EAsioError {
    public:
        ENoMemoryError(const char * lpszMethod, const char * lpszMsg)
            : EAsioError(lpszMethod, lpszMsg)
        {}
    };

    /// Base class for exceptions thrown by SoundDataQueue in Xrun situations
    class EXrunError : public EAsioError {
    public:
        // differentiates between overrun and underrun of the queue
        bool m_bUnderrun;

        /// Create Xrun Exception.
        /// \param lpszMethod
        ///   pointer to static string holding the name of the method that
        ///   raised the exception.  This has to be a static string since this
        ///   constructor will only copy the address, not the string itself.
        /// \param bUnderrun
        ///   true if an underrun occurred,
        ///   false if an overrun caused this exception.
        EXrunError(const char * lpszMethod, bool bUnderrun)
            : EAsioError(lpszMethod,
                         (bUnderrun
                          ? "Cannot read data from empty Queue"
                          : "Cannot write data to full Queue")),
              m_bUnderrun(bUnderrun)
        {}
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
