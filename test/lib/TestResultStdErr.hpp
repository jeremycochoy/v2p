#ifndef TESTRESULTSTDERR_H
#define TESTRESULTSTDERR_H

#include "TestResult.hpp"


class TestResultStdErr : public TestResult
{
public:
    virtual void addFailure (const Failure & failure);
    virtual void endTests ();
};


#endif
