// Copyright © 2015 Hansoft AB 
// Distributed under the MIT license, see license text in LICENSE.Malterlib

#pragma once

#define DMibIncluded_Exception

#include <exception>
#include <Mib/Core/Core>

namespace NMib::NException
{
	using CExceptionPointer = std::exception_ptr;

	inline_always int fg_UncaughtExceptions()
	{
		return std::uncaught_exceptions();
	}

	inline_always CExceptionPointer fg_CurrentException()
	{
		return std::current_exception();
	}
#ifdef DMibDebug
#define DMibExceptionTraceEnable
#endif
	class CNormalExceptionTag {};
	class CDebugExceptionTag {};
	/*¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯*\
	|	Class:				The class that all other exceptionsn derive				|
	|						from													|
	\*_____________________________________________________________________________*/

	struct CCallstack
	{
		void f_Trace(mint _Indent) const;
		NStr::CStr f_GetString(mint _Indent) const;
		NStr::CStr f_GetFunctionName(mint _iCallstack) const;
		static NStr::CStrNonTracked fs_ShortenFunctionName(ch8 const *_pFunction);


		CMibCodeAddress m_Callstack[128];
		mint m_CallstackLen;
	};

	class CExceptionBase
	{
		const static uint32 mcp_Magic = 0xE538CB10;
		uint32 m_Magic;
		const ch8 *m_pClass;
		const ch8 *m_pFile;
		const ch8 *m_pFunction;
		int32 m_Line;
		ch8 m_ErrorNoAlloc[128];
		NStorage::TCUniquePointer<NStr::CStr> m_pErrorAlloc;
		NStorage::TCUniquePointer<NStr::CStrNonTracked, NMemory::CAllocator_NonTrackedHeap> m_pErrorAllocNonTracked;
		NStorage::TCUniquePointer<CCallstack> m_pCallstack;
		NStorage::TCUniquePointer<CCallstack, NMemory::CAllocator_NonTrackedHeap> m_pCallstackNonTracked;
		uint32 m_TypeHash;

		void fp_Construct(const ch8 *_pError, bool _bTrace, bool _bStackTrace);
		void fp_ConstructNonTracked(const ch8 *_pError, bool _bTrace, bool _bStackTrace);
		void fp_Copy(const CExceptionBase &_Other);
	public:
		// If this exception could cause errors with memory management you should make sure that it's smaller than 127 characters to be able to fit in the m_ErrorNoAlloc buffer that requires no memory management.
		CExceptionBase(const ch8 *_pClass, const ch8 *_pFile, aint _Line, const ch8 *_pFunction, const ch8 *_pError, bool _bTrace, bool _bStackTrace, uint32 _TypeHash);
		CExceptionBase(const ch8 *_pClass, const ch8 *_pFile, aint _Line, const ch8 *_pFunction, NStr::CStr const &_Error, bool _bTrace, bool _bStackTrace, uint32 _TypeHash);
		CExceptionBase(const ch8 *_pClass, const ch8 *_pFile, aint _Line, const ch8 *_pFunction, NStr::CStrNonTracked const &_Error, bool _bTrace, bool _bStackTrace, uint32 _TypeHash);

		template <typename t_CError>
		CExceptionBase(const ch8 *_pClass, const ch8 *_pFile, aint _Line, const ch8 *_pFunction, NStr::TCStrAggregate<t_CError> const &_Error, bool _bTrace, bool _bStackTrace, uint32 _TypeHash);
		template <typename t_CError>
		CExceptionBase(const ch8 *_pClass, const ch8 *_pFile, aint _Line, const ch8 *_pFunction, NStr::TCStr<t_CError> const &_Error, bool _bTrace, bool _bStackTrace, uint32 _TypeHash);

		CExceptionBase(const CExceptionBase&_Other);
		CExceptionBase &operator =(const CExceptionBase&_Other);
		virtual ~CExceptionBase();
		const ch8 *f_GetErrorCharPointer() const;
		NStr::CStr f_GetErrorStr() const;
		NStr::CStrNonTracked f_GetErrorStrNonTracked() const;
		const ch8 *f_GetClass() const;
		const ch8 *f_GetFile() const;
		const ch8 *f_GetFunction() const;
		int32 f_GetLine() const;
		bint f_IsValid() const;
#ifdef DMibExceptionTraceEnable
		void f_TraceException(bool _bTrace) const;
#endif
		CCallstack const *f_GetCallstack() const;
		NStr::CStr f_GetCallstackStr(mint _Indent) const;

		template <typename t_COther>
		bint operator == (t_COther const &_Other) const
		{
			return NStr::fg_StrCmp(f_GetErrorCharPointer(), _Other.f_GetErrorCharPointer()) == 0
				&& NStr::fg_StrCmp(f_GetClass(), _Other.f_GetClass()) == 0
			;
		}

