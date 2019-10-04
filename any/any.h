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
#include "cppcorecheck\warnings.h"

class bad_any_cast : public std::bad_cast
{
	const char* what() const noexcept override
	{
		return "bad_any_cast";
	}
};

constexpr size_t small_space_size = 4 * sizeof(void*);

template<class T>
using any_is_small = std::bool_constant<std::is_trivially_copyable_v<T>
									 && sizeof(T) <= small_space_size
									 && alignof(T) <= alignof(void*)>; // Check if alignment shouldnt be % == 0

enum class any_representation
{
	Small,
	Big,
};

template<class T, class... Args>
void Construct(void* destination, Args&&... args) noexcept
{
	new(destination) T(std::forward<Args>(args)...);
}

struct any_big
{
	template <class T>
	static void Destroy(void* target) noexcept
	{
		std::destroy_at<T>(static_cast<T*>(target));

		_aligned_free(target);
	}

	template<class T>
	static void* Copy(const void* source)
	{
		return new T(*static_cast<const T*>(source));
	}

	template<class T>
	static void* Type()
	{
		return (void*)&typeid(T);
	}

	void (*_destroy)(void*);
	void* (*_copy)(const void*);
	void* (*_type)();
};

struct any_small
{
	template <class T>
	static void Destroy(void* target)
	{
		if constexpr (!std::is_trivially_copyable_v<T>)
		{
			std::destroy_at(static_cast<T*>(target));
		}
	}

	template<class T>
	static void Copy(void* destination, const void* what)
	{
		if constexpr (std::is_trivially_copyable_v<T>)
		{
			*static_cast<T*>(destination) = *static_cast<const T*>(what);
		}
		else
		{
			Construct(*static_cast<T*>(destination), *static_cast<const T*>(what));
		}
	}

	template<class T>
	static void Move(void* destination, void* what) noexcept
	{
		if constexpr (std::is_trivially_copyable_v<T>)
		{
			*static_cast<T*>(destination) = *static_cast<T*>(what);
		}
		else
		{
			Construct(*static_cast<T*>(destination), std::move(*static_cast<T*>(what)));
		}
	}

	template<class T>
	static void* Type()
	{
		return (void*) & typeid(T);
	}

	void (*_destroy)(void*);
	void (*_copy)(void*, const void*);
	void (*_move)(void*, void*);
	void* (*_type)();
};

template<class T>
any_big any_big_obj = { &any_big::Destroy<T>, &any_big::Copy<T>, &any_big::Type<T> };

template<class T>
any_small any_small_obj = { &any_small::Destroy<T>, &any_small::Copy<T>, &any_small::Move<T>, &any_small::Type<T> };

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
		}
	}

	template<class T, typename VT = std::decay_t<T>, typename = std::enable_if_t<!std::is_same_v<VT, any> // can use conjunction and negation for short circuit but it's too hard too 
																			   && std::is_copy_constructible_v<VT>>> // check if VT is a specialization of in_place_type_t
	any(T&& value)
	{
		emplace<VT>(std::forward<T>(value));
	}

	template<class T, class... Args, typename VT = std::decay_t<T>, typename = std::enable_if<std::is_copy_constructible_v<VT>
																						   && std::is_constructible_v<VT, Args...>>>
	explicit any(std::in_place_type_t<T>, Args&&... args)
	{
		emplace<VT>(std::forward<T>(args)...);
	}

	template<class T, class U, class...Args, typename VT = std::decay_t<T>, typename = std::enable_if_t<std::is_copy_constructible_v<VT> && 
																										std::is_constructible_v<VT, std::initializer_list<U>&, Args...>>>
	explicit any(std::in_place_type_t<T>, std::initializer_list<U> il, Args&&... args)
	{
		emplace<VT>(il, std::forward<Args>(args)...);
	}

	~any()
	{
		reset();
	}
	
	any& operator=(const any& rhs)
	{
		any(rhs).swap(*this);

		return *this;
	}

	any& operator=(any&& rhs) noexcept
	{
		any(std::move(rhs)).swap(*this);
		return *this;
	}

	template<class T, typename VT = std::decay_t<T>, typename = std::enable_if_t<!std::is_same_v<VT, any>
																				&& std::is_copy_constructible_v<VT>>>
	any& operator=(T&& rhs)
	{
		any tmp(rhs);

		tmp.swap(*this);

		return *this;
	}
	
	template<class T, class... Args>
	std::decay_t<T>& emplace(Args&&... args)
	{
		return emplace_impl<T>(any_is_small<T>{}, std::forward<Args>(args)...);
	}
	
	template<class T, class U, class... Args>
	std::decay_t<T>& emplace(std::initializer_list<U>, Args&& ...) = delete;

	void reset() noexcept
	{
		if (!has_value())
		{
			return;
		}

		switch (_representation)
		{
		case any_representation::Big:
			_storage.big_storage.handler->_destroy(_storage.big_storage.storage);
			_storage.big_storage.handler = nullptr;
			break;
		case any_representation::Small:
			_storage.small_storage.handler->_destroy(&_storage.small_storage.storage);
			_storage.small_storage.handler = nullptr;
			break;
		}
	}

	void swap(any& rhs) noexcept
	{
		any tmp;
		tmp._storage = rhs._storage;
		tmp._representation = rhs._representation;

		rhs._storage = _storage;
		rhs._representation = _representation;

		_storage = tmp._storage;
		_representation = tmp._representation;
	}

	bool has_value() const noexcept
	{
		switch (_representation)
		{
		case any_representation::Big:
			return _storage.big_storage.handler != nullptr;
		case any_representation::Small:
			return _storage.small_storage.handler != nullptr;
		default:
			return false;
		}
	}
	const std::type_info& type() const noexcept
	{
		if (has_value())
		{
			switch (_representation)
			{
				case any_representation::Big:
					return *static_cast<const std::type_info*>(_storage.big_storage.handler->_type());
				case any_representation::Small:
					return *static_cast<const std::type_info*>(_storage.small_storage.handler->_type());
			}
		}
		else
		{
			return typeid(void);
		}
	}

	template<class T>
	T* get_val() noexcept
	{
		return static_cast<T*>(get_val_impl(any_is_small<T>{}));
	}

