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
#include <variant>

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
	&& sizeof(T) <= small_space_size
	&& alignof(T) <= alignof(void*)>;

template<class T>
using any_is_small = std::bool_constant<sizeof(T) <= small_space_size
	&& alignof(T) <= alignof(void*)>; // Check if alignment is necessary

typedef std::aligned_storage_t<small_space_size, std::alignment_of_v<void*>> internal_storage_t;



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
	void (*_copy)(const void*);
};

struct any_small
{
	template <class T>
	static void Destroy(void* target) noexcept
	{
		std::destroy_at<T>(target);
	}

	template<class T>
	static void Copy(const void* destination, void* what)
	{
		Construct(*static_cast<T*>(destination), *static_cast<T*>(what));
	}

	template<class T>
	static void Move(const void* destination, void* what)
	{
		Construct(*static_cast<T*>(destination), std::move(*static_cast<T*>(what)));
	}

	void (*_destroy)(void*);
	void (*_copy)(const void*);
	void (*_move)(void*);
};

template<class T>
any_big any_big_obj = { &any_big::Destroy<T>, &any_big::Copy<T> };

template<class T>
any_small any_small_obj = { &any_small::Destroy<T>, &any_small::Copy<T>, &any_small::Move<T> };

class any
{
public:
	constexpr any() noexcept
		:_storage{}
	{
	}

	any(const any& other)
		:_storage{},
		_representation{}
	{
		if (!other.has_value())
		{
			return;
		}

		switch (other._representation)
		{
		case any_representation::Big:
			_storage = other._storage;
			//other._storage.big_storage._big_handler->_copy;
			break;
		case any_representation::Small:
			break;
		case any_representation::Trivial:
			break;
		}
	}
	any(any&& other) noexcept;

	template<class T>
	any(T&& value) = delete;

	template<class T, class... Args>
	explicit any(std::in_place_type_t<T>, Args&& ...) = delete;
	template<class T, class U, class...Args>
	explicit any(std::in_place_type_t<T>, std::initializer_list<U>, Args&& ...) = delete; //might not do

	~any()
	{

	}

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

	bool has_value() const noexcept;
	const std::type_info& type() const noexcept;

private:
	any_representation _representation;

	struct big_storage_t
	{
		void* _big_storage = nullptr;
		any_big* _big_handler;
	};

	typedef std::aligned_storage_t<small_space_size, std::alignment_of_v<void*>> internal_storage_t;
	struct small_storage_t
	{
		internal_storage_t _small_storage;
		any_small* _small_handler;
	};

	struct storage
	{
		union
		{
			internal_storage_t trivial_storage;
			big_storage_t big_storage;
			small_storage_t small_storage;
		};
	};

	storage _storage;
};

template<class T, class... Args>
void Construct(T& destination, Args&& ... args)
{
	new(destination)T(std::forward<Args>(args)...);
}