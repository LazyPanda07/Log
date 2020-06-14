#pragma once

#include <iostream>
#include <fstream>
#include <string>
#include <filesystem>
#include <mutex>

#include "CompileTimeCheck.h"
#include "Constants.h"

class Log
{
private:
	static inline std::ofstream logFile;
	static inline std::mutex writeLock;
	static inline std::filesystem::path currentLogFilePath;

private:
	enum class level
	{
		info,
		warning,
		error,
		fatalError
	};

private:
	template<typename T, typename... Args>
	static void stringFormat(std::string& format, T&& value, Args&&... args);

	template<typename... Args>
	static void stringFormat(std::string& format, Args&&... args);

	static void nextLogFile(std::filesystem::path&& logs) noexcept;

	//check arguments count
	static bool validation(const std::string& format, size_t count);

	//basic log function
	template<typename... Args>
	static void log(level type, std::string&& format, Args&&... args);

private:
	Log() = delete;

	Log(const Log&) = delete;

	~Log() = delete;

public:
	//init logFile
	static void init();

	template<typename... Args>
	static void info(std::string&& format, Args&&... args);

	template<typename... Args>
	static void warning(std::string&& format, Args&&... args);

	template<typename... Args>
	static void error(std::string&& format, Args&&... args);

	template<typename... Args>
	static void fatalError(std::string&& format, Args&&... args);
};

template<typename T, typename... Args>
void Log::stringFormat(std::string& format, T&& value, Args&&... args)
{
	size_t first = format.find('{');
	size_t last = format.find('}') + 1;

	if constexpr (std::is_fundamental_v<std::remove_reference_t<decltype(value)>>)
	{
		format.replace(std::begin(format) + first, std::begin(format) + last, std::to_string(value).data());
	}
	else if constexpr (is_iterable_v<decltype(value)>)
	{
		format.replace(std::begin(format) + first, std::begin(format) + last, std::begin(value), end(value));
	}
	else if constexpr (std::is_constructible_v<std::string_view, decltype(value)>)
	{
		std::string_view tem(value);

		format.replace(std::begin(format) + first, std::begin(format) + last, std::begin(tem), end(tem));
	}

	stringFormat(format, std::forward<Args>(args)...);
}

template<typename... Args>
void Log::stringFormat(std::string& format, Args&&... args)
{
	if constexpr (sizeof...(args))
	{
		stringFormat(format, std::forward<Args>(args)...);
	}
}

template<typename... Args>
void Log::log(level type, std::string&& format, Args&&... args)
{
	if (validation(format, sizeof...(args)))
	{
		stringFormat(format, std::forward<Args>(args)...);

		std::string additionalInformation;
		additionalInformation.reserve(16);

		additionalInformation += '[';

		switch (type)
		{
		case level::info:
			additionalInformation += "info";

			break;

		case level::warning:
			additionalInformation += "warning";

			break;

		case level::error:
			additionalInformation += "error";

			break;

		case level::fatalError:
			additionalInformation += "fatal error";

			break;

		default:
			return;
		}

		additionalInformation += "] ";

		format.insert(std::begin(format), std::begin(additionalInformation), std::end(additionalInformation));

		writeLock.lock();

		logFile << format << std::endl;

		if (std::filesystem::file_size(currentLogFilePath) >= logFileSize)
		{
			nextLogFile(currentLogFilePath.parent_path());
		}

		writeLock.unlock();
	}
	else
	{
		std::cout << "Not enough arguments for format string" << std::endl;
	}
}

template<typename... Args>
void Log::info(std::string&& format, Args&&... args)
{
	log(level::info, std::move(format), std::forward<Args>(args)...);
}

template<typename... Args>
void Log::warning(std::string&& format, Args&&... args)
{
	log(level::warning, std::move(format), std::forward<Args>(args)...);
}

template<typename... Args>
void Log::error(std::string&& format, Args&&... args)
{
	log(level::error, std::move(format), std::forward<Args>(args)...);
}

template<typename... Args>
void Log::fatalError(std::string&& format, Args&&... args)
{
	log(level::fatalError, std::move(format), std::forward<Args>(args)...);
}