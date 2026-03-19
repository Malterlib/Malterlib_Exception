// Copyright © 2022 Favro Holding AB
// Distributed under the MIT license, see license text in LICENSE.Malterlib

#include <Mib/Concurrency/AsyncResult>
#include <Mib/Test/Exception>
#include <Mib/Test/Performance>

#include <exception>
#include <stdexcept>

namespace
{
	using namespace NMib;
	using namespace NMib::NStorage;
	using namespace NMib::NException;
	using namespace NMib::NTest;

	template <typename tf_FOnValid>
	bool fg_VisitExceptionDirect(CExceptionPointer const &_pException, tf_FOnValid &&_fOnValid)
	{
		CDisableExceptionFilterScope Scope;
		try
		{
			std::rethrow_exception(_pException);
		}
		catch (NException::CExceptionMemory &_Exception)
		{
			_fOnValid(fg_Move(_Exception));
			return true;
		}
		catch (NException::CException &_Exception)
		{
			_fOnValid(fg_Move(_Exception));
			return true;
		}
		catch (...)
		{
		}

		return false;
	}

	class CExceptionPointer_Tests : public CTest
	{
	public:
		void f_DoTests()
		{
			DMibTestSuite(CTestCategory("MakeException") << CTestGroup("Performance"))
			{
				constexpr umint c_nTests = 5;
#ifdef DMibDebug
				constexpr umint c_nExceptions = 10000;
#else
				constexpr umint c_nExceptions = 100000;
#endif

				CTestPerformanceMeasure SharedPtrTime("SharedPtr");
				CTestPerformanceMeasure MalterlibTime("Malterlib");
				CTestPerformanceMeasure StdTime("Std");

				for (umint i = 0; i < c_nTests; ++i)
				{
					DMibTestScopeMeasure(SharedPtrTime, c_nExceptions);
					for (umint i = 0; i < c_nExceptions; ++i)
					{
						[[maybe_unused]] TCSharedPointer<CException> pException = fg_Construct("CException", DMibPFile, DMibPLine, DMibPFunction, "Test", false);
					}
				}

				for (umint i = 0; i < c_nTests; ++i)
				{
					DMibTestScopeMeasure(MalterlibTime, c_nExceptions);
					for (umint i = 0; i < c_nExceptions; ++i)
					{
						[[maybe_unused]] auto pTest = fg_MakeException(DMibErrorInstance("Test"));
					}
				}

				for (umint i = 0; i < c_nTests; ++i)
				{
					DMibTestScopeMeasure(StdTime, c_nExceptions);
					for (umint i = 0; i < c_nExceptions; ++i)
					{
						[[maybe_unused]] auto pTest = std::make_exception_ptr(DMibErrorInstance("Test"));
					}
				}

				CTestPerformance Performing(1.0);
				Performing.f_AddBaseline(SharedPtrTime);
				Performing.f_AddReference(StdTime);
				Performing.f_Add(MalterlibTime);
				DMibTest(DMibExpr(Performing));
			};
			DMibTestSuite(CTestCategory("VisitException") << CTestGroup("Performance"))
			{
				constexpr umint c_nTests = 5;
#ifdef DMibDebug
				constexpr umint c_nExceptions = 10000;
#else
				constexpr umint c_nExceptions = 100000;
#endif

#		define DMibErrorInstanceMemory(_Description) DMibImpExceptionInstance(NMib::NException::CExceptionMemory, _Description)

				auto pException = fg_MakeException(DMibErrorInstanceMemory("Test exception"));

				CTestPerformanceMeasure DirectTime("Direct");
				CTestPerformanceMeasure MalterlibTime("Malterlib");

				for (umint i = 0; i < c_nTests; ++i)
				{
					DMibTestScopeMeasure(DirectTime, c_nExceptions);
					for (umint i = 0; i < c_nExceptions; ++i)
					{
						fg_VisitExceptionDirect
							(
								pException
								, [&](auto &&_Exception)
								{
								}
							)
						;
					}
				}

				for (umint i = 0; i < c_nTests; ++i)
				{
					DMibTestScopeMeasure(MalterlibTime, c_nExceptions);
					for (umint i = 0; i < c_nExceptions; ++i)
					{
						fg_VisitException<NException::CExceptionPureCall, NException::CExceptionMemory, NException::CException>
							(
								pException
								, [&](auto &&_Exception)
								{
								}
							)
						;
					}
				}

				CTestPerformance Performing(1.0);
				Performing.f_AddReference(DirectTime);
				Performing.f_Add(MalterlibTime);
				DMibTest(DMibExpr(Performing));
			};
		}
	};

	DMibTestRegister(CExceptionPointer_Tests, Malterlib::Exception);
}

