#include <gtest/gtest.h>
#include <any>
#include "any.h"
#include <typeinfo>
#include <type_traits>
#include <typeindex>
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

	return 0;
}