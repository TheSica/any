#include <gtest/gtest.h>
#include "any.h"
#include <numeric>
#include <any>
#include "TestObject.h"

__declspec(align(16))
struct Align16
{
	explicit Align16(int x = 16) : mX(x) {}
	int mX;
};

inline bool operator==(const Align16& a, const Align16& b)
{
	return (a.mX == b.mX);
}

__declspec(align(32))
struct Align32
{
	explicit Align32(int x = 32) : mX(x) {}
	int mX;
};

inline bool operator==(const Align32& a, const Align32& b)
{
	return (a.mX == b.mX);
}

__declspec(align(64))
struct Align64
{
	explicit Align64(int x = 64) : mX(x) {}
	int mX;
};

inline bool operator==(const Align64& a, const Align64& b)
{
	return (a.mX == b.mX);
}

struct SmallTestObject
{
	static int mCtorCount;

	SmallTestObject() noexcept { mCtorCount++; }
	SmallTestObject(const SmallTestObject&) noexcept { mCtorCount++; }
	SmallTestObject(SmallTestObject&&) noexcept { mCtorCount++; }
	SmallTestObject& operator=(const SmallTestObject&) noexcept { mCtorCount++; return *this; }
	~SmallTestObject() noexcept { mCtorCount--; }

	static void Reset() { mCtorCount = 0; }
	static bool IsClear() { return mCtorCount == 0; }
};

int SmallTestObject::mCtorCount = 0;

struct RequiresInitList
{
	RequiresInitList(std::initializer_list<int> ilist)
		: sum(std::accumulate(begin(ilist), end(ilist), 0)) {}

	int sum;
};


TEST(CtorTests, GivenEmptyAny_DefaultConstructorWorks)
{
	any a;
	EXPECT_FALSE(a.has_value());
}

TEST(CtorTests, GivenSmallTestObject_CtorsAndDtorsAreCalledForSmallObject)
{
	SmallTestObject::Reset();
	{
		any a{ SmallTestObject() };
	}
	EXPECT_TRUE(SmallTestObject::IsClear());
}

TEST(CtorTests, GivenTestObject_CtorsAndDtorsAreCalled)
{
	TestObject::Reset();
	{
		any a{ TestObject() };
	}
	EXPECT_TRUE(TestObject::IsClear());
}

TEST(CtorTests, GivenNonEmptyAny_HasValue)
{
	any a(42);

	EXPECT_TRUE(a.has_value());
}

TEST(DtorTests, GivenNonEmptyObjects_DtorIsCalledAfterSwappingRepresentation)
{
	TestObject::Reset();
	{
		any a(42);
		any b{ TestObject() };

		b.swap(a);
	}
	EXPECT_TRUE(TestObject::IsClear());
}


TEST(CtorTests, GivenNonEmptyAny_SmallRepresentationCastToBigRepresentationWorks)
{
	any intAny = 3333u;

	EXPECT_EQ(any_cast<unsigned>(intAny), 3333u);

	intAny = TestObject(33333);

	EXPECT_EQ(any_cast<TestObject>(intAny).mX, 33333);
}

TEST(CtorTests, GivenNonEmptyAny_EqualsOperatorWorks)
{
	any a1 = 42;
	any a2 = a1;

	EXPECT_TRUE(a1.has_value());
	EXPECT_TRUE(a2.has_value());
	EXPECT_EQ(any_cast<int>(a1), any_cast<int>(a2));
}

TEST(CtorTests, GivenNonEmptyStringAny_ValueIsCorrect)
{
	any a(std::string("test string"));
	EXPECT_TRUE(a.has_value());
	EXPECT_EQ(any_cast<std::string>(a), "test string");
}

TEST(CtorTests, GivenEmptyAny_ConstructingTheAnyFromAScopeConstructedAnyWorks)
{
	any a1;
	EXPECT_FALSE(a1.has_value());

	{
		any a2(std::string("test string"));
		a1 = any_cast<std::string>(a2);

		EXPECT_TRUE(a1.has_value());
	}

	EXPECT_EQ(any_cast<std::string>(a1), "test string");
	EXPECT_TRUE(a1.has_value());
}

