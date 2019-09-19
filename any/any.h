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
*/


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
	const std::type_info& type() const noexcept = delete;

private:
	storage _storage;
};
