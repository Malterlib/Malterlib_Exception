// Copyright © 2023 Favro Holding AB
// Distributed under the MIT license, see license text in LICENSE.Malterlib

#pragma once

#if defined(DMalterlibLibCxxABI) && __has_feature(cxx_rtti)
	#define DMibHasFastExceptions
#endif

namespace NMib::NException
{
	template <typename tf_CException>
	CExceptionPointer fg_MakeException(tf_CException &&_Exception) noexcept;

	template <typename ...tfp_CExceptions, typename tf_FOnValid>
	bool fg_VisitException(CExceptionPointer const &_pException, tf_FOnValid &&_fOnValid);

	template <typename tf_CException>
	bool fg_ExceptionIsOfType(CExceptionPointer const &_pException);
}
