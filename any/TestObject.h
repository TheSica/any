#pragma once
#include <algorithm>

///////////////////////////////////////////////////////////////////////////////
/// kMagicValue
///
/// Used as a unique integer. We assign this to TestObject in its constructor
/// and verify in the TestObject destructor that the value is unchanged. 
/// This can be used to tell, for example, if an invalid object is being 
/// destroyed.
///
const uint32_t kMagicValue = 0x01f1cbe8;


///////////////////////////////////////////////////////////////////////////////
/// TestObject
///
/// Implements a generic object that is suitable for use in container tests.
/// Note that we choose a very restricted set of functions that are available
/// for this class. Do not add any additional functions, as that would 
/// compromise the intentions of the unit tests.
///

struct TestObject
{
	int             mX;                  // Value for the TestObject.
	bool            mbThrowOnCopy;       // Throw an exception of this object is copied, moved, or assigned to another.
	int64_t         mId;                 // Unique id for each object, equal to its creation number. This value is not coped from other TestObjects during any operations, including moves.
	uint32_t        mMagicValue;         // Used to verify that an instance is valid and that it is not corrupted. It should always be kMagicValue.
	static int64_t  sTOCount;            // Count of all current existing TestObjects.
	static int64_t  sTOCtorCount;        // Count of times any ctor was called.
	static int64_t  sTODtorCount;        // Count of times dtor was called.
	static int64_t  sTODefaultCtorCount; // Count of times the default ctor was called.
	static int64_t  sTOArgCtorCount;     // Count of times the x0,x1,x2 ctor was called.
	static int64_t  sTOCopyCtorCount;    // Count of times copy ctor was called.
	static int64_t  sTOMoveCtorCount;    // Count of times move ctor was called.
	static int64_t  sTOCopyAssignCount;  // Count of times copy assignment was called.
	static int64_t  sTOMoveAssignCount;  // Count of times move assignment was called.
	static int      sMagicErrorCount;    // Number of magic number mismatch errors.

	explicit TestObject(int x = 0, bool bThrowOnCopy = false)
		: mX(x), mbThrowOnCopy(bThrowOnCopy), mMagicValue(kMagicValue)
	{
		++sTOCount;
		++sTOCtorCount;
		++sTODefaultCtorCount;
		mId = sTOCtorCount;
	}

	// This constructor exists for the purpose of testing variadiac template arguments, such as with the emplace container functions.
	TestObject(int x0, int x1, int x2, bool bThrowOnCopy = false)
		: mX(x0 + x1 + x2), mbThrowOnCopy(bThrowOnCopy), mMagicValue(kMagicValue)
	{
		++sTOCount;
		++sTOCtorCount;
		++sTOArgCtorCount;
		mId = sTOCtorCount;
	}

	TestObject(const TestObject& testObject)
		: mX(testObject.mX), mbThrowOnCopy(testObject.mbThrowOnCopy), mMagicValue(testObject.mMagicValue)
	{
		++sTOCount;
		++sTOCtorCount;
		++sTOCopyCtorCount;
		mId = sTOCtorCount;
		if (mbThrowOnCopy)
		{
			throw "Disallowed TestObject copy";
		}
	}

	// Due to the nature of TestObject, there isn't much special for us to 
	// do in our move constructor. A move constructor swaps its contents with 
	// the other object, whhich is often a default-constructed object.
	TestObject(TestObject&& testObject)
		: mX(testObject.mX), mbThrowOnCopy(testObject.mbThrowOnCopy), mMagicValue(testObject.mMagicValue)
	{
		++sTOCount;
		++sTOCtorCount;
		++sTOMoveCtorCount;
		mId = sTOCtorCount;  // testObject keeps its mId, and we assign ours anew.
		testObject.mX = 0;   // We are swapping our contents with the TestObject, so give it our "previous" value.
		if (mbThrowOnCopy)
		{
			throw "Disallowed TestObject copy";
		}
	}

	TestObject& operator=(const TestObject& testObject)
	{
		++sTOCopyAssignCount;

		if (&testObject != this)
		{
			mX = testObject.mX;
			// Leave mId alone.
			mMagicValue = testObject.mMagicValue;
			mbThrowOnCopy = testObject.mbThrowOnCopy;
			if (mbThrowOnCopy)
			{
				throw "Disallowed TestObject copy";
			}
		}
		return *this;
	}

	TestObject& operator=(TestObject&& testObject)
	{
		++sTOMoveAssignCount;

		if (&testObject != this)
		{
			std::swap(mX, testObject.mX);
			// Leave mId alone.
			std::swap(mMagicValue, testObject.mMagicValue);
			std::swap(mbThrowOnCopy, testObject.mbThrowOnCopy);

			if (mbThrowOnCopy)
			{
				throw "Disallowed TestObject copy";
			}
		}
		return *this;
	}

	~TestObject()
	{
		if (mMagicValue != kMagicValue)
			++sMagicErrorCount;
		mMagicValue = 0;
		--sTOCount;
		++sTODtorCount;
	}

	static void Reset()
	{
		sTOCount = 0;
		sTOCtorCount = 0;
		sTODtorCount = 0;
		sTODefaultCtorCount = 0;
		sTOArgCtorCount = 0;
		sTOCopyCtorCount = 0;
		sTOMoveCtorCount = 0;
		sTOCopyAssignCount = 0;
		sTOMoveAssignCount = 0;
		sMagicErrorCount = 0;
	}

	static bool IsClear() // Returns true if there are no existing TestObjects and the sanity checks related to that test OK.
	{
		return (sTOCount == 0) && (sTODtorCount == sTOCtorCount) && (sMagicErrorCount == 0);
	}
};

// Operators
// We specifically define only == and <, in order to verify that 
// our containers and algorithms are not mistakenly expecting other 
// operators for the contained and manipulated classes.
inline bool operator==(const TestObject& t1, const TestObject& t2)
{
	return t1.mX == t2.mX;
}

inline bool operator<(const TestObject& t1, const TestObject& t2)
{
	return t1.mX < t2.mX;
}
// Normally you don't want to put your hash functions in the eastl namespace, as that namespace is owned by EASTL.
// However, these are the EASTL unit tests and we can say that they are also owned by EASTL.
template <>
struct std::hash<TestObject>
{
	size_t operator()(const TestObject& a) const
	{
		return static_cast<size_t>(a.mX);
	}
};


// use_mX
// Used for printing TestObject contents via the PrintSequence function,
// which is defined below. See the PrintSequence function for documentation.
// This function is an analog of the eastl::use_self and use_first functions.
// We declare this all in one line because the user should never need to 
// debug usage of this function.
template <typename T> struct use_mX { int operator()(const T& t) const { return t.mX; } };

///////////////////////////////////////////////////////////////////////////////
/// TestObjectHash
///
/// Implements a manually specified hash function for TestObjects.
///
struct TestObjectHash
{
	size_t operator()(const TestObject& t) const
	{
		return (size_t)t.mX;
	}
};

int64_t TestObject::sTOCount = 0;
int64_t TestObject::sTOCtorCount = 0;
int64_t TestObject::sTODtorCount = 0;
int64_t TestObject::sTODefaultCtorCount = 0;
int64_t TestObject::sTOArgCtorCount = 0;
int64_t TestObject::sTOCopyCtorCount = 0;
int64_t TestObject::sTOMoveCtorCount = 0;
int64_t TestObject::sTOCopyAssignCount = 0;
int64_t TestObject::sTOMoveAssignCount = 0;
int     TestObject::sMagicErrorCount = 0;