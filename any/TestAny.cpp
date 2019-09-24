#include <gtest/gtest.h>
#include "any.h"
#include <numeric>
#include <any>
#include "TestObject.h"

/*
enum operation
	{
		Get,
		Destroy,
		Copy,
		Move,
		TypeInfo
	};

	// Standard specifies to avoid the use of dynamically allocated memory for small contained values.
	// The storage shall be done on heap for non trivial types and within the any type for any trivial types
	union storage
	{
		//typedef std::aligned_storage_t<4 * sizeof(void*), std::alignment_of_v<void*>> internal_storage_t; // _aligned_malloc(4 * sizeof(void*), alignof(void*))

		void* external_storage;// = nullptr; // for non-trivial types
		void* internal_storage = _aligned_malloc(4 * sizeof(void*), alignof(void*)); // for trivial types
	};

	template <typename T>
	using use_internal_storage = std::bool_constant<
								 std::is_nothrow_move_constructible_v<T> &&
								 (sizeof(T) <= sizeof(storage)) &&
								 (std::alignment_of_v<storage> % std::alignment_of_v<T> == 0)>;

	// Handler should the storage be within the any type itself
	template <typename T>
	struct storage_handler_internal
	{
		template<typename V>
		static void construct(storage& strg, V&& value)
		{
			new(&strg.internal_storage) T(std::forward<V>(value));
		}

		template<typename... Args>
		static void construct_inplace(storage& strg, Args&&... args)
		{
			new(strg.internal_storage) T(std::forward<Args>(args)...);
		}

		static inline void destroy(any& refAny)
		{
			T& type = *static_cast<T&>(static_cast<void*>(&refAny.storage.internal_storage));

			type.~T();
		}

		static void* handler(operation op, const any* pThis, any* pOther)
		{
			switch (op)
			{
				case operation::Get:
				return pThis->_storage.internal_storage;
				case operation::Destroy:
				destroy(&pThis);
				break;
				case operation::Copy:
				pThis->_storage.internal_storage = pOther->_storage.internal_storage;
				break;
				case operation::Move:
				std::swap(pThis->_storage.internal_storage, pOther->_storage.internal_storage);
				break;
				case operation::TypeInfo:
				return typeid(T);
			}
		}
	};

	// Handler should the storage be on the heap
	template<typename T>
	struct storage_handler_external
	{
		template<typename V>
		static void construct(storage& strg, V&& value)
		{
			strg.external_storage = static_cast<T*>(_aligned_malloc(sizeof(T), alignof(T)));
			new(strg.external_storage) T(std::forward<V>(value));
		}

		template<typename... Args>
		static void construct_inplace(storage& strg, Args&&... args)
		{
			strg.external_storage = static_cast<T*>(_aligned_malloc(sizeof... (T), alignof(T)));
			new(strg.external_storage) T(std::forward<Args>(args)...);
		}

		static inline void destroy(any& refAny)
		{
			T& type = static_cast<T&>(static_cast<void*>(refAny.storage.external_storage));

			type.~T();

			//_aligned_free(refAny.storage.external_storage);
		}

		static void* handler(operation op, const any* pThis, any* pOther)
		{
			switch (op)
			{
			case operation::Get:
				return pThis->_storage.internal_storage;
			case operation::Destroy:
				destroy(&pThis);
				break;
			case operation::Copy:
				construct(pThis->_storage, static_cast<T>(&pOther->_storage.internal_storage));
				pThis->_storage.internal_storage = pOther->_storage.internal_storage;
				break;
			case operation::Move:
				std::swap(pThis->_storage.internal_storage, pOther->_storage.internal_storage);
				break;
			case operation::TypeInfo:
				return typeid(T);
			}
		}
	};

	template<typename T>
	using storage_handler = std::conditional_t<use_internal_storage<T>::value, storage_handler_internal<T>, storage_handler_external<T>>;

/* Synopsis
	any() noexcept;

	any(const any& other);
	any(any&& other) noexcept;

	template<class T>
		any(T&& value) = delete;

	template<class T, class... Args>
		explicit any(std::in_place_type_t<T>, Args&&...) = delete;
	template<class T, class U, class...Args>
		explicit any(std::in_place_type_t<T>, std::initializer_list<U>, Args&&...) = delete; //might not do

	~any();

	any& operator=(const any& rhs);
	any& operator=(any&& rhs) noexcept;
	template<class T>
		any& operator=(T&& rhs) = delete;

	template<class T, class... Args>
		std::decay_t<T>& emplace(Args&&...) = delete;
	template<class T, class U, class... Args>
		std::decay_t<T>& emplace(std::initializer_list<U>, Args&& ...) = delete;
	void reset() noexcept = delete;
	void swap(any& rhs) noexcept = delete;

	bool has_value() const noexcept = delete;
	const std::type_info& type() const noexcept;


public:
	any() noexcept
		: _storage()
	{
	}

	any(const any& other)
	{

	}
	any(any&& other) noexcept;

	template<class T>
	any(T&& value) = delete;

	template<class T, class... Args>
	explicit any(std::in_place_type_t<T>, Args&& ...) = delete;
	template<class T, class U, class...Args>
	explicit any(std::in_place_type_t<T>, std::initializer_list<U>, Args&& ...) = delete; //might not do

	~any();

	any& operator=(const any& rhs);
	any& operator=(any&& rhs) noexcept;
	template<class T>
	any& operator=(T&& rhs) = delete;

	template<class T, class... Args>
	std::decay_t<T>& emplace(Args&& ...) = delete;
	template<class T, class U, class... Args>
	std::decay_t<T>& emplace(std::initializer_list<U>, Args&& ...) = delete;
	void reset() noexcept = delete;
	void swap(any& rhs) noexcept = delete;

	bool has_value() const noexcept = delete;
	const std::type_info& type() const noexcept = delete;

private:
	storage _storage; */

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

