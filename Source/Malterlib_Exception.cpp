// Copyright © 2015 Hansoft AB 
// Distributed under the MIT license, see license text in LICENSE.Malterlib

#include <Mib/Core/Core>

namespace NMib::NException
{
	/************************************************************************************************\
	||¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯||
	|| CExceptionBase
	||______________________________________________________________________________________________||
	\************************************************************************************************/

#ifdef DMibExceptionTraceEnable
	namespace
	{
		struct CEnableExceptionTraceThreadLocal
		{
			CEnableExceptionTraceThreadLocal() = default;
			CEnableExceptionTraceThreadLocal(CEnableExceptionTraceThreadLocal const &_Other)
				: m_bEnableTrace(_Other.m_bEnableTrace.f_Load())
			{
			}

			NAtomic::TCAtomic<bool> m_bEnableTrace = true;
		};

		constinit NStorage::TCAggregate<NMib::NThread::TCThreadLocal<CEnableExceptionTraceThreadLocal, NMib::NMemory::CAllocator_NonTrackedHeap, NMib::NThread::EThreadLocalFlag_Inherit>, 64>
			g_EnableExceptionTrace = {DAggregateInit}
		;
		bool g_EnableGlobalExceptionTrace = true;
	}
#endif

	bool fg_SetGlobalEnableExceptionTrace(bool _bEnabled)
	{
#ifdef DMibExceptionTraceEnable
		bool bOld = g_EnableGlobalExceptionTrace;
		g_EnableGlobalExceptionTrace = _bEnabled;
		return bOld;
#else
		return false;
#endif
	}

	bool fg_SetEnableExceptionTrace(bool _bEnabled)
	{
#ifdef DMibExceptionTraceEnable
		auto &ThreadLocal = **g_EnableExceptionTrace;
		bool bOld = ThreadLocal.m_bEnableTrace.f_Load();
		ThreadLocal.m_bEnableTrace = _bEnabled;
		return bOld;
#else
		return false;
#endif
	}

	NStr::CStr fg_ExceptionString(CExceptionPointer const &_pExceptionPointer)
	{
		try
		{
			std::rethrow_exception(_pExceptionPointer);
		}
		catch (NException::CException const &_Exception)
		{
			return _Exception.f_GetErrorStr();
		}
		return "";
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
#if DMibConfig_Exception_SupportStackTraces
		if (_bStackTrace)
		{
			m_pCallstack = fg_Construct();
			m_pCallstack->m_CallstackLen = NSys::fg_System_GetStackTrace(m_pCallstack->m_Callstack, 128);
		}
#endif
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
#if DMibConfig_Exception_SupportStackTraces
		if (_bStackTrace)
		{
			m_pCallstackNonTracked = fg_Construct();
			m_pCallstackNonTracked->m_CallstackLen = NSys::fg_System_GetStackTrace(m_pCallstackNonTracked->m_Callstack, 128);
		}
#endif
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
#if DMibConfig_Exception_SupportStackTraces
		if (_bStackTrace)
		{
			m_pCallstack = fg_Construct();
			m_pCallstack->m_CallstackLen = NSys::fg_System_GetStackTrace(m_pCallstack->m_Callstack, 128);
		}
#endif
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
#if DMibConfig_Exception_SupportStackTraces
		if (_bStackTrace)
		{
			m_pCallstackNonTracked = fg_Construct();
			m_pCallstackNonTracked->m_CallstackLen = NSys::fg_System_GetStackTrace(m_pCallstackNonTracked->m_Callstack, 128);
		}
#endif
#ifdef DMibExceptionTraceEnable
		f_TraceException(_bTrace);
#endif
	}

	NStr::CStr CCallstack::f_GetFunctionName(mint _iCallstack) const
	{
		if (_iCallstack >= m_CallstackLen)
			return {};

		CStackTraceInfo *pInfo = NSys::fg_Debug_AquireStackTraceInfo(m_Callstack[_iCallstack]);
		if (!pInfo || !pInfo->m_pFunctionName)
			return {};
		NStr::CStr FunctionName = pInfo->m_pFunctionName;
		
		NSys::fg_Debug_ReleaseStackTraceInfo(pInfo);

		return FunctionName;
	}

