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
	struct handler
	{
		template<typename V>
		static void Construct(storage& strg, V&& v)
		{
			if constexpr (std::is_trivially_constructible_v<T>)
			{
				new(&strg.internal_storage) T(std::forward<V>(v));
			}
			else
			{
				strg.external_storage = static_cast<T*>(_aligned_malloc(sizeof(T), alignof(T)));

				new(strg.external_storage) T(std::forward<V>(v));
			}
		}

		static void Destroy(any& rAny)
		{
			if constexpr (!std::is_trivially_constructible_v<T>)
			{
				std::destroy_at(rAny._storage.external_storage);
			}
		}

		static void Copy(const any& srcAny, any& destAny)
		{
			if constexpr (std::is_trivially_constructible_v<T>)
			{
				Construct(destAny._storage, *(T*)(&srcAny._storage.internal_storage));
			}
			else
			{
				Construct(destAny._storage, *static_cast<T*>(srcAny._storage.external_storage));
			}
		}

		static void Move(const any& srcAny, any& destAny)
		{
			if constexpr (std::is_trivially_constructible_v<T>)
			{
				Construct(destAny._storage, std::move(*(T*)(&srcAny._storage.internal_storage)));
			}
			else
			{
				Construct(destAny._storage, std::move(*(T*)(srcAny._storage.external_storage)));
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
				return handler<decltype(pThis->type().name())>().GetStorage(*pThis);
				break;
			case operation::Destroy:
				handler<decltype(pThis->type().name())>().Destroy(*const_cast<any*>(pThis));
				break;
			case operation::Copy:
				handler<decltype(pThis->type().name())>().Copy(*pThis, *pOther);
				break;
			case operation::Move:
				handler<decltype(pThis->type().name())>().Move(*pThis, *pOther);
				break;
			case operation::TypeInfo:
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
			_handler_func =  std::move(other._handler_func);
		}
	}

	template<class T>
	any(T&& value, std::enable_if_t<!std::is_same_v<std::decay_t<T>, any>>)
	{
		handler<T>().Construct(_storage, std::forward<T>(value));
		_handler_func = &handler<T>::handler_func;
	}

	template<class T, class... Args, typename = std::enable_if_t<std::conjunction_v<
									 std::is_copy_constructible<std::decay_t<T>>, 
									 std::is_constructible<std::decay_t<T>, Args...>>>>
		explicit any(std::in_place_type_t<T>, Args&& ...)
	{

	}
	template<class T, class U, class...Args>
	explicit any(std::in_place_type_t<T>, std::initializer_list<U>, Args&& ...) = delete; //might not do

	~any() = default;

	any& operator=(const any& rhs);
	any& operator=(any&& rhs) noexcept;
	template<class T>
	any& operator=(T&& rhs) = delete;

	template<class T, class... Args>
	void emplace(Args&& ...args)
	{
		handler<T>().Construct(_storage, std::forward<Args>(args)...);
		_handler_func = &handler<T>::handler_func;
	}

	template<class T, class U, class... Args>
	std::decay_t<T>& emplace(std::initializer_list<U>, Args&& ...) = delete;
	void reset() noexcept = delete;
	void swap(any& rhs) noexcept = delete;

	bool has_value() const noexcept
	{
		return _handler_func != nullptr;
	}

	const std::type_info& type() const noexcept;

	template<class ValueType>
	const ValueType* any_cast(const any* pAny)
	{
		if constexpr (std::is_trivially_constructible_v<ValueType>)
		{
			auto get = (void*)(&pAny->_storage.internal_storage);
			return static_cast<const ValueType*>(get);
		}
		else
		{
			return static_cast<const ValueType*>(pAny->_storage.external_storage);
		}
	}

private:
	storage _storage;
	void* (*_handler_func)(operation, const any*, any*) = nullptr;
};
