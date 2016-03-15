// Copyright © 2015 Hansoft AB 
// Distributed under the MIT license, see license text in LICENSE.Malterlib

#pragma once

#define DMibIncluded_Exception

#include <Mib/Core/Core>

namespace NMib
{
	namespace NException
	{
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
			NPtr::TCUniquePointer<NStr::CStr> m_pErrorAlloc;
			NPtr::TCUniquePointer<NStr::CStrNonTracked, NMem::CAllocator_NonTrackedHeap> m_pErrorAllocNonTracked;
			NPtr::TCUniquePointer<CCallstack> m_pCallstack;
			NPtr::TCUniquePointer<CCallstack, NMem::CAllocator_NonTrackedHeap> m_pCallstackNonTracked;
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
			~CExceptionBase();
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
			CCallstack const *f_GetCallstack();

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
			
#ifdef DMibRuntimeTypeRegistry
			template <typename tf_CStream>
			void f_Feed(tf_CStream &_Stream) const;
			template <typename tf_CStream>
			void f_Consume(tf_CStream &_Stream);
#endif
		};

#		ifdef DMibRuntimeTypeRegistry
#			define DMibException_TypeHash(d_Type) ::NMib::fg_GetTypeHash<d_Type>()
#			define DMibImpErrorClass_TypeRegistry(d_CClass) DMibConcurrencyRegisterException(d_CClass)
#		else
#			define DMibException_TypeHash(d_Type) 0
#			define DMibImpErrorClass_TypeRegistry(d_CClass) 
#		endif
		
		template <typename t_CTag>
		class TCException : public CExceptionBase
		{
		public:
			template <typename t_CError>
			TCException(const ch8 *_pClass, const ch8 *_pFile, aint _Line, const ch8 *_pFunction, t_CError &&_Error, bool _bTrace, bool _bStackTrace = true, uint32 _TypeHash = DMibException_TypeHash(TCException))
				: CExceptionBase
				(
					_pClass ? _pClass 
					: NTraits::TCIsSame<t_CTag, CNormalExceptionTag>::mc_Value ? "CException"
					: NTraits::TCIsSame<t_CTag, CDebugExceptionTag>::mc_Value ? "CDebugException"
					: fg_GetTypeName<TCException>()
					, _pFile
					, _Line
					, _pFunction
					, fg_Forward<t_CError>(_Error)
					, _bTrace
					, _bStackTrace
					, _TypeHash
				)
			{
				DMibImpErrorClass_TypeRegistry(TCException);
			}
			template <typename t_CError>
			TCException(const ch8 *_pClass, t_CError &&_Error, bool _bTrace, bool _bStackTrace = true, uint32 _TypeHash = DMibException_TypeHash(TCException))
				: CExceptionBase
				(
					_pClass ? _pClass 
					: NTraits::TCIsSame<t_CTag, CNormalExceptionTag>::mc_Value ? "CException"
					: NTraits::TCIsSame<t_CTag, CDebugExceptionTag>::mc_Value ? "CDebugException"
					: fg_GetTypeName<TCException>()
					, ""
					, 0
					, ""
					, fg_Forward<t_CError>(_Error)
					, _bTrace
					, _bStackTrace
					, _TypeHash
				)
			{
				DMibImpErrorClass_TypeRegistry(TCException);
			}
		};

		typedef TCException<CNormalExceptionTag> CException;
		typedef TCException<CDebugExceptionTag> CDebugException;