TEST(CtorTests, TT)
{
	any a1;
	EXPECT_FALSE(a1.has_value());

	{
		any a2(std::string("test string"));
		a1 = a2;
		EXPECT_TRUE(a1.has_value());
	}

	EXPECT_EQ(any_cast<std::string&>(a1), "test string");
	EXPECT_TRUE(a1.has_value());
}

TEST(CtorTests, GivenAlignedTypes_AnyConstructsWithRequestedAlignment)
{
	{
		any a = Align16(1337);
		EXPECT_TRUE(any_cast<Align16>(a) == Align16(1337));
	}

	{
		any a = Align32(1337);
		EXPECT_TRUE(any_cast<Align32>(a) == Align32(1337));
	}

	{
		any a = Align64(1337);
		EXPECT_TRUE(any_cast<Align64>(a) == Align64(1337));
	}
}

TEST(CtorTests, GivenFloat_AnyCtorDeducesTheTypeCorrectly)
{
	float f = 42.f;
	any a(f);
	EXPECT_EQ(any_cast<float>(a), 42.f);
}

TEST(AnyCastsTest, GivenNonEmptyAny_AnyCastReturnsExpectedValue)
{
	any a(42);

	EXPECT_EQ(any_cast<int>(a), 42);
}

TEST(AnyCastsTest, GivenNonEmptyAny_AnyCastHoldsExpectedValue)
{
	any a(42);

	EXPECT_NE(any_cast<int>(a), 1337);
}

TEST(AnyCastsTest, GivenNonEmptyAny_AnyCastingModifiesTheValue)
{
	any a(42);

	any_cast<int&>(a) = 10;
	EXPECT_EQ(any_cast<int>(a), 10);
}

TEST(AnyCastsTest, GivenNonEmptyFloatAny_AnyCastingModifiesTheValue)
{
	any a(1.f);

	any_cast<float&>(a) = 1337.f;
	EXPECT_EQ(any_cast<float>(a), 1337.f);
}

TEST(AnyCastsTest, GivenNonEmptyStringAny_AnyCastingModifiesTheValue)
{
	any a(std::string("hello world"));

	EXPECT_EQ(any_cast<std::string>(a), "hello world");
	EXPECT_EQ(any_cast<std::string&>(a), "hello world");
}

TEST(AnyCastsTest, GivenNonEmptyCustomType_AnyCastingModifiesTheValue)
{
	struct custom_type { int data; };

	any a = custom_type{};
	any_cast<custom_type&>(a).data = 42;
	EXPECT_EQ(any_cast<custom_type>(a).data, 42);
}

TEST(AnyCastsTest, GivenNonEmptyAny_AnyCastingToDifferentTypeThrows)
{
	any a = 42;
	EXPECT_EQ(any_cast<int>(a), 42);

	EXPECT_ANY_THROW((any_cast<short>(a), 42));
}

TEST(AnyCastsTest, GivenNonEmptyAnyVector_AnyCastsTestSuccessfullyToExpectedTypes)
{
	std::vector<any> va = { 42, 'a', 42.f, 3333u, 4444ul, 5555ull, 6666.0, std::string("dolhasca") };

	EXPECT_EQ(any_cast<int>(va[0]), 42);
	EXPECT_EQ(any_cast<char>(va[1]), 'a');
	EXPECT_EQ(any_cast<float>(va[2]), 42.f);
	EXPECT_EQ(any_cast<unsigned>(va[3]), 3333u);
	EXPECT_EQ(any_cast<unsigned long>(va[4]), 4444ul);
	EXPECT_EQ(any_cast<unsigned long long>(va[5]), 5555ull);
	EXPECT_EQ(any_cast<double>(va[6]), 6666.0);
	EXPECT_EQ(any_cast<std::string>(va[7]), "dolhasca");
}

TEST(AnyCastsTest, GivenEmptyAnyVector_AnyCastsTestSuccessfulyAfterPushBack)
{
	std::vector<std::any> va;
	va.push_back(42);
	va.push_back(std::string("rob"));
	va.push_back('a');
	va.push_back(42.f);

	EXPECT_EQ(any_cast<int>(va[0]), 42);
	EXPECT_EQ(any_cast<std::string>(va[1]), "rob");
	EXPECT_EQ(any_cast<char>(va[2]), 'a');
	EXPECT_EQ(any_cast<float>(va[3]), 42.f);
}

