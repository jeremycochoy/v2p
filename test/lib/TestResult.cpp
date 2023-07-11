#include "TestResult.hpp"
#include "Failure.hpp"


TestResult::TestResult() : failureCount (0), testCount(0), secondsElapsed(0)
{
    ::time(&startTime);
}


void TestResult::testWasRun()
{
	testCount++;
}

void TestResult::startTests ()
{
}


void TestResult::addFailure (const Failure & /*failure*/)
{
	failureCount++;
}

void TestResult::endTests ()
{
    time_t endTime;
    ::time(&endTime);
    secondsElapsed = (int)(endTime - startTime);
}
