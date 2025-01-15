#pragma once
#include "function_traits.h"
#include "variable_traits.h"

#include <string>
#include <iostream>


template <typename T>
struct TypeInfo;

template <typename RetT, typename... Params>
auto function_pointer_type(RetT(*)(Params...)) -> RetT(*)(Params...);

template <typename RetT, typename Class, typename... Params>
auto function_pointer_type(RetT(Class::*)(Params...)) -> RetT(Class::*)(Params...);

template <typename RetT, typename Class, typename... Params>
auto function_pointer_type(RetT(Class::*)(Params...)const) -> RetT(Class::*)(Params...) const;

template <auto F>
using function_pointer_type_t = decltype(function_pointer_type(F));

template <auto F>
using function_traits_t = function_traits<function_pointer_type_t<F>>;

template <typename T>
struct is_function {
	static constexpr bool value = std::is_function_v<std::remove_pointer_t<T>> ||
		std::is_member_function_pointer_v<T>;
};

template <typename T>
constexpr bool is_function_v = is_function<T>::value;

template <typename T, bool isFunc>
struct basic_field_traits;

template <typename T>
struct basic_field_traits<T, true> : public function_traits<T> {
	using traits = function_traits<T>;

	constexpr bool is_member() const {
		return traits::is_member;
	}

	constexpr bool is_const() const {
		return traits::is_const;
	}

	constexpr bool is_function() const {
		return true;
	}

	constexpr bool is_variable() const {
		return false;
	}

	constexpr size_t param_count() const {
		return std::tuple_size_v<typename traits::args>;
	}
};

template <typename T>
struct basic_field_traits<T, false> : public variable_traits<T> {
	using traits = variable_traits<T>;

	constexpr bool is_member() const {
		return traits::is_member;
	}

	constexpr bool is_const() const {
		return traits::is_const;
	}

	constexpr bool is_function() const {
		return false;
	}

	constexpr bool is_variable() const {
		return true;
	}
};

template <typename T>
struct field_traits : public basic_field_traits<T, is_function_v<T>> {
	constexpr field_traits(T&& pointer, std::string_view name) : pointer{ pointer }, name{ name } {}

	T pointer;
	std::string_view name;
};

#define BEGIN_CLASS(X) template <> struct TypeInfo<X> {

#define functions(...)\
	static constexpr auto functions = std::make_tuple(__VA_ARGS__);
#define func(F)\
	field_traits {F, #F} 

#define END_CLASS()\
}\
;

template <typename T>
constexpr auto reflected_type() {
	return TypeInfo<T>{};
}

template <size_t idx, typename... Args, typename Class>
void VisitTuple(const std::tuple<Args...>& tuple, Class* instance) {
	using tuple_type = std::tuple<Args...>;
	if constexpr (idx >= std::tuple_size_v<tuple_type>) {
		return;
	}
	else {
		if constexpr (auto elem = std::get<idx>(tuple); elem.param_count() == 0) {
			// 调用成员函数
			(instance->*elem.pointer)();
		}

		// 递归的进行遍历
		VisitTuple<idx + 1>(tuple, instance);
	}
}
