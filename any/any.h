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
		_aligned_free(target);
	}

	template<class T>
	static void* Copy(const void* source)
	{
		return new T(*static_cast<T*>(source));
	}

	void (*_destroy)(void*);
	void* (*_copy)(const void*);
};

struct any_small
{
	template <class T>
	static void Destroy(void* target) noexcept
	{
		std::destroy_at<T>(target);
	}

	template<class T>
	static void Copy(const void* destination, const void* what)
	{
		Construct(*static_cast<T*>(destination), *static_cast<T*>(what));
	}

	template<class T>
	static void Move(const void* destination, void* what)
	{
		Construct(*static_cast<T*>(destination), std::move(*static_cast<T*>(what)));
	}

	void (*_destroy)(void*);
	void (*_copy)(const void*, const void*);
	void (*_move)(const void*, void*);
};

template<class T>
any_big any_big_obj = { &any_big::Destroy<T>, &any_big::Copy<T> };

template<class T>
any_small any_small_obj = { &any_small::Destroy<T>, &any_small::Copy<T>, &any_small::Move<T> };

class any
{
public:
	constexpr any() noexcept
		:_storage{},
		_representation{}
	{
	}

	any(const any& other) noexcept
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
			_storage.big_storage.handler = other._storage.big_storage.handler;
			_storage.big_storage.storage = other._storage.big_storage.handler->_copy(other._storage.big_storage.storage);
			_representation = any_representation::Big;
			break;
		case any_representation::Small:
			_storage.small_storage.handler = other._storage.small_storage.handler;
			other._storage.small_storage.handler->_copy(&_storage.small_storage.storage, &other._storage.small_storage.storage);
			_representation = any_representation::Small;
			break;
		case any_representation::Trivial:
			_storage.trivial_storage = other._storage.trivial_storage;
			_representation = any_representation::Trivial;
			break;
		}
	}

	any(any&& other) noexcept
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
			_storage.big_storage.handler = other._storage.big_storage.handler;
			_storage.big_storage.storage = other._storage.big_storage.storage;
			_representation = any_representation::Big;
			break;
		case any_representation::Small:
			_storage.small_storage.handler = other._storage.small_storage.handler;
			other._storage.small_storage.handler->_move(&_storage.small_storage.storage, &other._storage.small_storage.storage);
			_representation = any_representation::Small;
			break;
		case any_representation::Trivial:
			_storage.trivial_storage = other._storage.trivial_storage;
			_representation = any_representation::Trivial;
			break;
		}
	}

	template<class T, std::enable_if_t<!std::is_same_v<std::decay_t<T>, any> // can use conjunction and negation for short circuit but it's too hard too read
																			 && std::is_copy_constructible_v<std::decay_t<T>>>> // check if VT is a specialization of in_place_type_t
	any(T&& value)
	{
		emplace<std::decay_t<T>>(std::forward<T>(value));
	}

	template<class T, class... Args, typename VT = std::decay_t<T>, typename = std::enable_if<std::is_copy_constructible_v<T>
																						   && std::is_constructible_v<std::decay_t<T>, Args...>>>
	explicit any(std::in_place_type_t<T> ip, Args&&... args)
	{
		emplace<VT>(std::forward<T>(args)...);
	}

	template<class T, class U, class...Args>
	explicit any(std::in_place_type_t<T>, std::initializer_list<U>, Args&& ...) = delete; //might not do

	~any()
	{
		switch (_representation)
		{
		case any_representation::Big:
			_storage.big_storage.handler->_destroy(_storage.big_storage.storage);
			break;
		case any_representation::Small:
			_storage.small_storage.handler->_destroy(&_storage.small_storage.storage);
			break;
		case any_representation::Trivial:
			break;
		}
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
	template<class T, class... Args>
	T& emplace(Args&&... args)
	{
		return emplace_impl<T>(any_is_trivial<T>{}, any_is_small<T>{}, std::forward<Args>(args)...);
	}

	template<class T, class... Args>
	T& emplace_impl(std::true_type, std::false_type, Args&&... args) // any_is_trivial, any_is_small
	{
		// specialization for trivial any
		Construct(static_cast<T&>(_storage.trivial_storage), std::forward<Args>(args)...);
		_representation = any_representation::Trivial;
		return static_cast<T&>(_storage.trivial_storage);
	}

	template<class T, class... Args>
	T& emplace_impl(std::false_type, std::true_type, Args&&... args) // any_is_trivial, any_is_small
	{
		// specialization for small any
		Construct(static_cast<T&>(_storage.small_storage.storage), std::forward<Args>(args)...);
		_storage.small_storage.handler = &any_small_obj<T>;
		_representation = any_representation::Small;
		return static_cast<T&>(_storage.small_storage.storage);
	}

	template<class T, class... Args>
	T& emplace_impl(std::false_type, std::false_type, Args&&... args) // any_is_trivial, any_is_small
	{
		_storage.big_storage.storage = _aligned_malloc(sizeof(T), alignof(T));
		Construct(static_cast<T&>(_storage.big_storage.storage), std::forward<Args>(args)...);
		_storage.big_storage.handler = &any_big_obj<T>;
		_representation = any_representation::Trivial;
		return static_cast<T&>(_storage.trivial_storage);
	}

	struct big_storage_t
	{
		void* storage = nullptr;
		any_big* handler;
	};

	typedef std::aligned_storage_t<small_space_size, std::alignment_of_v<void*>> internal_storage_t;
	struct small_storage_t
	{
		internal_storage_t storage;
		any_small* handler;
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
	any_representation _representation;
};

template<class T, class... Args>
void Construct(T& destination, Args&&... args)
{
	new(destination)T(std::forward<Args>(args)...);
}