TEST(CtorTests, GivenNonEmptyAny_SmallRepresentationCastToBigRepresentationWorks)
{
	any intAny = 3333u;

	EXPECT_EQ(any_cast<unsigned>(intAny), 3333u);

	intAny = TestObject(33333);

	EXPECT_EQ(any_cast<TestObject>(intAny).mX, 33333);
}

TEST(CtorTests, GivenNonEmptyAny_HasValue)
{
	any a(42);

	EXPECT_TRUE(a.has_value());
}

TEST(CtorTests, GivenNonEmptyAny_EqualsOperatorWorks)
{
	any a1 = 42;
	any a2 = a1;

	EXPECT_TRUE(a1.has_value());
	EXPECT_TRUE(a2.has_value());
	EXPECT_EQ(any_cast<int>(a1), any_cast<int>(a2));
}

TEST(AnyCasts, GivenNonEmptyAny_AnyCastReturnsExpectedValue)
{
	any a(42);

	EXPECT_EQ(any_cast<int>(a), 42);
}

TEST(AnyCasts, GivenNonEmptyAny_AnyCastHoldsExpectedValue)
{
	any a(42);

	EXPECT_NE(any_cast<int>(a), 1337);
}

TEST(AnyCasts, GivenNonEmptyAny_AnyCastingModifiesTheValue)
{
	any a(42);

	any_cast<int&>(a) = 10;
	EXPECT_EQ(any_cast<int>(a), 10);
}

TEST(AnyCasts, GivenNonEmptyFloatAny_AnyCastingModifiesTheValue)
{
	any a(1.f);

	any_cast<float&>(a) = 1337.f;
	EXPECT_EQ(any_cast<float>(a), 1337.f);
}

TEST(AnyCasts, GivenNonEmptyStringAny_AnyCastingModifiesTheValue)
{
	any a(std::string("hello world"));

	EXPECT_EQ(any_cast<std::string>(a), "hello world");
	EXPECT_EQ(any_cast<std::string&>(a), "hello world");
}

TEST(AnyCasts, GivenNonEmptyCustomType_AnyCastingModifiesTheValue)
{
	struct custom_type { int data; };

	any a = custom_type{};
	any_cast<custom_type&>(a).data = 42;
	EXPECT_EQ(any_cast<custom_type>(a).data, 42);
}

TEST(AnyCasts, GivenNonEmptyAny_AnyCastingToDifferentTypeThrows)
{
	any a = 42;
	EXPECT_EQ(any_cast<int>(a), 42);

	EXPECT_ANY_THROW((any_cast<short>(a), 42));
}

TEST(AnyCasts, GivenNonEmptyAnyVector_AnyCastsSuccessfullyToExpectedTypes)
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

TEST(AnyCasts, GivenEmptyAnyVector_AnyCastsSuccessfulyAfterPushBack)
{
	std::vector<any> va;
	va.push_back(42);
	va.push_back(std::string("rob"));
	va.push_back('a');
	va.push_back(42.f);

	EXPECT_EQ(any_cast<int>(va[0]) , 42);
	EXPECT_EQ(any_cast<std::string>(va[1]) , "rob");
	EXPECT_EQ(any_cast<char>(va[2]) , 'a');
	EXPECT_EQ(any_cast<float>(va[3]) , 42.f);
}

TEST(AnyCasts, GivenSmallAnyObject_ReplacingItWithALargerOneDoesntCorrputTheSurroundingMemory)
{
	TestObject::Reset();
	{
		std::vector<any> va = { 42, 'a', 42.f, 3333u, 4444ul, 5555ull, 6666.0 };

		EXPECT_EQ(any_cast<int>(va[0]) , 42);
		EXPECT_EQ(any_cast<char>(va[1]) , 'a');
		EXPECT_EQ(any_cast<float>(va[2]) , 42.f);
		EXPECT_EQ(any_cast<unsigned>(va[3]) , 3333u);
		EXPECT_EQ(any_cast<unsigned long>(va[4]) , 4444ul);
		EXPECT_EQ(any_cast<unsigned long long>(va[5]) , 5555ull);
		EXPECT_EQ(any_cast<double>(va[6]) , 6666.0);

		va[3] = TestObject(3333); // replace a small integral with a large heap allocated object.

		EXPECT_EQ(any_cast<int>(va[0]) , 42);
		EXPECT_EQ(any_cast<char>(va[1]) , 'a');
		EXPECT_EQ(any_cast<float>(va[2]) , 42.f);
		EXPECT_EQ(any_cast<TestObject>(va[3]).mX , 3333); // not 3333u because TestObject ctor takes a signed type.
		EXPECT_EQ(any_cast<unsigned long>(va[4]) , 4444ul);
		EXPECT_EQ(any_cast<unsigned long long>(va[5]) , 5555ull);
		EXPECT_EQ(any_cast<double>(va[6]) , 6666.0);
	}
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

TEST(EmplaceTests, GivenEmptyAny_EmplacingLargeObjectsWorks)
{
	TestObject::Reset();
	{
		any a;
		a.emplace<TestObject>();
		EXPECT_TRUE(a.has_value());
	}
	EXPECT_TRUE(TestObject::IsClear());
}
