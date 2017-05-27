// Copyright © 2015 Hansoft AB 
// Distributed under the MIT license, see license text in LICENSE.Malterlib

#include <Mib/Core/Core>

namespace NMib
{

	namespace NException
	{
        /************************************************************************************************\
		||¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯||
		|| CExceptionBase
		||______________________________________________________________________________________________||
		\************************************************************************************************/

#ifdef DMibExceptionTraceEnable
		namespace 
		{
			NAggregate::TCAggregate<NMib::NThread::TCThreadLocal<TCAutoClearInt<bint, true>, NMib::NMem::CAllocator_NonTrackedHeap, NMib::NThread::EThreadLocalFlag_Inherit>, 64> g_EnableExceptionTrace = {DAggregateInit};
			bool g_EnableGlobalExceptionTrace = true;
		}
#endif

		bint fg_SetGlobalEnableExceptionTrace(bint _bEnabled)
		{
#ifdef DMibExceptionTraceEnable
			bint bOld = g_EnableGlobalExceptionTrace;
			g_EnableGlobalExceptionTrace = _bEnabled;
			return bOld;
#else
			return false;
#endif
		}
		
		bint fg_SetEnableExceptionTrace(bint _bEnabled)
		{
#ifdef DMibExceptionTraceEnable
			bint bOld = **g_EnableExceptionTrace;
			**g_EnableExceptionTrace = _bEnabled;
			return bOld;
#else
			return false;
#endif
		}
		
		CExceptionBase &CExceptionBase::operator =(const CExceptionBase&_Other)
		{
			m_Magic = mcp_Magic;
			m_pClass = _Other.m_pClass;
			m_pFile = _Other.m_pFile;
			m_pFunction = _Other.m_pFunction;
			m_Line = _Other.m_Line;
			m_pErrorAlloc.f_Clear();
			m_pErrorAllocNonTracked.f_Clear();
			m_pCallstack.f_Clear();
			m_pCallstackNonTracked.f_Clear();
			m_TypeHash = _Other.m_TypeHash;
			fp_Copy(_Other);
			return *this;
		}

		void CExceptionBase::fp_Copy(const CExceptionBase &_Other)
		{
			if (_Other.m_pErrorAlloc)
			{
				m_pErrorAlloc = fg_Copy(_Other.m_pErrorAlloc);
				m_ErrorNoAlloc[0] = 0;
			}
			else if (_Other.m_pErrorAllocNonTracked)
			{
				m_pErrorAllocNonTracked = fg_Copy(_Other.m_pErrorAllocNonTracked);
				m_ErrorNoAlloc[0] = 0;
			}
			else
				NStr::fg_StrCopy(m_ErrorNoAlloc, _Other.m_ErrorNoAlloc, 128);

			if (_Other.m_pCallstack)
				m_pCallstack = fg_Copy(_Other.m_pCallstack);
			else if (_Other.m_pCallstackNonTracked)
				m_pCallstackNonTracked = fg_Copy(_Other.m_pCallstackNonTracked);
		}
		
		CExceptionBase::CExceptionBase(const CExceptionBase&_Other)
			: m_Magic(mcp_Magic)
			, m_pClass(_Other.m_pClass)
			, m_pFile(_Other.m_pFile)
			, m_pFunction(_Other.m_pFunction)
			, m_Line(_Other.m_Line)
			, m_TypeHash(_Other.m_TypeHash)
		{
			fp_Copy(_Other);
		}

		void CExceptionBase::fp_Construct(const ch8 *_pError, bool _bTrace, bool _bStackTrace)
		{
			if (_bStackTrace)
			{
				m_pCallstack = fg_Construct();
				m_pCallstack->m_CallstackLen = NSys::fg_System_GetStackTrace(m_pCallstack->m_Callstack, 128);
			}

			if (NStr::fg_StrLen(_pError) > 127)
			{
				m_pErrorAlloc = fg_Construct(_pError);
				m_ErrorNoAlloc[0] = 0;
			}
			else
				NStr::fg_StrCopy(m_ErrorNoAlloc, _pError, 128);
#ifdef DMibExceptionTraceEnable
			f_TraceException(_bTrace);
#endif
		}

