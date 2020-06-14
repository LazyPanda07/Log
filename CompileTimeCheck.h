#pragma once

#include <type_traits>

template<typename T>
struct is_iterable
{
private:
	template<typename U>
	static constexpr decltype(std::declval<U>().begin(), std::true_type()) get(int);

	template<typename>
	static constexpr std::false_type get(...);

public:
	using type = decltype(get<T>(0));

	enum
	{
		value = type::value
	};
};

template<typename T>
inline constexpr bool is_iterable_v = is_iterable<T>::value;