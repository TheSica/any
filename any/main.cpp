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
	std::any a = 3;

	return 0;
}