		template <typename t_CFormatter>
		int f_GetStringFormatType(t_CFormatter &_Formatter);
		template <typename t_CFormatter>
		// This crashes GCC
		// auto f_CreateStringFormatter(t_CFormatter &_Formatter) const -> decltype(NStr::fg_CreateStringFormatter(_Formatter, ""))
		NStr::CStrFormatTypeClassifier_String f_CreateStringFormatter(t_CFormatter &_Formatter) const;

		inline_always_debug uint32 f_TypeHash() const;

		virtual CExceptionPointer f_ExceptionPointer() const = 0;

#ifdef DMibRuntimeTypeRegistry
		template <typename tf_CStream>
		void f_Feed(tf_CStream &_Stream) const;
		template <typename tf_CStream>
		void f_Consume(tf_CStream &_Stream);
#endif
	};

	template <typename t_CException>
	struct TCIsExcption : public NTraits::TCCompileTimeConstant<bool, NTraits::TCIsBaseOf<typename NTraits::TCRemoveReference<t_CException>::CType, CExceptionBase>::mc_Value>
	{
	};

	template <typename tf_CException, TCEnableIfType<NTraits::TCIsBaseOf<typename NTraits::TCRemoveReference<tf_CException>::CType, CExceptionBase>::mc_Value> * = nullptr>
	CExceptionPointer fg_ExceptionPointer(tf_CException &&_Exception)
	{
		return _Exception.f_ExceptionPointer();
	}

	NStr::CStr fg_ExceptionString(CExceptionPointer const &_pExceptionPointer);

	template <typename tf_CException, TCEnableIfType<!NTraits::TCIsBaseOf<typename NTraits::TCRemoveReference<tf_CException>::CType, CExceptionBase>::mc_Value> * = nullptr>
	CExceptionPointer fg_ExceptionPointer(tf_CException &&_Exception)
	{
		return std::make_exception_ptr(fg_Forward<tf_CException>(_Exception));
	}

#		ifdef DMibRuntimeTypeRegistry
#			define DMibException_TypeHash(d_Type) DMibConstantTypeHash(d_Type)
#			define DMibImpErrorClass_TypeRegistry(d_CClass) DMibConcurrencyRegisterException(d_CClass)
#		else
#			define DMibException_TypeHash(d_Type) 0
#			define DMibImpErrorClass_TypeRegistry(d_CClass) 
#		endif

	class CException : public CExceptionBase
	{
	public:
		template <typename t_CError>
		CException(const ch8 *_pClass, const ch8 *_pFile, aint _Line, const ch8 *_pFunction, t_CError &&_Error, bool _bTrace, bool _bStackTrace = true, uint32 _TypeHash = ms_TypeHash)
			: CExceptionBase
			(
				_pClass ? _pClass : "CException"
				, _pFile
				, _Line
				, _pFunction
				, fg_Forward<t_CError>(_Error)
				, _bTrace
				, _bStackTrace
				, _TypeHash
			)
		{
			fp_RegisterTypeRegistry();
		}
		template <typename t_CError>
		CException(const ch8 *_pClass, t_CError &&_Error, bool _bTrace, bool _bStackTrace = true, uint32 _TypeHash = ms_TypeHash)
			: CExceptionBase
			(
				_pClass ? _pClass : "CException"
				, ""
				, 0
				, ""
				, fg_Forward<t_CError>(_Error)
				, _bTrace
				, _bStackTrace
				, _TypeHash
			)
		{
			fp_RegisterTypeRegistry();
		}

		CExceptionPointer f_ExceptionPointer() const override;

		static uint32 ms_TypeHash;
	private:
		void fp_RegisterTypeRegistry() const;
	};

	class CDebugException : public CExceptionBase
	{
	public:
		template <typename t_CError>
		CDebugException(const ch8 *_pClass, const ch8 *_pFile, aint _Line, const ch8 *_pFunction, t_CError &&_Error, bool _bTrace, bool _bStackTrace = true, uint32 _TypeHash = ms_TypeHash)
			: CExceptionBase
			(
				_pClass ? _pClass : "CDebugException"
				, _pFile
				, _Line
				, _pFunction
				, fg_Forward<t_CError>(_Error)
				, _bTrace
				, _bStackTrace
				, _TypeHash
			)
		{
			fp_RegisterTypeRegistry();
		}
		template <typename t_CError>
		CDebugException(const ch8 *_pClass, t_CError &&_Error, bool _bTrace, bool _bStackTrace = true, uint32 _TypeHash = ms_TypeHash)
			: CExceptionBase
			(
				_pClass ? _pClass : "CDebugException"
				, ""
				, 0
				, ""
				, fg_Forward<t_CError>(_Error)
				, _bTrace
				, _bStackTrace
				, _TypeHash
			)
		{
			fp_RegisterTypeRegistry();
		}

