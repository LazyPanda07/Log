#pragma once

#ifdef LOG_DLL
#ifdef __LINUX__
#define LOG_API_FUNCTION extern "C" __attribute__((visibility("default")))
#else
#define LOG_API_FUNCTION extern "C" __declspec(dllexport)

#pragma warning(disable: 4251)
#endif
#else
#define LOG_API_FUNCTION
#endif // LOG_DLL

#include <iostream>
#include <fstream>
#include <string>
#include <filesystem>
#include <mutex>

#include "CompileTimeCheck.h"
#include "LogConstants.h"

namespace Log
{
	enum class dateFormat
	{
		DMY,
		MDY,
		YMD
	};

	enum class level
	{
		info,
		warning,
		error,
		fatalError
	};

	LOG_API_FUNCTION dateFormat dateFormatFromString(const std::string& source);

	/// @brief Current date with selected dateFormat
	/// @return 
	LOG_API_FUNCTION std::string getFullCurrentDate();

	/**
	* @brief Get Log library version
	*/
	LOG_API_FUNCTION std::string getLogLibraryVersion();

	/// @brief Init logFile
	/// @param logDateFormat 
	/// @param endlAfterLog 
	LOG_API_FUNCTION void init(dateFormat logDateFormat = dateFormat::DMY, bool endlAfterLog = true, const std::filesystem::path& pathToLogs = "");

	LOG_API_FUNCTION bool isInitialized();

	LOG_API_FUNCTION const std::filesystem::path& getCurrentLogFilePath();

	template<typename... Args>
	void info(std::string&& format, Args&&... args);

	template<typename... Args>
	void warning(std::string&& format, Args&&... args);

	template<typename... Args>
	void error(std::string&& format, Args&&... args);

	template<typename... Args>
	void fatalError(std::string&& format, Args&&... args);

	/**
	* @brief For internal usage
	*/
	template<typename T, typename... Args>
	void __stringFormat(std::string& format, T&& value, Args&&... args);

	/**
	* @brief For internal usage
	*/
	template<typename... Args>
	void __stringFormat(std::string& format, Args&&... args);

	/**
	* @brief For internal usage
	*/
	template<typename... Args>
	void __log(level type, std::string&& format, Args&&... args);
};

template<typename... Args>
void Log::info(std::string&& format, Args&&... args)
{
	__log(level::info, std::move(format), std::forward<Args>(args)...);
}

template<typename... Args>
void Log::warning(std::string&& format, Args&&... args)
{
	__log(level::warning, std::move(format), std::forward<Args>(args)...);
}

template<typename... Args>
void Log::error(std::string&& format, Args&&... args)
{
	__log(level::error, std::move(format), std::forward<Args>(args)...);
}

template<typename... Args>
void Log::fatalError(std::string&& format, Args&&... args)
{
	__log(level::fatalError, std::move(format), std::forward<Args>(args)...);
}

template<typename T, typename... Args>
void Log::__stringFormat(std::string& format, T&& value, Args&&... args)
{
	size_t first = format.find('{');
	size_t last = format.find('}') + 1;

	if constexpr (std::is_fundamental_v<std::remove_reference_t<decltype(value)>>)
	{
		format.replace(format.begin() + first, format.begin() + last, std::to_string(value).data());
	}
	else if constexpr (is_iterable_v<decltype(value)>)
	{
		format.replace(format.begin() + first, format.begin() + last, std::begin(value), std::end(value));
	}
	else if constexpr (std::is_constructible_v<std::string_view, decltype(value)>)
	{
		std::string_view tem(value);

		format.replace(format.begin() + first, format.begin() + last, tem.begin(), tem.end());
	}

	__stringFormat(format, std::forward<Args>(args)...);
}

template<typename... Args>
void Log::__stringFormat(std::string& format, Args&&... args)
{
	if constexpr (sizeof...(args))
	{
		__stringFormat(format, std::forward<Args>(args)...);
	}
}

template<typename... Args>
void Log::__log(level type, std::string&& format, Args&&... args)
{
	if (validation(format, sizeof...(args)))
	{
		__stringFormat(format, std::forward<Args>(args)...);

		std::string additionalInformation;
		additionalInformation.reserve(additionalInformationSize);

		additionalInformation += '[';

		switch (type)
		{
		case level::info:
			additionalInformation += "INFO";

			break;

		case level::warning:
			additionalInformation += "WARNING";

			break;

		case level::error:
			additionalInformation += "ERROR";

			break;

		case level::fatalError:
			additionalInformation += "FATAL_ERROR";

			break;

		default:
			return;
		}

		additionalInformation += "] GMT " + getFullCurrentDate() + " " + getCurrentThread() + " ";

		format.insert(format.begin(), additionalInformation.begin(), additionalInformation.end());

		std::unique_lock<std::mutex> lock(writeLock);

		if (std::filesystem::file_size(currentLogFilePath) >= logFileSize || !checkDate())
		{
			nextLogFile();
		}

		logFile << format;

		if (endlAfterLog)
		{
			logFile << std::endl;
		}

		logFile.flush();
	}
	else
	{
		std::cerr << "Not enough arguments for format string" << std::endl;
	}
}
