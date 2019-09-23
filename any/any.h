#pragma once

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

class any
{
private:

	template <class T> friend const T* any_cast(const any* operand) noexcept;
	template <class T> friend T* any_cast(any* operand) noexcept;
	template <class T> friend T any_cast(const any& operand);
	template <class T> friend T any_cast(any& operand);
	template <class T> friend T any_cast(any&& operand);


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
		typedef std::aligned_storage_t<4 * sizeof(void*), std::alignment_of_v<void*>> internal_storage_t; // _aligned_malloc(4 * sizeof(void*), alignof(void*))

		void* external_storage = nullptr; // for non-trivial types
		internal_storage_t internal_storage;//= _aligned_malloc(4 * sizeof(void*), alignof(void*)); // for trivial types
	};

	template <typename T>
	using use_internal_storage = std::bool_constant<
		std::is_nothrow_move_constructible_v<T> &&
		(sizeof(T) <= sizeof(storage)) &&
		(std::alignment_of_v<storage> % std::alignment_of_v<T> == 0)>;

	template <typename T>
	struct handler
	{
		template<typename V>
		static void Construct(storage& strg, V&& v, std::true_type)
		{
			new(&strg.internal_storage) T(std::forward<V>(v));
		}

		template<typename V>
		static void Construct(storage& strg, V&& v, std::false_type)
		{
			strg.external_storage = static_cast<T*>(_aligned_malloc(sizeof(T), alignof(T)));

			new(strg.external_storage) T(std::forward<V>(v));
		}

		static void Destroy(any& rAny, std::true_type)
		{
			if constexpr (!std::is_trivially_constructible_v<T>)
			{
				auto t = static_cast<T*>(static_cast<void*>(&rAny._storage.internal_storage));
				t->~T();
			}
		}

		static void Destroy(any& rAny, std::false_type)
		{
			if constexpr (!std::is_trivially_constructible_v<T>)
			{
				auto t = static_cast<T*>(rAny._storage.external_storage);
				t->~T();
			}

			_aligned_free(rAny._storage.external_storage);
		}

		static void Copy(const any& srcAny, any& destAny)
		{
			if constexpr (std::is_trivially_constructible_v<T>)
			{
				Construct(destAny._storage, *(T*)(&srcAny._storage.internal_storage), use_internal_storage<T>{});
			}
			else
			{
				Construct(destAny._storage, *static_cast<T*>(srcAny._storage.external_storage), use_internal_storage<T>{});
			}
		}

		static void Move(const any& srcAny, any& destAny)
		{
			if constexpr (std::is_trivially_constructible_v<T>)
			{
				Construct(destAny._storage, std::move(*(T*)(&srcAny._storage.internal_storage)), use_internal_storage<T>{});
			}
			else
			{
				Construct(destAny._storage, std::move(*(T*)(srcAny._storage.external_storage)), use_internal_storage<T>{});
			}
		}

		static void* GetStorage(const any& rAny)
		{
			if constexpr (std::is_trivially_constructible_v<T>)
			{
				return (void*)(&rAny._storage.internal_storage);
			}
			else
			{
				return static_cast<void*>(rAny._storage.external_storage);
			}
		}

		static type_info GetType()
		{
			return typeid(T);
		}

		static void* handler_func(operation op, const any* pThis, any* pOther)
		{
			switch (op)
			{
			case operation::Get:
				return handler<T>().GetStorage(*pThis);
				break;
			case operation::Destroy:
				handler<T>().Destroy(*const_cast<any*>(pThis), use_internal_storage<T>{});
				break;
			case operation::Copy:
				handler<T>().Copy(*pThis, *pOther);
				break;
			case operation::Move:
				handler<T>().Move(*pThis, *pOther);
				break;
			case operation::TypeInfo:
				return (void*) & typeid(T);
				break;
			}
		}
	};