		CExceptionPointer f_ExceptionPointer() const override;

		static uint32 ms_TypeHash;
	private:
		void fp_RegisterTypeRegistry() const;
	};

	bint fg_SetEnableExceptionTrace(bint _bEnabled);
	bint fg_SetGlobalEnableExceptionTrace(bint _bEnabled);

#ifdef DMibExceptionTraceEnable
	struct CDisableExceptionTraceScope final : public CCoroutineThreadLocalHandler
	{
		inline CDisableExceptionTraceScope();
		inline ~CDisableExceptionTraceScope();
		void f_Suspend() override;
		void f_Resume() override;

	private:
		bint mp_bOldEnable;
	};
#else
	struct CDisableExceptionTraceScope
	{
		inline CDisableExceptionTraceScope();
		inline ~CDisableExceptionTraceScope();
	};
#endif

#		define DMibImpErrorInstance(d_CClass, d_Description, ...) d_CClass(nullptr, DMibPFile, DMibPLine, DMibPFunction, d_Description, false, ##__VA_ARGS__)
#		define DMibImpError(d_CClass, d_Description, ...) throw d_CClass(nullptr, DMibPFile, DMibPLine, DMibPFunction, d_Description, true, ##__VA_ARGS__)
#		define DMibImpExceptionInstance(d_CClass, d_Description, ...) d_CClass(nullptr, DMibPFile, DMibPLine, DMibPFunction, d_Description, false, ##__VA_ARGS__)
#		define DMibImpErrorSpecific(d_CClass, d_Description, d_SpecificData, ...) throw d_CClass(nullptr, DMibPFile, DMibPLine, DMibPFunction, d_Description, true, d_SpecificData, ##__VA_ARGS__)
#		define DMibImpExceptionInstanceSpecific(d_CClass, d_Description, d_SpecificData, ...) d_CClass(nullptr, DMibPFile, DMibPLine, DMibPFunction, d_Description, false, d_SpecificData, ##__VA_ARGS__)

#		define DMibError(d_Description, ...) throw NMib::NException::CException("CException", DMibPFile, DMibPLine, DMibPFunction, d_Description, true, ##__VA_ARGS__)
#		define DMibErrorInstance(d_Description, ...) NMib::NException::CException("CException", DMibPFile, DMibPLine, DMibPFunction, d_Description, false, ##__VA_ARGS__)

#		ifndef DMibPNoShortCuts
#			define DExceptionInstance DMibImpExceptionInstance
#			define DError DMibError
#			define DErrorInstance DMibErrorInstance
#		endif

#		ifdef DMibRuntimeTypeRegistry

#			define DMibImpErrorSpecificClass_Streaming(d_CParent) \
			template <typename tf_CStream>\
			void f_Feed(tf_CStream &_Stream) const\
			{\
				d_CParent::f_Feed(_Stream);\
				_Stream << m_SpecificData;\
			}\
			template <typename tf_CStream>\
			void f_Consume(tf_CStream &_Stream)\
			{\
				d_CParent::f_Consume(_Stream);\
				_Stream >> m_SpecificData;\
			}

#		else
#			define DMibImpErrorSpecificClass_Streaming(d_CParent)
#		endif


#		define DMibImpErrorClassDefine(d_CClass, d_CParent) \
		class d_CClass : public d_CParent\
		{\
		public:\
			template <typename t_CError>\
			d_CClass(const ch8 *_pClass, const ch8 *_pFile, aint _Line, const ch8 *_pFunction, t_CError &&_Error, bool _bTrace, bool _bStackTrace = true, uint32 _TypeHash = ms_TypeHash)\
				: d_CParent(_pClass ? _pClass : DMibStringize(d_CClass), _pFile, _Line, _pFunction, fg_Forward<t_CError>(_Error), _bTrace, _bStackTrace, _TypeHash)\
			{\
				fp_RegisterTypeRegistry();\
			}\
			template <typename t_CError>\
			d_CClass(const ch8 *_pClass, t_CError &&_Error, bool _bTrace, bool _bStackTrace = true, uint32 _TypeHash = ms_TypeHash)\
				: d_CParent(_pClass ? _pClass : DMibStringize(d_CClass), fg_Forward<t_CError>(_Error), _bTrace, _bStackTrace, _TypeHash)\
			{\
				fp_RegisterTypeRegistry();\
			}\
			NMib::NException::CExceptionPointer f_ExceptionPointer() const override;\
			static uint32 ms_TypeHash;\
		private:\
			void fp_RegisterTypeRegistry() const;\
		};\

#		define DMibImpErrorClassImplement(d_CClass) \
			uint32 d_CClass::ms_TypeHash = DMibException_TypeHash(d_CClass);\
			NMib::NException::CExceptionPointer d_CClass::f_ExceptionPointer() const\
			{\
				return std::make_exception_ptr(*this);\
			}\
			void d_CClass::fp_RegisterTypeRegistry() const\
			{\
				DMibImpErrorClass_TypeRegistry(d_CClass);\
			}\


#		define DMibImpErrorSpecificClassDefine(d_CClass, d_CParent, d_CSpecificType) \
		class d_CClass : public d_CParent\
		{\
			d_CSpecificType m_SpecificData;\
		public:\
			template <typename t_CError>\
			d_CClass(const ch8 *_pClass, const ch8 *_pFile, aint _Line, const ch8 *_pFunction, t_CError &&_Error, bool _bTrace, d_CSpecificType const &_SpecificData = fg_Default(), bool _bStackTrace = true, uint32 _TypeHash = ms_TypeHash)\
				: d_CParent(_pClass ? _pClass : DMibStringize(d_CClass), _pFile, _Line, _pFunction, fg_Forward<t_CError>(_Error), _bTrace, _bStackTrace, _TypeHash)\
				, m_SpecificData(_SpecificData)\
			{\
				fp_RegisterTypeRegistry();\
			}\
			template <typename t_CError>\
			d_CClass(const ch8 *_pClass, t_CError &&_Error, bool _bTrace, d_CSpecificType const &_SpecificData = fg_Default(), bool _bStackTrace = true, uint32 _TypeHash = ms_TypeHash)\
				: d_CParent(_pClass ? _pClass : DMibStringize(d_CClass), fg_Forward<t_CError>(_Error), _bTrace, _bStackTrace, _TypeHash)\
				, m_SpecificData(_SpecificData)\
			{\
				fp_RegisterTypeRegistry();\
			}\
			NMib::NException::CExceptionPointer f_ExceptionPointer() const override;\
			d_CSpecificType const &f_GetSpecific() const\
			{\
				return m_SpecificData;\
			}\
			DMibImpErrorSpecificClass_Streaming(d_CParent)\
			static uint32 ms_TypeHash;\
		private:\
			void fp_RegisterTypeRegistry() const;\
		};\

