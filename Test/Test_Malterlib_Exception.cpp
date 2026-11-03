// Copyright © Unbroken AB
// SPDX-License-Identifier: Apache-2.0 WITH LLVM-exception

#ifdef DCompiler_MSVC
#	pragma warning(push,4) // force inline not inlined
#	pragma warning(default:4714) // force inline not inlined
#endif

#include <Mib/Concurrency/AsyncResult>
#include <Mib/Test/Exception>
#include <Mib/Test/Performance>
#include <Mib/Time/PerfTimeMeasure>

namespace NMib::NException
{
	template <typename t_CCatch, typename t_CLambda, typename t_CParent>
	struct TCExceptionalCatch : public t_CParent
	{
		t_CLambda m_Lambda;
		inline_always TCExceptionalCatch(t_CLambda _Lambda, t_CParent _Parent)
			: t_CParent(_Parent)
			, m_Lambda(_Lambda)
		{
		}

		template <typename tf_CCatch, typename tf_CLambda>
		inline_always TCExceptionalCatch<tf_CCatch, tf_CLambda, TCExceptionalCatch> f_Catch(tf_CLambda _Lambda) const
		{
			return TCExceptionalCatch<tf_CCatch, tf_CLambda, TCExceptionalCatch>(_Lambda, *this);
		}

		inline_always void f_Execute() const
		{
			try
			{
				t_CParent::f_Execute();
			}
			catch (t_CCatch const &_Exception)
			{
				m_Lambda(_Exception);
			}
		}
	};

	template <typename t_CLambda>
	struct TCExceptional
	{
		t_CLambda m_Lambda;
		inline_always TCExceptional(t_CLambda _Lambda)
			: m_Lambda(_Lambda)
		{
		}

		template <typename tf_CCatch, typename tf_CLambda>
		inline_always TCExceptionalCatch<tf_CCatch, tf_CLambda, TCExceptional> f_Catch(tf_CLambda _Lambda) const
		{
			return TCExceptionalCatch<tf_CCatch, tf_CLambda, TCExceptional>(_Lambda, *this);
		}

		inline_always void f_Execute() const
		{
			m_Lambda();
		}
	};

	template <typename tf_CLambda>
	inline_always TCExceptional<tf_CLambda> fg_Try(tf_CLambda const &_Lambda)
	{
		return TCExceptional<tf_CLambda>(_Lambda);
	}

	template <typename tf_CExceptional>
	inline_always void fg_Execute(tf_CExceptional const &_Lambda)
	{
		_Lambda.f_Execute();
	}
}
#ifdef DCompiler_MSVC
#	pragma warning(pop) // force inline not inlined
#endif

namespace
{
	using namespace NMib::NConcurrency;