	NStr::CStrNonTracked CCallstack::fs_ShortenFunctionName(ch8 const *_pFunction)
	{
		NStr::CStrNonTracked FunctionName;
		NStr::CStrNonTracked ClassName;

		mint nOpen = 0;
		ch8 const *pParse = _pFunction;
		ch8 const *pIdentStart = pParse;
		bool bFoundFunction = false;
		while (*pParse)
		{
			if (NStr::fg_StrStartsWith(pParse, "operator>>"))
			{
				pParse += 10;
				continue;
			}
			if (NStr::fg_StrStartsWith(pParse, "operator<<"))
			{
				pParse += 10;
				continue;
			}
			if (NStr::fg_StrStartsWith(pParse, "operator>"))
			{
				pParse += 9;
				continue;
			}
			if (NStr::fg_StrStartsWith(pParse, "operator<"))
			{
				pParse += 9;
				continue;
			}
			if (NStr::fg_StrStartsWith(pParse, "operator()"))
			{
				pParse += 10;
				continue;
			}
			if (NStr::fg_StrStartsWith(pParse, "operator[]"))
			{
				pParse += 10;
				continue;
			}
			if (*pParse == '<')
			{
				if (nOpen == 0)
				{
					if (ClassName.f_IsEmpty())
						ClassName.f_AddStr(pIdentStart, pParse - pIdentStart);
					else if (FunctionName.f_IsEmpty())
						FunctionName.f_AddStr(pIdentStart, pParse - pIdentStart);
					pIdentStart = pParse;
				}
				++nOpen;
			}
			else if (*pParse == '(')
			{
				if (nOpen == 0)
				{
					bFoundFunction = true;
					if (FunctionName.f_IsEmpty())
						FunctionName.f_AddStr(pIdentStart, pParse - pIdentStart);
					pIdentStart = pParse;
				}
				++nOpen;
			}
			else if (*pParse == '[')
				++nOpen;
			else if (*pParse == ')' || *pParse == '>' || *pParse == ']')
			{
				--nOpen;
				if (nOpen == 0)
				{
					pIdentStart = pParse + 1;
					if (*pParse == '>')
					{
						while (*pIdentStart == ':')
							++pIdentStart;
					}
				}
			}
			else if (nOpen == 0)
			{
				if (*pParse == ' ')
				{
					if (!bFoundFunction)
					{
						FunctionName.f_Clear();
						ClassName.f_Clear();
					}

					pIdentStart = pParse + 1;
				}
			}

			++pParse;
		}

		if (!bFoundFunction)
			return _pFunction;
		else if (!FunctionName.f_IsEmpty() && !ClassName.f_IsEmpty())
			return NStr::fg_Format("{}::{}()", ClassName, FunctionName);
		else if (FunctionName.f_IsEmpty())
			return ClassName + "{}";

		return _pFunction;
	}

	void CCallstack::f_Trace(mint _Indent) const
	{
		for (mint i = 0; i < m_CallstackLen; ++i)
		{
			CStackTraceInfo *pInfo = NSys::fg_Debug_AquireStackTraceInfo(m_Callstack[i]);
			if (pInfo)
			{
				NStr::CStrNonTracked FunctionName = fs_ShortenFunctionName(pInfo->m_pFunctionName ? pInfo->m_pFunctionName : "");

				if (pInfo->m_pSourceFileName)
				{
					NSys::fg_DebugOutput
						(
						 	(
							 	NStr::CStrNonTracked::CFormat("{sf ,sj*}" DMibPFileLineFormat " {}\n")
							 	<< ""
							 	<< _Indent
							 	<< pInfo->m_pSourceFileName
							 	<< pInfo->m_SourceLine
							 	<< FunctionName
							)
						 	.f_GetStr().f_GetStr()
						)
					;
				}
				else
					NSys::fg_DebugOutput((NStr::CStrNonTracked::CFormat("{sf ,sj*}{}\n") << "" << _Indent << FunctionName).f_GetStr().f_GetStr());

				NSys::fg_Debug_ReleaseStackTraceInfo(pInfo);
			}
		}
	}