		bint fg_SetEnableExceptionTrace(bint _bEnabled);
		bint fg_SetGlobalEnableExceptionTrace(bint _bEnabled);

#		define DMibImpErrorInstance(d_CClass, d_Description) d_CClass(nullptr, d_Description, false)
#		define DMibImpError(d_CClass, d_Description) throw d_CClass(nullptr, DMibPFile, DMibPLine, DMibPFunction, d_Description, true)
#		define DMibImpExceptionInstance(d_CClass, d_Description) d_CClass(nullptr, DMibPFile, DMibPLine, DMibPFunction, d_Description, false)
#		define DMibImpErrorSpecific(d_CClass, d_Description, d_SpecificData) throw d_CClass(nullptr, DMibPFile, DMibPLine, DMibPFunction, d_Description, true, d_SpecificData)
#		define DMibImpExceptionInstanceSpecific(d_CClass, d_Description, d_SpecificData) d_CClass(nullptr, DMibPFile, DMibPLine, DMibPFunction, d_Description, false, d_SpecificData)

#		define DMibError(d_Description) throw NMib::NException::CException("CException", DMibPFile, DMibPLine, DMibPFunction, d_Description, true)
#		define DMibErrorInstance(d_Description) NMib::NException::CException("CException", d_Description, false)

#		ifndef DMibPNoShortCuts
#			define DExceptionInstance DMibImpExceptionInstance
#			define DError(d_Description) DMibError(d_Description)
#			define DErrorInstance(d_Description) DMibErrorInstance(d_Description)
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
		
		
#		define DMibImpErrorClass(d_CClass, d_CParent) \
			class d_CClass : public d_CParent\
			{\
			public:\
				template <typename t_CError>\
				d_CClass(const ch8 *_pClass, const ch8 *_pFile, aint _Line, const ch8 *_pFunction, t_CError &&_Error, bool _bTrace, uint32 _TypeHash = DMibException_TypeHash(d_CClass))\
					: d_CParent(_pClass ? _pClass : DMibStringize(d_CClass), _pFile, _Line, _pFunction, fg_Forward<t_CError>(_Error), _bTrace, _TypeHash)\
				{\
					DMibImpErrorClass_TypeRegistry(d_CClass);\
				}\
			};\
			

#		define DMibImpErrorSpecificClass(d_CClass, d_CParent, d_CSpecificType) \
			class d_CClass : public d_CParent\
			{\
				d_CSpecificType m_SpecificData;\
			public:\
				template <typename t_CError>\
				d_CClass(const ch8 *_pClass, const ch8 *_pFile, aint _Line, const ch8 *_pFunction, t_CError &&_Error, bool _bTrace, d_CSpecificType const &_SpecificData = fg_Default(), uint32 _TypeHash = DMibException_TypeHash(d_CClass))\
					: d_CParent(_pClass ? _pClass : DMibStringize(d_CClass), _pFile, _Line, _pFunction, fg_Forward<t_CError>(_Error), _bTrace, _TypeHash)\
					, m_SpecificData(_SpecificData)\
				{\
					DMibImpErrorClass_TypeRegistry(d_CClass);\
				}\
				d_CSpecificType const &f_GetSpecific() const\
				{\
					return m_SpecificData;\
				}\
				DMibImpErrorSpecificClass_Streaming(d_CParent)\
			};\
			
			


		/*¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯*\
		|	Class:				A memory exception										|
		\*_____________________________________________________________________________*/

		DMibImpErrorClass(CExceptionMemory, CException);
		
#		define DMibErrorMemory(_Description) DMibImpError(NMib::NException::CExceptionMemory, _Description)

#		ifndef DMibPNoShortCuts
#			define DErrorMemory(_Description) DMibErrorMemory(_Description)
#		endif

		/*¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯*\
		|	Class:				A System Implementation exception						|
		\*_____________________________________________________________________________*/

		DMibImpErrorClass(CExceptionSystemImplementation, CException);
#		define DMibErrorSystemImp(_Description) DMibImpError(NMib::NException::CExceptionSystemImplementation, _Description)

#		ifndef DMibPNoShortCuts
#			define DErrorSystemImp(_Description) DMibErrorSystemImp(_Description)
#		endif

		/*¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯*\
		|	Class:				A pure call exception									|
		\*_____________________________________________________________________________*/

		DMibImpErrorClass(CExceptionPureCall, CException);
#		define DMibErrorPureCall(_Description) DMibImpError(NMib::NException::CExceptionPureCall, _Description)

#		ifndef DMibPNoShortCuts
#			define DErrorPureCall(_Description) DMibErrorPureCall(_Description)
#		endif

		/*¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯*\
		|	Class:				A bad function call exception							|
		\*_____________________________________________________________________________*/

		DMibImpErrorClass(CExceptionBadFunctionCall, CException);
#		define DMibErrorBadFunctionCall(_Description) DMibImpError(NMib::NException::CExceptionBadFunctionCall, _Description)

#		ifndef DMibPNoShortCuts
#			define DErrorBadFunctionCall(_Description) DMibErrorBadFunctionCall(_Description)
#		endif


		/*¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯*\
		|	Class:				Safe check exception									|
		\*_____________________________________________________________________________*/

		DMibImpErrorClass(CExceptionSafeCheck, CDebugException);
#		define DMibErrorSafeCheck(_Description) DMibImpError(NMib::NException::CExceptionSafeCheck, _Description)

#		ifndef DMibPNoShortCuts
#			define DErrorSafeCheck(_Description) DMibErrorSafeCheck(_Description)
#		endif

	}
	namespace NFile
	{
		DMibImpErrorClass(CExceptionFile, NException::CException);
		
#		define DMibErrorFile(_Description) DMibImpError(NMib::NFile::CExceptionFile, _Description)

#		ifndef DMibPNoShortCuts
#			define DErrorFile(_Description) DMibErrorFile(_Description)
#		endif
	}
}
