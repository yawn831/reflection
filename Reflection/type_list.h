#pragma once
template <typename... Args>
struct type_list {
	static constexpr size_t size = sizeof...(Args);
};

namespace detail {

	template <typename>
	struct head;

	template <typename T, typename... Remains>
	struct head<type_list<T, Remains...>> {
		using type = T;
	};

	template <typename>
	struct tail;

	template <typename T, typename... Remains>
	struct tail<type_list<T, Remains...>> {
		using type = type_list<Remains...>;
	};


	template <typename, size_t>
	struct nth;

	template <typename T, typename... Remains>
	struct nth<type_list<T, Remains...>, 0> {
		using type = T;
	};

	template <typename T, typename... Remains, size_t N>
	struct nth<type_list<T, Remains...>, N> {
		using type = typename nth<type_list<Remains...>, N - 1>::type;
	};
}

template <typename TypeList>
using head = typename detail::head<TypeList>::type;

template <typename TypeList>
using tail = typename detail::tail<TypeList>::type;

template <typename TypeList, size_t N>
using nth = typename detail::nth<TypeList, N>::type;