		void CExceptionBase::fp_ConstructNonTracked(const ch8 *_pError, bool _bTrace, bool _bStackTrace)
		{
			if (_bStackTrace)
			{
				m_pCallstackNonTracked = fg_Construct();
				m_pCallstackNonTracked->m_CallstackLen = NSys::fg_System_GetStackTrace(m_pCallstackNonTracked->m_Callstack, 128);
			}
			if (NStr::fg_StrLen(_pError) > 127)
			{
				m_pErrorAllocNonTracked = fg_Construct(_pError);
				m_ErrorNoAlloc[0] = 0;
			}
			else
				NStr::fg_StrCopy(m_ErrorNoAlloc, _pError, 128);
#ifdef DMibExceptionTraceEnable
			f_TraceException(_bTrace);
#endif
		}

		CExceptionBase::CExceptionBase(const ch8 *_pClass, const ch8 *_pFile, aint _Line, const ch8 *_pFunction, NStr::CStr const &_Error, bool _bTrace, bool _bStackTrace, uint32 _TypeHash)
			: m_Magic(mcp_Magic)
			, m_pClass(_pClass ? _pClass : "CExceptionBase")
			, m_pFile(_pFile ? _pFile : "Unknown")
			, m_pFunction(_pFunction ? _pFunction : "Unknown")
			, m_Line(_Line)
			, m_pErrorAlloc(fg_Construct(_Error))
			, m_TypeHash(_TypeHash)
		{
			m_ErrorNoAlloc[0] = 0;
			if (_bStackTrace)
			{
				m_pCallstack = fg_Construct();
				m_pCallstack->m_CallstackLen = NSys::fg_System_GetStackTrace(m_pCallstack->m_Callstack, 128);
			}
#ifdef DMibExceptionTraceEnable
			f_TraceException(_bTrace);
#endif
		}

		CExceptionBase::CExceptionBase(const ch8 *_pClass, const ch8 *_pFile, aint _Line, const ch8 *_pFunction, NStr::CStrNonTracked const &_Error, bool _bTrace, bool _bStackTrace, uint32 _TypeHash)
			: m_Magic(mcp_Magic)
			, m_pClass(_pClass ? _pClass : "CExceptionBase")
			, m_pFile(_pFile ? _pFile : "Unknown")
			, m_pFunction(_pFunction ? _pFunction : "Unknown")
			, m_Line(_Line)
			, m_pErrorAllocNonTracked(fg_Construct(_Error))
			, m_TypeHash(_TypeHash)
		{
			m_ErrorNoAlloc[0] = 0;
			if (_bStackTrace)
			{
				m_pCallstackNonTracked = fg_Construct();
				m_pCallstackNonTracked->m_CallstackLen = NSys::fg_System_GetStackTrace(m_pCallstackNonTracked->m_Callstack, 128);
			}
#ifdef DMibExceptionTraceEnable
			f_TraceException(_bTrace);
#endif
		}