	struct CExceptions_Tests : public NMib::NTest::CTest
	{
		void f_DoTests()
		{
			// << CTestGroup("Unfinished")
			DMibTestCategoryFlags(CTestCategory("Exception lambdas"), ETestCategoryFlag_DisableExceptionFilter | ETestCategoryFlag_Tests)
			{
				{
					int volatile bThrowException = false;
					bool bExceptionCaught = false;

					NMib::NTime::CPrefCyclesTimeMeasureMin ExceptionsLambdaTime;
					NMib::NTime::CPrefCyclesTimeMeasureMin ExceptionsTime;
					NMib::NTime::CPrefCyclesTimeMeasureMin NoExceptionsTime;
					for (umint i = 0; i < 10; ++i)
					{
						ExceptionsLambdaTime.f_Start();
						for (umint i = 0; i < 10000; ++i)
						{
							NMib::NException::fg_Try
								(
									[&]()
									{
										if (bThrowException)
											throw int(30);
									}
								)
								.f_Catch<int>
								(
									[&](int _Exception)
									{
										bExceptionCaught = true;
									}
								)
								.f_Catch<float>
								(
									[&](float _Exception)
									{
										bExceptionCaught = true;
									}
								)
								.f_Execute()
							;
						}
						ExceptionsLambdaTime.f_Stop();
					}
					DMibTest(DMibExpr(bExceptionCaught) == DMibExpr(false));

					for (umint i = 0; i < 10; ++i)
					{
						ExceptionsTime.f_Start();
						for (umint i = 0; i < 10000; ++i)
						{
							try
							{
								if (bThrowException)
									throw int(30);
							}
							catch (int)
							{
								bExceptionCaught = true;
							}
							catch (float)
							{
								bExceptionCaught = true;
							}
						}
						ExceptionsTime.f_Stop();
					}

					for (umint i = 0; i < 10; ++i)
					{
						NoExceptionsTime.f_Start();
						for (umint i = 0; i < 10000; ++i)
						{
							if (bThrowException)
								throw int(30);
						}
						NoExceptionsTime.f_Stop();
					}

					DMibTest(DMibExpr(bExceptionCaught) == DMibExpr(false) && DMibExpr(2));
					/*
					ExceptionsLambdaTime /= 10000;
					ExceptionsTime /= 10000;
					NoExceptionsTime /= 10000;
					DMibTest(DMibExpr(ExceptionsLambdaTime) / DMibExpr(ExceptionsTime) >= DMibExpr(1.0));
					DMibTest(DMibExpr(ExceptionsTime) / DMibExpr(NoExceptionsTime) >= DMibExpr(1.0));*/
				}
				{
					bool bExceptionCaught = false;
					NMib::NException::fg_Try
						(
							[&]()
							{
								throw int(30);
							}
						)
						.f_Catch<int>
						(
							[&](int _Exception)
							{
								bExceptionCaught = true;
								// DMibTrace("{}\r\n", _Exception);
								// throw;
							}
						)
						.f_Execute()
					;

					DMibTest(DMibExpr(bExceptionCaught) == DMibExpr(true));
				}
				{
					bool bExceptionCaught = false;
					NMib::NException::fg_Try
						(
							[&]()
							{
								throw float(30);
							}
						)
						.f_Catch<int>
						(
							[&](int _Exception)
							{
								bExceptionCaught = true;
								// DMibTrace("{}\r\n", _Exception);
								// throw;
							}
						)
						.f_Catch<float>
						(
							[&](float _Exception)
							{
								bExceptionCaught = true;
								// DMibTrace("{}\r\n", _Exception);
								// throw;
							}
						)
						.f_Execute()
					;

					DMibTest(DMibExpr(bExceptionCaught) == DMibExpr(true) && DMibExpr(2));
				}
				{
					int ExceptionCaught = false;
					NMib::NException::fg_Try
						(
							[&]()
							{
								throw int(30);
							}
						)
						.f_Catch<int>
						(
							[&](int _Exception)
							{
								++ExceptionCaught;
								throw;
							}
						)
						.f_Catch<int>
						(
							[&](int _Exception)
							{
								++ExceptionCaught;
								//throw;
							}
						)
						.f_Execute()
					;

					DMibTest(DMibExpr(ExceptionCaught) == DMibExpr(2));
				}
			};
			DMibTestSuite("General")
			{
				TCAsyncResult<int> Result;
				try
				{
					DMibError("Test");
					Result.f_SetResult(1);
				}
				catch (NMib::NException::CException const &)
				{
					Result.f_SetCurrentException();
				}

				DMibTest(!DMibExpr(Result));

				DMibExpectException(Result.f_Get(), DMibErrorInstance("Test"));

				TCAsyncResult<int> NewResult = Result;

				DMibExpectException(NewResult.f_Get(), DMibErrorInstance("Test"));
			};
			DMibTestSuite(CTestCategory("GetExceptionString") << CTestGroup("Performance"))
			{
				CTestPerformance PerfTotal(0.75, false);
				umint nIterations = 11;
#ifdef DMibDebug
				umint nTests = 10000;
#else
				umint nTests = 100000;
#endif
				CAsyncResult Result;
				Result.f_SetException(fg_MakeException(DMibErrorInstance("Test error")));
				{
					CTestPerformanceMeasure Measure("Malterlib");
					for (umint i = 0; i < nIterations; ++i)
					{
						Measure.f_Start();
						[&]() inline_never
						{
							for (umint i = 0; i < nTests; ++i)
								Result.f_GetExceptionStr();
						}
						();
						Measure.f_Stop(nTests);
					}
					PerfTotal.f_Add(Measure);
				}
				DMibExpectTrue(PerfTotal);
			};
		}
	};

	DMibTestRegister(CExceptions_Tests, Malterlib::Exception);
}

