// Copyright © 2015 Hansoft AB 
// Distributed under the MIT license, see license text in LICENSE.Malterlib

#pragma once

#include <Mib/Core/Core>

namespace NMib
{
	namespace NException
	{
		template <typename t_CFormatter>
		// This crashes GCC
		// auto f_CreateStringFormatter(t_CFormatter &_Formatter) const -> decltype(NStr::fg_CreateStringFormatter(_Formatter, ""))
		NStr::CStrFormatTypeClassifier_String CExceptionBase::f_CreateStringFormatter(t_CFormatter &_Formatter) const
		{
			return NStr::fg_CreateStringFormatter(_Formatter, fg_ByValue(f_GetErrorStr()));
		}
    
		template <typename t_CError>
		CExceptionBase::CExceptionBase(const ch8 *_pClass, const ch8 *_pFile, aint _Line, const ch8 *_pFunction, NStr::TCStrAggregate<t_CError> const &_Error, bool _bTrace, bool _bStackTrace, uint32 _TypeHash)
			: m_Magic(mcp_Magic)
			, m_pClass(_pClass ? _pClass : "CExceptionBase")
			, m_pFile(_pFile ? _pFile : "Unknown")
			, m_pFunction(_pFunction ? _pFunction : "Unknown")
			, m_Line(_Line)
			, m_TypeHash(_TypeHash)
		{
			if (NTraits::TCIsSame<typename t_CError::CStrTraits::CAllocator, NStr::CStrNonTracked::CAllocator>::mc_Value || !NStr::TCStr<t_CError>::mc_AllocatesMemory)
				fp_ConstructNonTracked(_Error.f_GetStr(), _bTrace, _bStackTrace);
			else
				fp_Construct(_Error.f_GetStr(), _bTrace, _bStackTrace);
		}
		
		template <typename t_CError>
		CExceptionBase::CExceptionBase(const ch8 *_pClass, const ch8 *_pFile, aint _Line, const ch8 *_pFunction, NStr::TCStr<t_CError> const &_Error, bool _bTrace, bool _bStackTrace, uint32 _TypeHash)
			: m_Magic(mcp_Magic)
			, m_pClass(_pClass ? _pClass : "CExceptionBase")
			, m_pFile(_pFile ? _pFile : "Unknown")
			, m_pFunction(_pFunction ? _pFunction : "Unknown")
			, m_Line(_Line)
			, m_TypeHash(_TypeHash)
		{
			if (NTraits::TCIsSame<typename t_CError::CStrTraits::CAllocator, NStr::CStrNonTracked::CAllocator>::mc_Value || !NStr::TCStr<t_CError>::mc_AllocatesMemory)
				fp_ConstructNonTracked(_Error.f_GetStr(), _bTrace, _bStackTrace);
			else
				fp_Construct(_Error.f_GetStr(), _bTrace, _bStackTrace);
		}

		inline_always_debug uint32 CExceptionBase::f_TypeHash() const
		{
			return m_TypeHash;
		}
		
#ifdef DMibRuntimeTypeRegistry
		template <typename tf_CStream>
		void CExceptionBase::f_Feed(tf_CStream &_Stream) const
		{
			_Stream << f_GetErrorStrNonTracked();
		}
		
		template <typename tf_CStream>
		void CExceptionBase::f_Consume(tf_CStream &_Stream)
		{
			NStr::CStrNonTracked NonTracked;
			_Stream >> NonTracked;
			m_pErrorAllocNonTracked = fg_Construct(NonTracked);
			m_ErrorNoAlloc[0] = 0;
		}
#endif
		
#ifdef DMibExceptionTraceEnable
		CDisableExceptionTraceScope::CDisableExceptionTraceScope()
			: mp_bOldEnable(fg_SetEnableExceptionTrace(false))
		{
		}
		CDisableExceptionTraceScope::~CDisableExceptionTraceScope()
		{
			fg_SetEnableExceptionTrace(mp_bOldEnable);
		}
#else
		CDisableExceptionTraceScope::CDisableExceptionTraceScope()
		{
		}
		
		CDisableExceptionTraceScope::~CDisableExceptionTraceScope()
		{
		}
#endif
	}
}
