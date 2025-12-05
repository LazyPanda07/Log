#pragma once

#include <string_view>

namespace log_utils
{
	template<size_t N>
	struct FixedString
	{
		char value[N];

		constexpr FixedString(char const (&str)[N])
		{
			for (size_t i = 0; i < N; i++)
			{
				value[i] = str[i];
			}
		}

		constexpr operator std::string_view() const
		{
			return { value, N - 1 };
		}
	};

	template<size_t N>
	FixedString(char const (&)[N]) -> FixedString<N>;

	template<FixedString Source>
	consteval size_t countBracePairs()
	{
		size_t result = 0;
		std::string_view formatString = Source;

		for (size_t i = 0; i + 1 < formatString.size(); i++)
		{
			if (formatString[i] == '{' && formatString[i + 1] == '}')
			{
				result++;
			}
		}

		return result;
	}

	template<FixedString Format, typename... Args>
	concept LogFormat = countBracePairs<Format>() == sizeof...(Args);
}