		void CCallstack::f_Trace(mint _Indent) const
		{
			for (mint i = 0; i < m_CallstackLen; ++i)
			{
				CStackTraceInfo *pInfo = NSys::fg_Debug_AquireStackTraceInfo(m_Callstack[i]);
				if (pInfo)
				{
					const ch8 *FileName = (pInfo->m_pSourceFileName) ? pInfo->m_pSourceFileName : "**Unknown**";
					(void)FileName;
					NSys::fg_DebugOutput((NStr::CStrNonTracked::CFormat("{sf ,sj*}" DMibPFileLineFormat " {}\n") << "" << _Indent << FileName << pInfo->m_SourceLine << (pInfo->m_pFunctionName ? pInfo->m_pFunctionName : "")).f_GetStr().f_GetStr());

					NSys::fg_Debug_ReleaseStackTraceInfo(pInfo);
				}
			}
		}
	
#ifdef DMibExceptionTraceEnable
		void CExceptionBase::f_TraceException(bool _bTrace) const
		{
			if (_bTrace && **g_EnableExceptionTrace)
			{
				NSys::fg_DebugOutput((NStr::CStrNonTracked::CFormat(DMibPFileLineFormat " {}: {}" DMibNewLine) << m_pFile << m_Line << m_pClass << f_GetErrorCharPointer()).f_GetStr().f_GetStr());

				if (m_pCallstack)
					m_pCallstack->f_Trace(0);
			}
		}
#endif

		CExceptionBase::CExceptionBase(const ch8 *_pClass, const ch8 *_pFile, aint _Line, const ch8 *_pFunction, const ch8 *_pError, bool _bTrace, bool _bStackTrace, uint32 _TypeHash)
			: m_Magic(mcp_Magic)
			, m_pClass(_pClass ? _pClass : "CExceptionBase")
			, m_pFile(_pFile ? _pFile : "Unknown")
			, m_pFunction(_pFunction ? _pFunction : "Unknown")
			, m_Line(_Line)
			, m_TypeHash(_TypeHash)
		{

			if (_bStackTrace)
			{
				m_pCallstack = fg_Construct();
				m_pCallstack->m_CallstackLen = NSys::fg_System_GetStackTrace(m_pCallstack->m_Callstack, 128);
			}

			if (NStr::fg_StrLen(_pError) > 127)
			{
				m_pErrorAlloc = fg_Construct(_pError);
				m_ErrorNoAlloc[0] = 0;
			}
			else
			{
				NStr::fg_StrCopy(m_ErrorNoAlloc, _pError, 128);
			}
#ifdef DMibExceptionTraceEnable
			f_TraceException(_bTrace);
#endif
		}

		CExceptionBase::~CExceptionBase()
		{
		}

		bint CExceptionBase::f_IsValid() const
		{
			return m_Magic == mcp_Magic;
		}

		const ch8 *CExceptionBase::f_GetErrorCharPointer() const
		{
			if (m_pErrorAlloc)
				return *m_pErrorAlloc;
			else if (m_pErrorAllocNonTracked)
				return *m_pErrorAllocNonTracked;
			else
				return m_ErrorNoAlloc;
		}

		NStr::CStr CExceptionBase::f_GetErrorStr() const
		{
			if (m_pErrorAlloc)
				return *m_pErrorAlloc;
			else if (m_pErrorAllocNonTracked)
				return *m_pErrorAllocNonTracked;
			else
				return NStr::CStr(m_ErrorNoAlloc);
		}

		CCallstack const *CExceptionBase::f_GetCallstack()
		{
			if (m_pCallstack)
				return m_pCallstack.f_Get();
			else if (m_pCallstackNonTracked)
				return m_pCallstackNonTracked.f_Get();

			return nullptr;
		}


		NStr::CStrNonTracked CExceptionBase::f_GetErrorStrNonTracked() const
		{
			if (m_pErrorAlloc)
				return *m_pErrorAlloc;
			else if (m_pErrorAllocNonTracked)
				return *m_pErrorAllocNonTracked;
			else
				return NStr::CStrNonTracked(m_ErrorNoAlloc);
		}

		const ch8 *CExceptionBase::f_GetClass() const
		{
			return m_pClass;
		}
		const ch8 *CExceptionBase::f_GetFile() const
		{
			return m_pFile;
		}
		const ch8 *CExceptionBase::f_GetFunction() const
		{
			return m_pFunction;
		}
		int32 CExceptionBase::f_GetLine() const
		{
			return m_Line;
		}

	}
}

