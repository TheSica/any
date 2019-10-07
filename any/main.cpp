#include <gtest/gtest.h>
#include <any>
#include "any.h"
#include <typeinfo>
#include <type_traits>
#include <typeindex>
struct S
{
	S(std::string v)
	{
		tt  = v;
	}

	S(S&& v)
	{
	};

	std::string tt;
};

int main()
{
	testing::InitGoogleTest();
	RUN_ALL_TESTS();


	return 0;
}