TEST(AnyCastsTest, GivenSmallAnyObject_ReplacingItWithALargerOneDoesntCorrputTheSurroundingMemory)
{
	TestObject::Reset();
	{
		std::vector<any> va = { 42, 'a', 42.f, 3333u, 4444ul, 5555ull, 6666.0 };

		EXPECT_EQ(any_cast<int>(va[0]), 42);
		EXPECT_EQ(any_cast<char>(va[1]), 'a');
		EXPECT_EQ(any_cast<float>(va[2]), 42.f);
		EXPECT_EQ(any_cast<unsigned>(va[3]), 3333u);
		EXPECT_EQ(any_cast<unsigned long>(va[4]), 4444ul);
		EXPECT_EQ(any_cast<unsigned long long>(va[5]), 5555ull);
		EXPECT_EQ(any_cast<double>(va[6]), 6666.0);

		va[3] = TestObject(3333); // replace a small integral with a large heap allocated object.

		EXPECT_EQ(any_cast<int>(va[0]), 42);
		EXPECT_EQ(any_cast<char>(va[1]), 'a');
		EXPECT_EQ(any_cast<float>(va[2]), 42.f);
		EXPECT_EQ(any_cast<TestObject>(va[3]).mX, 3333); // not 3333u because TestObject ctor takes a signed type.
		EXPECT_EQ(any_cast<unsigned long>(va[4]), 4444ul);
		EXPECT_EQ(any_cast<unsigned long long>(va[5]), 5555ull);
		EXPECT_EQ(any_cast<double>(va[6]), 6666.0);
	}
}

TEST(AnyCasts, GivenNonEmptyAny_EquivalenceCastsWorkAsExpected)
{
	any a, b;
	EXPECT_TRUE(!a.has_value() == !b.has_value());

	EXPECT_ANY_THROW(any_cast<int>(a) == any_cast<int>(b));

	a = 42; b = 24;
	EXPECT_TRUE(any_cast<int>(a) != any_cast<int>(b));
	EXPECT_TRUE(a.has_value() == b.has_value());

	a = 42; b = 42;
	EXPECT_TRUE(any_cast<int>(a) == any_cast<int>(b));
	EXPECT_TRUE(a.has_value() == b.has_value());
}

TEST(AnyCastsTest, GivenEmptyAny_CastsReturnNullptr)
{
	any* a = nullptr;
	EXPECT_TRUE(any_cast<int>(a) == nullptr);
	EXPECT_TRUE(any_cast<short>(a) == nullptr);
	EXPECT_TRUE(any_cast<long>(a) == nullptr);
	EXPECT_TRUE(any_cast<std::string>(a) == nullptr);

	any b;
	EXPECT_TRUE(any_cast<short>(&b) == nullptr);
	EXPECT_TRUE(any_cast<const short>(&b) == nullptr);
	EXPECT_TRUE(any_cast<volatile short>(&b) == nullptr);
	EXPECT_TRUE(any_cast<const volatile short>(&b) == nullptr);

	EXPECT_TRUE(any_cast<short*>(&b) == nullptr);
	EXPECT_TRUE(any_cast<const short*>(&b) == nullptr);
	EXPECT_TRUE(any_cast<volatile short*>(&b) == nullptr);
	EXPECT_TRUE(any_cast<const volatile short*>(&b) == nullptr);
}

TEST(EmplaceTests, GivenEmptyAny_EmplacingSmallObjectsWorks)
{
	any a;

	a.emplace<int>(42);
	EXPECT_TRUE(a.has_value());
	EXPECT_EQ(any_cast<int>(a), 42);

	a.emplace<short>((short)8); // no way to define a short literal we must cast here.
	EXPECT_EQ(any_cast<short>(a), 8);
	EXPECT_TRUE(a.has_value());

	a.reset();
	EXPECT_FALSE(a.has_value());
}

