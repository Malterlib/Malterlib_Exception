// Copyright © Unbroken AB
// SPDX-License-Identifier: Apache-2.0 WITH LLVM-exception

#pragma once

#ifdef DMibHasFastExceptions
extern "C"
{
	void *__cxa_allocate_exception(size_t thrown_size) throw();
	void __cxa_make_exception_ptr(void *thrown_object, std::type_info const *tinfo, void (*dest)(void *));
	bool __cxa_can_catch(void *thrown_object, std::type_info const *tinfo);
}
#endif

namespace NMib::NException
{
#ifdef DMibHasFastExceptions
	template <typename tf_CException>
	CExceptionPointer fg_MakeException(tf_CException &&_Exception) noexcept
	{
		using CException = NTraits::TCRemoveReferenceAndQualifiers<tf_CException>;

		auto *pExceptionMemory = __cxa_allocate_exception(sizeof(CException));

		new (pExceptionMemory) CException(fg_Forward<tf_CException>(_Exception));

		__cxa_make_exception_ptr
			(
				pExceptionMemory
				, &typeid(CException)
				, [](void *_pMemory)
				{
					((CException *)_pMemory)->~CException();
				}
			)
		;

		std::exception_ptr ptr;
		*((void **)&ptr) = pExceptionMemory;

		return ptr;
	}

	template <typename ...tfp_CExceptions, typename tf_FOnValid>
	bool fg_VisitException(CExceptionPointer const &_pException, tf_FOnValid &&_fOnValid)
	{
		void *pException = *((void **)&_pException);
		bool bHandled =
			(
				[&]
				{
					if (__cxa_can_catch(pException, &typeid(tfp_CExceptions)))
					{
						_fOnValid(fg_Move(*((tfp_CExceptions *)pException)));
						return true;
					}

					return false;
				}
				() || ...
			)
		;

		return bHandled;
	}

	template <typename tf_CException>
	bool fg_ExceptionIsOfType(CExceptionPointer const &_pException)
	{
		void *pException = *((void **)&_pException);
		return __cxa_can_catch(pException, &typeid(tf_CException));
	}
#else
	template <typename tf_CException>
	CExceptionPointer fg_MakeException(tf_CException &&_Exception) noexcept
	{
		CDisableExceptionFilterScope DisableExceptionFilter;
		return std::make_exception_ptr(fg_Forward<tf_CException>(_Exception));
	}

	namespace NPrivate
	{
		template <typename tf_FOnValid>
		bool fg_VisitExceptionHelper(CExceptionPointer const &_pException, tf_FOnValid &&_fOnValid, NMeta::TCTypeList<>)
		{
			std::rethrow_exception(_pException);
			return false;
		}

		template <typename tf_CFirstException, typename ...tfp_CExceptions, typename tf_FOnValid>
		bool fg_VisitExceptionHelper(CExceptionPointer const &_pException, tf_FOnValid &&_fOnValid, NMeta::TCTypeList<tf_CFirstException, tfp_CExceptions...>)
		{
			try
			{
				return fg_VisitExceptionHelper<tfp_CExceptions...>(_pException, _fOnValid, NMeta::TCTypeList<tfp_CExceptions...>());
			}
			catch (tf_CFirstException &_Exception)
			{
				_fOnValid(fg_Move(_Exception));
				return true;
			}
			return false;
		}
	}

	template <typename ...tfp_CExceptions, typename tf_FOnValid>
	bool fg_VisitException(CExceptionPointer const &_pException, tf_FOnValid &&_fOnValid)
	{
		CDisableExceptionFilterScope DisableExceptionFilter;
		try
		{
			return NPrivate::fg_VisitExceptionHelper(_pException, _fOnValid, NMeta::TCReverseTemplateArguments<NMeta::TCTypeList<tfp_CExceptions...>>());
		}
		catch (...)
		{
		}

		return false;
	}

	template <typename tf_CException>
	bool fg_ExceptionIsOfType(CExceptionPointer const &_pException)
	{
		CDisableExceptionFilterScope DisableExceptionFilter;
		try
		{
			std::rethrow_exception(_pException);
		}
		catch (tf_CException const &)
		{
			return true;
		}
		catch (...)
		{
		}
		return false;
	}
#endif
}
