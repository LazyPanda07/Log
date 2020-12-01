#pragma once

#include <string>

inline constexpr uintmax_t logFileSize = 128 * 1024 * 1024;	//128 MB
inline constexpr size_t cDateSize = 11;
inline constexpr size_t cPlusPlusDateSize = 10;	//without \0 from strftime function

inline constexpr std::string_view parentFolder = "logs";
inline constexpr std::string_view fileExtension = ".log";