public:
	constexpr any() noexcept
		:
		_storage(),
		_handler_func(nullptr)
	{
	}

	any(const any& other)
		:
		_handler_func(nullptr)
	{
		if (other.has_value())
		{
			other._handler_func(operation::Copy, &other, this);

			_handler_func = other._handler_func;
		}
	}

	any(any&& other) noexcept
		:
		_storage(),
		_handler_func(nullptr)
	{
		if (other.has_value())
		{
			other._handler_func(operation::Move, &other, this);
			_handler_func = std::move(other._handler_func);
		}
	}

	template<class T, typename = std::enable_if_t<!std::is_same_v<std::decay_t<T>, any>&& std::is_copy_constructible_v<std::decay_t<T>>>>
	any(T&& value)
	{
		handler<T>().Construct(_storage, std::forward<T>(value), use_internal_storage<T>{});
		_handler_func = &handler<T>::handler_func;
	}

	template<class T, class... Args, typename = std::enable_if_t<std::conjunction_v<
		std::is_copy_constructible<std::decay_t<T>>,
		std::is_constructible<std::decay_t<T>, Args...>>>>
		explicit any(std::in_place_type_t<T>, Args && ...args)
	{
		handler<T>().Construct(_storage, std::forward<Args>(args)...);
	}

	template<class T, class U, class...Args, typename = std::enable_if_t<std::conjunction_v<
		std::is_copy_constructible<std::decay_t<T>>,
		std::is_constructible<std::decay_t<T>, std::initializer_list<U>&, Args...>>>>
		explicit any(std::in_place_type_t<T>, std::initializer_list<U> il, Args && ...args)
	{
		emplace(il, std::forward<Args>(args)...);
	}

	~any()
	{
		reset();
	}

	any& operator=(const any& rhs);
	any& operator=(any&& rhs) noexcept;
	template<class T>
	any& operator=(T&& rhs) = delete;

	template<class T, class... Args>
	void emplace(Args&& ...args)
	{
		handler<T>().Construct(_storage, std::forward<Args>(args)..., use_internal_storage<T>{});
		_handler_func = &handler<T>::handler_func;
	}

	template<class T, class U, class... Args>
	std::decay_t<T>& emplace(std::initializer_list<U>, Args&& ...) = delete;

	void reset() noexcept
	{
		if (has_value())
		{
			_handler_func(operation::Destroy, this, nullptr);

			_handler_func = nullptr;
		}
	}

	void swap(any& rhs) noexcept = delete;

	bool has_value() const noexcept
	{
		return _handler_func != nullptr;
	}

	const std::type_info& type() const noexcept
	{
		if (_handler_func)
		{
			auto* pTypeInfo = _handler_func(operation::TypeInfo, this, nullptr);
			return *static_cast<const std::type_info*>(pTypeInfo);
		}
		else
		{
			return typeid(void);
		}
	}

private:
	storage _storage;
	void* (*_handler_func)(operation, const any*, any*) = nullptr;
};

template<class T>
inline T any_cast(const any& operand)
{
	static_assert(std::is_constructible_v<T, const std::_Remove_cvref_t<T>&>);

	const auto ptr = static_cast<T>(any_cast<std::_Remove_cvref_t<T>>(&operand));

	if (!ptr)
	{
		throw bad_any_cast({});
	}

	return static_cast<T>(*ptr);
}

template<class T>
inline T any_cast(any& operand)
{
	static_assert(std::is_constructible_v<T, std::_Remove_cvref_t<T>&>);

	auto ptr = static_cast<T*>(any_cast<std::_Remove_cvref_t<T>>(&operand));

	if (!ptr)
	{
		throw bad_any_cast({});
	}

	return *ptr;
}

template<class T>
inline T any_cast(any&& operand)
{
	static_assert(std::is_constructible_v<T, std::_Remove_cvref_t<T>>);

	const auto ptr = static_cast<T>(any_cast<std::_Remove_cvref_t<T>>(&operand));

	if (!ptr)
	{
		throw bad_any_cast({});
	}

	return static_cast<T>(std::move(*ptr));
}

template<class T>
inline const T* any_cast(const any* operand) noexcept
{
	if (operand != nullptr && operand->type() == typeid(T) /*&& operand->has_value()*/)
	{
		return static_cast<T*>(operand->_handler_func(any::operation::Get, operand, nullptr));
	}

	return nullptr;
}

template<class T>
inline T* any_cast(any* operand) noexcept
{
	if (operand != nullptr && operand->type() == typeid(T) /*&& operand->has_value()*/)
	{
		return static_cast<T*>(operand->_handler_func(any::operation::Get, operand, nullptr));
	}

	return nullptr;
}