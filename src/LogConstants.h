#pragma once

#include <string_view>

namespace log_constants
{
	inline uintmax_t logFileSize = 128 * 1024 * 1024;	// 128 MB
	inline constexpr size_t additionalInformationSize = 128;

	inline constexpr std::string_view fileExtension = ".log";
}
