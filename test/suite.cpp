#include "lib/TestHarness.hpp"
#include "lib/TestResultStdErr.hpp"

int main()
{
    TestResultStdErr result;
    TestRegistry::runAllTests(result);
    return (result.getFailureCount());
}