	NStr::CStr CCallstack::f_GetString(mint _Indent) const
	{
		NStr::CStr Output;
		for (mint i = 0; i < m_CallstackLen; ++i)
		{
			CStackTraceInfo *pInfo = NSys::fg_Debug_AquireStackTraceInfo(m_Callstack[i]);
			if (pInfo)
			{
				NStr::CStrNonTracked FunctionName = fs_ShortenFunctionName(pInfo->m_pFunctionName ? pInfo->m_pFunctionName : "");

				if (pInfo->m_pSourceFileName)
					Output += NStr::CStr::CFormat("{sf ,sj*}" DMibPFileLineFormat " {}\n") << "" << _Indent << pInfo->m_pSourceFileName << pInfo->m_SourceLine << FunctionName;
				else
					Output += NStr::CStr::CFormat("{sf ,sj*}{}\n") << "" << _Indent << FunctionName;

				NSys::fg_Debug_ReleaseStackTraceInfo(pInfo);
			}
		}
		return Output;
	}

#ifdef DMibExceptionTraceEnable
	void CExceptionBase::f_TraceException(bool _bTrace) const
	{
		if (_bTrace && (**g_EnableExceptionTrace).m_bEnableTrace.f_Load(NAtomic::EMemoryOrder_Relaxed))
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
#if DMibConfig_Exception_SupportStackTraces
		if (_bStackTrace)
		{
			m_pCallstack = fg_Construct();
			m_pCallstack->m_CallstackLen = NSys::fg_System_GetStackTrace(m_pCallstack->m_Callstack, 128);
		}
#endif
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

	bool CExceptionBase::f_IsValid() const
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

	CCallstack const *CExceptionBase::f_GetCallstack() const
	{
		if (m_pCallstack)
			return m_pCallstack.f_Get();
		else if (m_pCallstackNonTracked)
			return m_pCallstackNonTracked.f_Get();

		return nullptr;
	}

	NStr::CStr CExceptionBase::f_GetCallstackStr(mint _Indent) const
	{
		if (m_pCallstack)
		{
			if (auto *pCallstack = m_pCallstack.f_Get())
				return pCallstack->f_GetString(_Indent);
		}
		else if (m_pCallstackNonTracked)
		{
			if (auto *pCallstack = m_pCallstackNonTracked.f_Get())
				return pCallstack->f_GetString(_Indent);
		}
		return "";
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

#ifdef DMibExceptionTraceEnable
	void CDisableExceptionTraceScope::f_Suspend()
	{
		fg_SetEnableExceptionTrace(mp_bOldEnable);
	}

	void CDisableExceptionTraceScope::f_Resume()
	{
		mp_bOldEnable = fg_SetEnableExceptionTrace(false);
	}
#endif

	uint32 CException::ms_TypeHash = DMibException_TypeHash(CException);

	CExceptionPointer CException::f_ExceptionPointer() const
	{
		return std::make_exception_ptr(*this);
	}

	void CException::fp_RegisterTypeRegistry() const
	{
		DMibImpErrorClass_TypeRegistry(CException);
	}

	uint32 CDebugException::ms_TypeHash = DMibException_TypeHash(CDebugException);

	CExceptionPointer CDebugException::f_ExceptionPointer() const
	{
		return std::make_exception_ptr(*this);
	}

	void CDebugException::fp_RegisterTypeRegistry() const
	{
		DMibImpErrorClass_TypeRegistry(CDebugException);
	}

	DMibImpErrorClassImplement(CExceptionMemory);
	DMibImpErrorClassImplement(CExceptionSystemImplementation);
	DMibImpErrorClassImplement(CExceptionPureCall);
	DMibImpErrorClassImplement(CExceptionBadFunctionCall);
	DMibImpErrorClassImplement(CExceptionSafeCheck);
	DMibImpErrorClassImplement(CExceptionExceptionVector);
	DMibImpErrorClassImplement(CExceptionWrapped);
}

namespace NMib::NFile
{
	DMibImpErrorClassImplement(CExceptionFile);
}
