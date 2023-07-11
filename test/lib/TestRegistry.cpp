#include "Test.hpp"
#include "TestResult.hpp"
#include "TestRegistry.hpp"

#include <time.h>
#include <iostream>

void TestRegistry::addTest (Test *test)
{
	instance ().add (test);
}


void TestRegistry::runAllTests (TestResult& result)
{
	instance().seed = (unsigned int)time(NULL);
	srand(instance().seed);
	std::cerr << "Run tests with seed " << instance().seed << std::endl;
	instance().run (result);
}


TestRegistry& TestRegistry::instance () {
	static TestRegistry registry;
	return registry;
}


void TestRegistry::add (Test *test) {
	tests.push_back (test);
}


void TestRegistry::run (TestResult& result) {
	result.startTests ();
	for (std::vector<Test *>::iterator it = tests.begin (); it != tests.end (); ++it)
		(*it)->run (result);
	result.endTests ();
}
