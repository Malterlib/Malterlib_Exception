// Copyright © 2015 Hansoft AB 
// Distributed under the MIT license, see license text in LICENSE.Malterlib

#ifdef DCompiler_MSVC
#pragma warning(push,4) // force inline not inlined
#pragma warning(default:4714) // force inline not inlined
#endif

#include <Mib/Concurrency/AsyncResult>
#include <Mib/Test/Exception>
#include <Mib/Test/Performance>

namespace NMib::NException
{
	template <typename t_CCatch, typename t_CLambda, typename t_CParent>
	class TCExceptionalCatch : public t_CParent
	{
	public:

		t_CLambda m_Lambda;
		inline_always TCExceptionalCatch(t_CLambda _Lambda, t_CParent _Parent)
			: t_CParent(_Parent)
			, m_Lambda(_Lambda)
		{
		}

		template <typename t_CCatch2, typename t_CLambda2>
		inline_always TCExceptionalCatch<t_CCatch2, t_CLambda2, TCExceptionalCatch> f_Catch(t_CLambda2 _Lambda) const
		{
			return TCExceptionalCatch<t_CCatch2, t_CLambda2, TCExceptionalCatch>(_Lambda, *this);
		}

		inline_always void f_Execute() const
		{
			try
			{
				t_CParent::f_Execute();
			}
			catch (const t_CCatch &_Exception)
			{
				m_Lambda(_Exception);
			}
		}
	};

	template <typename t_CLambda>
	class TCExceptional
	{
	public:

		t_CLambda m_Lambda;
		inline_always TCExceptional(t_CLambda _Lambda)
			: m_Lambda(_Lambda)
		{
		}

		template <typename t_CCatch2, typename t_CLambda2>
		inline_always TCExceptionalCatch<t_CCatch2, t_CLambda2, TCExceptional> f_Catch(t_CLambda2 _Lambda) const
		{
			return TCExceptionalCatch<t_CCatch2, t_CLambda2, TCExceptional>(_Lambda, *this);
		}

		inline_always void f_Execute() const
		{
			m_Lambda();
		}
	};

	template <typename t_CLambda>
	inline_always TCExceptional<t_CLambda> fg_Try(const t_CLambda &_Lambda)
	{
		return TCExceptional<t_CLambda>(_Lambda);
	}

	template <typename t_CExceptional>
	inline_always void fg_Execute(const t_CExceptional &_Lambda)
	{
		_Lambda.f_Execute();
	}
}
#ifdef DCompiler_MSVC
#pragma warning(pop) // force inline not inlined
#endif

namespace
{
	using namespace NMib::NConcurrency;

	class CExceptions_Tests : public NMib::NTest::CTest
	{
	public:
		void f_DoTests()
		{
			//  << CTestGroup("Unfinished")
			DMibTestCategoryFlags(CTestCategory("Exception lambdas"), ETestCategoryFlag_DisableExceptionFilter | ETestCategoryFlag_Tests)
			{
				{
					volatile int bThrowExcption = false;
					bool bExceptionCaught = false;

					NMib::NTime::CCyclesMin ExceptionsLambdaTime;
					NMib::NTime::CCyclesMin ExceptionsTime;
					NMib::NTime::CCyclesMin NoExceptionsTime;
					for (mint i = 0; i < 10; ++i)
					{
						ExceptionsLambdaTime.f_Start();
						for (mint i = 0; i < 10000; ++i)
						{
							NMib::NException::fg_Try
							(
								[&] ()
								{
									if (bThrowExcption)
										throw int(30);
								}
							)
							.f_Catch<int>
							(
								[&] (int _Exception)
								{
									bExceptionCaught = true;
								}
							)
							.f_Catch<float>
							(
								[&] (float _Exception)
								{
									bExceptionCaught = true;
								}
							)
							.f_Execute();
						}
						ExceptionsLambdaTime.f_Stop();
					}
					DMibTest(DMibExpr(bExceptionCaught) == DMibExpr(false));

					for (mint i = 0; i < 10; ++i)
					{
						ExceptionsTime.f_Start();
						for (mint i = 0; i < 10000; ++i)
						{
							try
							{
								if (bThrowExcption)
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

					for (mint i = 0; i < 10; ++i)
					{
						NoExceptionsTime.f_Start();
						for (mint i = 0; i < 10000; ++i)
						{
							if (bThrowExcption)
								throw int(30);
						}
						NoExceptionsTime.f_Stop();
					}
					

					DMibTest(DMibExpr(bExceptionCaught) == DMibExpr(false) && DMibExpr(2));
/*					ExceptionsLambdaTime /= 10000;
					ExceptionsTime /= 10000;
					NoExceptionsTime /= 10000;
					DMibTest(DMibExpr(ExceptionsLambdaTime) / DMibExpr(ExceptionsTime) >= DMibExpr(1.0));
					DMibTest(DMibExpr(ExceptionsTime) / DMibExpr(NoExceptionsTime) >= DMibExpr(1.0));*/
				}
				{
					bool bExceptionCaught = false;
					NMib::NException::fg_Try
					(
						[&] ()
						{
							throw int(30);
						}
					)
					.f_Catch<int>
					(
						[&] (int _Exception)
						{
							bExceptionCaught = true;
							//DMibTrace("{}\r\n", _Exception);
		//					throw;
						}
					).f_Execute();
					

					DMibTest(DMibExpr(bExceptionCaught) == DMibExpr(true));
				}
				{
					bool bExceptionCaught = false;
					NMib::NException::fg_Try
					(
						[&] ()
						{
							throw float(30);
						}
					)
					.f_Catch<int>
					(
						[&] (int _Exception)
						{
							bExceptionCaught = true;
							//DMibTrace("{}\r\n", _Exception);
		//					throw;
						}
					)
					.f_Catch<float>
					(
						[&] (float _Exception)
						{
							bExceptionCaught = true;
							//DMibTrace("{}\r\n", _Exception);
		//					throw;
						}
					).f_Execute();
					

					DMibTest(DMibExpr(bExceptionCaught) == DMibExpr(true) && DMibExpr(2));
				}
				{
					int ExceptionCaught = false;
					NMib::NException::fg_Try
					(
						[&] ()
						{
							throw int(30);
						}
					)
					.f_Catch<int>
					(
						[&] (int _Exception)
						{
							++ExceptionCaught;
							throw;
						}
					)
					.f_Catch<int>
					(
						[&] (int _Exception)
						{
							++ExceptionCaught;
							//throw;
						}
					).f_Execute();
					

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
				mint nIterations = 11;
#ifdef DMibDebug
				mint nTests = 10000;
#else
				mint nTests = 100000;
#endif
				CAsyncResult Result;
				Result.f_SetException(fg_ExceptionPointer(DMibErrorInstance("Test error")));
				{
					CTestPerformanceMeasure Measure("Malterlib");
					for (mint i = 0; i < nIterations; ++i)
					{
						Measure.f_Start();
						[&]() inline_never
							{
								for (mint i = 0; i < nTests; ++i)
									Result.f_GetExceptionStr();
							}
							()
						;
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

