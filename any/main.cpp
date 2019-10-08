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
		tt = v;
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

	{
		any a1 = std::list<int>{ 1,2,3 };
		any a2 = std::list<int>{ 4,5,6 };
		a1.swap(a2);
		std::list<int> r = any_cast<const std::list<int>&>(a1);
		std::list<int> a = {1, 2, 3};
		
	}

	return 0;
}