#include "Test.hpp"
#include "TestRegistry.hpp"
#include "TestResult.hpp"
#include "Failure.hpp"

Test::Test (const std::string& testName) : name (testName)
{
	TestRegistry::addTest (this);
}

void Test::run (TestResult& result)
{
#ifndef DONT_CATCH_EXCEPTIONS
    try
    {
#endif
        setup();
	    runTest (result);
#ifndef DONT_CATCH_EXCEPTIONS
    }
    catch (...) {
        result.addFailure (Failure ("Unhandled exception", name, "", 0)); \
    }
#endif
    teardown();
	result.testWasRun();
}
