#pragma once
/*
	At any point, an <any> stores one of the following types:
		1. Big types
		2. Small types
		3. Trivial types

	In the 1st case, <any> will dynamically allocate memory on the heap for the object
	In the 2nd case, <any> will store the object inside any itself
	In the 3rd case, <any> will store the object inside any itself and no destructor shall be called for the object. Additionally, the copy and moves will also be treaded differently
*/
#include <utility>
#include <initializer_list>
#include <type_traits>

class bad_any_cast : public std::bad_cast
{
	const char* what() const noexcept override
	{
		return "bad_any_cast";
	}
};

constexpr size_t small_space_size = 4 * sizeof(void*);

template<class T>
using any_is_trivial = std::bool_constant<std::is_trivially_copyable_v<T> // Check if alignment is necessary
									   && sizeof(T) <= small_space_size>;

template<class T>
using any_is_small = std::bool_constant<sizeof(T) <= small_space_size>; // Check if alignment is necessary

enum class any_representation
{
	Big,
	Small,
	Trivial
};

struct any_big
{
	template <class T>
	static void Destroy(void* target) noexcept
	{
		delete static_cast<T*>(target);
	}

	template<class T>
	static void* Copy(const void* source)
	{
		return new T(*static_cast<T*>(source));
	}

	void (*_destroy)(void*);
	void (*_copy)(const void *);
};

struct any_small
{

	void (*_destroy)(void*);
	void (*_copy)(const void*);
	void (*_move)(void*);
};

struct any_small
{

};

class any
{
};