private:
	template<class T, class... Args>
	std::decay_t<T>& emplace_impl(std::true_type, Args&&... args) // any_is_trivial, any_is_small
	{
		// small any
		_storage.small_storage.handler = &any_small_obj<T>;
		Construct<T>(static_cast<void*>(&_storage.small_storage.storage), std::forward<Args>(args)...);
		_representation = any_representation::Small;
		return reinterpret_cast<T&>(_storage.small_storage.storage);
	}

	template<class T, class... Args>
	std::decay_t<T>& emplace_impl(std::false_type, Args&&... args) // any_is_trivial, any_is_small
	{
		// big any
		_storage.big_storage.handler = &any_big_obj<T>;
		_storage.big_storage.storage = _aligned_malloc(sizeof(T), alignof(T));
		Construct<T>(_storage.big_storage.storage, std::forward<Args>(args)...);
		_representation = any_representation::Big;
		return reinterpret_cast<T&>(_storage.big_storage.storage);
	}

	void* get_val_impl(std::true_type) noexcept
	{
		return (static_cast<void*>(&_storage.small_storage.storage));
	}

	void* get_val_impl(std::false_type) noexcept
	{
		return (_storage.big_storage.storage);
	}

	struct big_storage_t
	{
		void* storage;
		any_big* handler;
	};

	struct small_storage_t
	{
		typedef std::aligned_storage_t<small_space_size, std::alignment_of_v<void*>> internal_storage_t;
		internal_storage_t storage;
		any_small* handler;
	};

	struct storage
	{
		union
		{
			small_storage_t small_storage;
			big_storage_t big_storage;
		};

		std::type_info* _typeInfo;
	};

	storage _storage;
	any_representation _representation;
};

inline void swap(any& x, any& y) noexcept 
{
	x.swap(y);
}

template<class T, class... Args>
any make_any(Args&&... args)
{
	return any(std::in_place_type<T>, std::forward<Args>(args)...);
}

template<class T, class U, class... Args>
any make_any(std::initializer_list<U> il, Args&&... args)
{
	return any(std::in_place_type<T>, il, std::forward<Args>(args)...);
}

template<class T>
T any_cast(const any& operand)
{
	static_assert(std::is_constructible_v<T, const std::remove_cv_t<std::remove_reference_t<T>>&>);

	const auto storagePtr = any_cast<std::remove_cv_t<std::remove_reference_t<T>>>(&operand);

	if (!storagePtr)
	{
		throw bad_any_cast({});
	}

	return static_cast<T>(*storagePtr);
}

template<class T>
T any_cast(any& operand)
{
	static_assert(std::is_constructible_v<T, std::remove_cv_t<std::remove_reference_t<T>>&>);

	const auto storagePtr = any_cast<std::remove_cv_t<std::remove_reference_t<T>>>(&operand);

	if (!storagePtr)
	{
		throw bad_any_cast({});
	}

	return static_cast<T>(*storagePtr);
}

template<class T>
T any_cast(any&& operand)
{
	static_assert(std::is_constructible_v<T, std::remove_cv_t<std::remove_reference_t<T>>>);

	const auto storagePtr = any_cast<std::remove_cv_t<std::remove_reference_t<T>>>(&operand);

	if (!storagePtr)
	{
		throw bad_any_cast({});
	}

	return static_cast<T>(std::move(*storagePtr));
}

template<class T>
const T* any_cast(const any* operand) noexcept
{
	if (operand != nullptr && operand->type() == typeid(T))
	{
		return /*const_cast*/ operand->get_val<T>();
	}

	return nullptr;
}

template<class T>
T* any_cast(/*const*/ any* operand) noexcept
{
	if (operand != nullptr && operand->type() == typeid(T))
	{
		return /*const_cast*/ operand->get_val<T>();
	}

	return nullptr;
}