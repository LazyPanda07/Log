#pragma once

#include <iostream>
#include <fstream>
#include <string>
#include <filesystem>
#include <shared_mutex>

#include "CompileTimeCheck.h"
#include "LogConstants.h"

class Log
{
public:
	enum class dateFormat
	{
		DMY,
		MDY,
		YMD
	};

public:
	static dateFormat dateFormatFromString(const std::string& source);

private:
	static inline std::filesystem::path currentLogFilePath;
	static inline std::shared_mutex writeLock;
	static inline std::ofstream logFile;
	static inline dateFormat logDateFormat;
	static inline bool endlAfterLog;

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

	static void nextLogFile();

	//create directory with current date
	static void newLogFolder();

	//check arguments count
	static bool validation(const std::string& format, size_t count);

	//check current log file date equals current date
	static bool checkDate();

	//check file size is less than logFileSize
	static bool checkFileSize(const std::filesystem::path& filePath);

	//returns current date with next format: days-months-years hours-minutes-seconds
	static std::string getFullCurrentDate();

	//returns information about current thread with next format: thread id = <id><tabulation>
	//using ostringstream
	static std::string getCurrentThread();

	//get date in dateFormat
	static void getDate(std::string& outDate, const tm* time);

	//basic log function
	template<typename... Args>
	static void log(level type, std::string&& format, Args&&... args);

private:
	Log() = delete;

	Log(const Log&) = delete;

	Log(Log&&) noexcept = delete;

	Log& operator = (const Log&) = delete;

	Log& operator = (Log&&) noexcept = delete;

	~Log() = delete;

public:
	//init logFile
	static void init(dateFormat logDateFormat = dateFormat::DMY, bool endlAfterLog = true);

	static bool isInitialized();

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
		additionalInformation.reserve(additionalInformationSize);

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

		additionalInformation += "] GMT " + getFullCurrentDate() + " " + getCurrentThread() + " ";

		format.insert(format.begin(), additionalInformation.begin(), additionalInformation.end());

		writeLock.lock();

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
