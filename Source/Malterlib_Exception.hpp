// Copyright © Unbroken AB
// SPDX-License-Identifier: Apache-2.0 WITH LLVM-exception

#pragma once

#include <Mib/Core/Core>

namespace NMib::NConcurrency::NPrivate
{
	void fg_FeedException(NStream::CBinaryStreamDefault &_Stream, NException::CExceptionPointer const &_pException, bool _bVersioned);
	NException::CExceptionPointer fg_ConsumeException(NStream::CBinaryStreamDefault &_Stream, bool _bVersioned);
}

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
	CExceptionBase::CExceptionBase
		(
			ch8 const *_pClass
			, ch8 const *_pFile
			, aint _Line
			, ch8 const *_pFunction
			, NStr::TCStr<t_CError> const &_Error
			, bool _bTrace
			, bool _bStackTrace
			, uint32 _TypeHash
		)
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

		static constexpr uint32 mc_StreamVersion = 1; // 1: m_pWrapped with its own stream version

		// From version 1 the reader has exception stream versions, so the wrapped exception streams with its own version too
		template <typename tf_CStream>
		void f_Stream(tf_CStream &_Stream)
		{
			bool bVersioned = _Stream.f_GetVersion() >= 1;

			if constexpr (tf_CStream::mc_bFeed)
				NConcurrency::NPrivate::fg_FeedException(_Stream, m_pWrapped, bVersioned);
			else
				m_pWrapped = NConcurrency::NPrivate::fg_ConsumeException(_Stream, bVersioned);
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

		static constexpr uint32 mc_StreamVersion = 1; // 1: m_Exceptions with their own stream versions

		// From version 1 the reader has exception stream versions, so each exception streams with its own version too
		template <typename tf_CStream>
		void f_Stream(tf_CStream &_Stream)
		{
			bool bVersioned = _Stream.f_GetVersion() >= 1;

			if constexpr (tf_CStream::mc_bFeed)
			{
				NStream::fg_FeedLenToStream(_Stream, m_Exceptions.f_GetLen());
				for (auto &pException : m_Exceptions)
					NConcurrency::NPrivate::fg_FeedException(_Stream, pException, bVersioned);
			}
			else
			{
				uint64 nExceptions = 0;
				NStream::fg_ConsumeLenFromStream(_Stream, nExceptions);
				NStream::fg_CheckLengthLimit(_Stream, nExceptions);

				m_Exceptions.f_Clear();
				for (uint64 iException = 0; iException < nExceptions; ++iException)
					m_Exceptions.f_Insert(NConcurrency::NPrivate::fg_ConsumeException(_Stream, bVersioned));
			}
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
#	define DMibErrorInstanceExceptionVector(d_Description, d_Specific, ...) \
			DMibImpExceptionInstanceSpecific(NMib::NException::CExceptionExceptionVector, d_Description, d_Specific, ##__VA_ARGS__)
}

namespace NMib::NException
{
	/// An exception pointer that streams with the exception's stream version by default, so the reader gets the fields added in newer
	/// versions. Readers without exception stream versions, and data streamed before, misread that. When the reader can be older, take
	/// _bVersioned from the containing data's stream version, on both sides: the stream doesn't say which format it holds.
	struct CStreamableExceptionPointer
	{
		CStreamableExceptionPointer() = default;
		explicit CStreamableExceptionPointer(bool _bVersioned)
			: m_bVersioned(_bVersioned)
		{
		}

		CStreamableExceptionPointer(CExceptionPointer const &_pException, bool _bVersioned = true)
			: m_pException(_pException)
			, m_bVersioned(_bVersioned)
		{
		}

		CStreamableExceptionPointer(CExceptionPointer &&_pException, bool _bVersioned = true)
			: m_pException(fg_Move(_pException))
			, m_bVersioned(_bVersioned)
		{
		}

		CExceptionPointer m_pException;
		bool m_bVersioned = true;
	};
}

namespace NMib::NStream
{
	template <typename t_CStream>
	class TCBinaryStreamTypeReference<t_CStream, NException::CStreamableExceptionPointer>
	{
	public:
		static void fs_Feed(t_CStream &_Stream, NException::CStreamableExceptionPointer const &_Data)
		{
			NConcurrency::NPrivate::fg_FeedException(_Stream, _Data.m_pException, _Data.m_bVersioned);
		}

		static void fs_Consume(t_CStream &_Stream, NException::CStreamableExceptionPointer &_Data)
		{
			_Data.m_pException = NConcurrency::NPrivate::fg_ConsumeException(_Stream, _Data.m_bVersioned);
		}
	};

	// A plain exception pointer has no stream format, because the exception's own fields change between versions and only the
	// containing data knows whether its reader has exception stream versions. Stream a CStreamableExceptionPointer instead. When the
	// reader can be older, give the containing data an mc_StreamVersion and pass whether that version has exception stream versions
	// as its _bVersioned, or to NConcurrency::NPrivate::fg_FeedException and fg_ConsumeException like CExceptionWrappedData::f_Stream.
	// Distributed actor calls stream their exceptions with the protocol version and need neither.
	template <typename t_CStream>
	class TCBinaryStreamTypeReference<t_CStream, NException::CExceptionPointer>
	{
	public:
		template <typename tf_CData>
		static void fs_Feed(t_CStream &_Stream, tf_CData &&_Data)
		{
			static_assert
				(
					false
					, "CExceptionPointer can't be streamed directly. Use NException::CStreamableExceptionPointer or version the containing data,"
					" see the comment on TCBinaryStreamTypeReference<t_CStream, NException::CExceptionPointer>"
				)
			;
		}

		template <typename tf_CData>
		static void fs_Consume(t_CStream &_Stream, tf_CData &_Data)
		{
			static_assert
				(
					false
					, "CExceptionPointer can't be streamed directly. Use NException::CStreamableExceptionPointer or version the containing data,"
					" see the comment on TCBinaryStreamTypeReference<t_CStream, NException::CExceptionPointer>"
				)
			;
		}
	};
}