TEST(EmplaceTests, GivenEmptyAny_EmplacingLargeObjects_Works)
{
	TestObject::Reset();
	{
		any a;
		a.emplace<TestObject>();
		EXPECT_TRUE(a.has_value());
	}
	EXPECT_TRUE(TestObject::IsClear());
}

TEST(EmplaceTests, GivenEmptyAny_EmplaceInitializingThroughInitializerList_Works)
{
	{
		any a;
		a.emplace<RequiresInitList>(std::initializer_list<int>{1, 2, 3, 4, 5, 6});

		EXPECT_TRUE(a.has_value());
		EXPECT_EQ(any_cast<RequiresInitList>(a).sum, 21);
	}
}

TEST(SwapTests, GivenNonEmptyAny_AnySwapWorks)
{
	any a1 = 42;
	any a2 = 24;
	EXPECT_EQ(any_cast<int>(a1), 42);
	EXPECT_EQ(any_cast<int>(a2), 24);

	a1.swap(a2);
	EXPECT_EQ(any_cast<int>(a1), 24);
	EXPECT_EQ(any_cast<int>(a2), 42);
}

TEST(SwapTests, GivenNonEmptyAny_STDSwapWorksOnAny)
{
	any a1 = 42;
	any a2 = 24;

	EXPECT_EQ(any_cast<int>(a1), 42);
	EXPECT_EQ(any_cast<int>(a2), 24);

	std::swap(a1, a2);

	EXPECT_EQ(any_cast<int>(a1), 24);
	EXPECT_EQ(any_cast<int>(a2), 42);
}

TEST(SwapTests, GivenNonEmptyListAny_SwapWorksAsExpected)
{
	any a1 = std::list<int>{1, 2, 3};
	any a2 = std::list<int>{4, 5, 6};

	a1.swap(a2);

	std::list<int> result = any_cast<const std::list<int>&>(a1);

	EXPECT_TRUE((result == std::list<int>{4, 5, 6}));
}

TEST(SwapTests, GivenNonEmptyStringAny_SwapWorksAsExpected)
{
	any a1 = std::string("firstString");
	any a2 = std::string("secondString");

	a1.swap(a2);

	std::string result = any_cast<const std::string&>(a1);

	EXPECT_EQ(result, std::string("secondString"));
}

TEST(SwapTests, GivenNonEmptyTOList_SwapWorksAsExpected)
{
	any a1 = std::list<TestObject>{TestObject(1), TestObject(2), TestObject(3), TestObject(4), TestObject(5)};
	any a2 = std::list<TestObject>{TestObject(6), TestObject(7), TestObject(8), TestObject(9), TestObject(10)};

	a1.swap(a2);

	auto result = any_cast<const std::list<TestObject>&>(a1);

	EXPECT_TRUE((result == std::list<TestObject>{TestObject(6), TestObject(7), TestObject(8), TestObject(9), TestObject(10)}));
}

TEST(TypeInfoTests, GivenNonEmptyAnys_TypeInfoIsCorrect)
{
	EXPECT_EQ(std::strcmp(any(42).type().name(), "int"), 0);
	EXPECT_EQ(std::strcmp(any(42.f).type().name(), "float"), 0);
	EXPECT_EQ(std::strcmp(any(42u).type().name(), "unsigned int"), 0);
	EXPECT_EQ(std::strcmp(any(42ul).type().name(), "unsigned long"), 0);
	EXPECT_EQ(std::strcmp(any(42l).type().name(), "long"), 0);
}

TEST(OperatorEQTests, GivenNonEmptyAny_MovingIntoAnyWorks)
{
	any a = std::string("hello world");
	EXPECT_EQ(any_cast<std::string&>(a), "hello world");

	auto s = std::move(any_cast<std::string&>(a)); // move string out
	EXPECT_EQ(s, "hello world");
	EXPECT_TRUE(any_cast<std::string&>(a).empty());

	any_cast<std::string&>(a) = move(s); // move string in
	EXPECT_EQ(any_cast<std::string&>(a), "hello world");
}

TEST(MakeAnyTests, GivenAuto_MakeAnyWorks)
{
	{
		auto a = make_any<int>(42);
		EXPECT_EQ(any_cast<int>(a), 42);
	}
}