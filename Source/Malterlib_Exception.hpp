// Copyright © 2015 Hansoft AB 
// Distributed under the MIT license, see license text in LICENSE.Malterlib

#pragma once

#include <Mib/Core/Core>

namespace NMib::NException
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
		if constexpr (NTraits::cIsSame<typename t_CError::CStrTraits::CAllocator, NStr::CStrNonTracked::CAllocator> || !NStr::TCStr<t_CError>::mc_AllocatesMemory)
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
		if constexpr (NTraits::cIsSame<typename t_CError::CStrTraits::CAllocator, NStr::CStrNonTracked::CAllocator> || !NStr::TCStr<t_CError>::mc_AllocatesMemory)
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

	struct CExceptionWrappedData
	{
		CExceptionWrappedData() = default;
		CExceptionWrappedData(CExceptionPointer const &_pWrapped)
			: m_pWrapped(_pWrapped)
		{
		}

		template <typename tf_CStream>
		void f_Stream(tf_CStream &_Stream)
		{
			_Stream % m_pWrapped;
		}

		CExceptionPointer m_pWrapped;
	};

	DMibImpErrorSpecificClassDefine(CExceptionWrapped, CException, CExceptionWrappedData);

#	define DMibErrorWrapped(d_Description, d_Specific, ...) DMibImpErrorSpecific(NMib::NException::CExceptionWrapped, d_Description, d_Specific, ##__VA_ARGS__)
#	define DMibErrorInstanceWrapped(d_Description, d_Specific, ...) DMibImpExceptionInstanceSpecific(NMib::NException::CExceptionWrapped, d_Description, d_Specific, ##__VA_ARGS__)

	class CExceptionExceptionVector;
	struct CExceptionExceptionVectorData
	{
		CExceptionExceptionVectorData() = default;
		CExceptionExceptionVectorData(NContainer::TCVector<CExceptionPointer> &&_Exceptions)
			: m_Exceptions(fg_Move(_Exceptions))
		{
		}
		
		template <typename tf_CStream>
		void f_Stream(tf_CStream &_Stream)
		{
			_Stream % m_Exceptions;
		}

		NContainer::TCVector<CExceptionPointer> m_Exceptions;

		struct CErrorCollector
		{
			void f_AddError(CExceptionPointer &&_pException);
			CExceptionPointer f_GetException() &&;
			bool f_HasError() const;

		private:
			NContainer::TCVector<CExceptionPointer> mp_Exceptions;
		};
	};

	DMibImpErrorSpecificClassDefine(CExceptionExceptionVector, CException, CExceptionExceptionVectorData);

#	define DMibErrorExceptionVector(d_Description, d_Specific, ...) DMibImpErrorSpecific(NMib::NException::CExceptionExceptionVector, d_Description, d_Specific, ##__VA_ARGS__)
#	define DMibErrorInstanceExceptionVector(d_Description, d_Specific, ...) DMibImpExceptionInstanceSpecific(NMib::NException::CExceptionExceptionVector, d_Description, d_Specific, ##__VA_ARGS__)
}

namespace NMib::NConcurrency::NPrivate
{
	void fg_FeedException(NStream::CBinaryStreamDefault &_Stream, NException::CExceptionPointer const &_pException);
	void fg_FeedException(NStream::CBinaryStreamDefault &_Stream, NException::CExceptionBase const &_pException);
	NException::CExceptionPointer fg_ConsumeException(NStream::CBinaryStreamDefault &_Stream);
}

namespace NMib::NStream
{
	template <typename t_CStream>
	class TCBinaryStreamTypeReference<t_CStream, NException::CExceptionPointer>
	{
	public:
		static constexpr void fs_Feed(t_CStream &_Stream, NException::CExceptionPointer const &_Data)
		{
			NConcurrency::NPrivate::fg_FeedException(_Stream, _Data);
		}

		static constexpr void fs_Consume(t_CStream &_Stream, NException::CExceptionPointer &_Data)
		{
			_Data = NConcurrency::NPrivate::fg_ConsumeException(_Stream);
		}
	};
}
