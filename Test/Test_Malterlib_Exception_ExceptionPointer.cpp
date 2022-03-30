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

	class CExceptionPointer_Tests : public CTest
	{
	public:
		void f_DoTests()
		{
			DMibTestSuite(CTestCategory("Performance") << CTestGroup("Performance"))
			{
				constexpr mint c_nTests = 5;
				constexpr mint c_nExceptions = 100000;

				CTestPerformanceMeasure RawMalterlibTime("RawMalterlib");
				CTestPerformanceMeasure RawStdTime("RawStd");
				CTestPerformanceMeasure MalterlibTime("Malterlib");
				CTestPerformanceMeasure StdTime("Std");
				for (mint i = 0; i < c_nTests; ++i)
				{
					DMibTestScopeMeasure(RawMalterlibTime, c_nExceptions);
					for (mint i = 0; i < c_nExceptions; ++i)
						TCSharedPointer<CException> pException = fg_Construct("CException", DMibPFile, DMibPLine, DMibPFunction, "Test", false);
				}

				for (mint i = 0; i < c_nTests; ++i)
				{
					DMibTestScopeMeasure(RawStdTime, c_nExceptions);
					for (mint i = 0; i < c_nExceptions; ++i)
						TCSharedPointer<std::exception> pException = fg_Construct<std::runtime_error>("Test");
				}

				for (mint i = 0; i < c_nTests; ++i)
				{
					DMibTestScopeMeasure(MalterlibTime, c_nExceptions);
					for (mint i = 0; i < c_nExceptions; ++i)
						std::make_exception_ptr(DMibErrorInstance("Test"));
				}

				for (mint i = 0; i < c_nTests; ++i)
				{
					DMibTestScopeMeasure(StdTime, c_nExceptions);
					for (mint i = 0; i < c_nExceptions; ++i)
						std::make_exception_ptr(std::runtime_error("Test"));
				}

				CTestPerformance Performing(1.0);
				Performing.f_AddBaseline(RawMalterlibTime);
				Performing.f_AddBaseline(RawStdTime);
				Performing.f_AddReference(StdTime);
				Performing.f_Add(MalterlibTime);
				DMibTest(DMibExpr(Performing));
			};
		}
	};

	DMibTestRegister(CExceptionPointer_Tests, Malterlib::Exception);
}