	/*¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯*\
	|	Class:				A memory exception										|
	\*_____________________________________________________________________________*/

	DMibImpErrorClassDefine(CExceptionMemory, CException);

#		define DMibErrorMemory(_Description) DMibImpError(NMib::NException::CExceptionMemory, _Description)

#		ifndef DMibPNoShortCuts
#			define DErrorMemory(_Description) DMibErrorMemory(_Description)
#		endif

	/*¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯*\
	|	Class:				A System Implementation exception						|
	\*_____________________________________________________________________________*/

	DMibImpErrorClassDefine(CExceptionSystemImplementation, CException);
#		define DMibErrorSystemImp(_Description) DMibImpError(NMib::NException::CExceptionSystemImplementation, _Description)

#		ifndef DMibPNoShortCuts
#			define DErrorSystemImp(_Description) DMibErrorSystemImp(_Description)
#		endif

	/*¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯*\
	|	Class:				A pure call exception									|
	\*_____________________________________________________________________________*/

	DMibImpErrorClassDefine(CExceptionPureCall, CException);
#		define DMibErrorPureCall(_Description) DMibImpError(NMib::NException::CExceptionPureCall, _Description)

#		ifndef DMibPNoShortCuts
#			define DErrorPureCall(_Description) DMibErrorPureCall(_Description)
#		endif

	/*¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯*\
	|	Class:				A bad function call exception							|
	\*_____________________________________________________________________________*/

	DMibImpErrorClassDefine(CExceptionBadFunctionCall, CException);
#		define DMibErrorBadFunctionCall(_Description) DMibImpError(NMib::NException::CExceptionBadFunctionCall, _Description)

#		ifndef DMibPNoShortCuts
#			define DErrorBadFunctionCall(_Description) DMibErrorBadFunctionCall(_Description)
#		endif


	/*¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯*\
	|	Class:				Safe check exception									|
	\*_____________________________________________________________________________*/

	DMibImpErrorClassDefine(CExceptionSafeCheck, CDebugException);
#		define DMibErrorSafeCheck(_Description) DMibImpError(NMib::NException::CExceptionSafeCheck, _Description)

#		ifndef DMibPNoShortCuts
#			define DErrorSafeCheck(_Description) DMibErrorSafeCheck(_Description)
#		endif

}

namespace NMib::NFile
{
	DMibImpErrorClassDefine(CExceptionFile, NException::CException);

#		define DMibErrorFile(_Description) DMibImpError(NMib::NFile::CExceptionFile, _Description)

#		ifndef DMibPNoShortCuts
#			define DErrorFile(_Description) DMibErrorFile(_Description)
#		endif
}

#ifndef DMibPNoShortCuts
	using namespace NMib::NException;
#endif
