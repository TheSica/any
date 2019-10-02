#include <gtest/gtest.h>
#include <any>
#include "any.h"

struct S
{
	S(int v) noexcept
	{
		tt = v;
	}

	int tt = 0;
};

int main()
{
	testing::InitGoogleTest();
	RUN_ALL_TESTS();

	any a(3);
	std::any b(3);
	return 0;
}