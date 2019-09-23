#include <gtest/gtest.h>
#include <any>
#include "any.h"

struct S
{
	S(int v)
	{
		tt = v;
	}

	int tt = 0;
};

int main()
{
	testing::InitGoogleTest();
	RUN_ALL_TESTS();

	int testObj = 123456;
	S s(4);
	any testeroo;
	testeroo.emplace<int>(testObj);

	auto a = testeroo.any_cast<int>(&testeroo);
	
	any test(testeroo);

	auto b = test.any_cast<int>(&test);

	testeroo.emplace<int>(5);


	return 0;
}