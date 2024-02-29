#pragma once

#ifdef LOG_DLL
#ifdef __LINUX__
#define LOG_API __attribute__((visibility("default")))
#else
#define LOG_API __declspec(dllexport)

#pragma warning(disable: 4251)
#endif
#else
#define LOG_API 
#endif // LOG_DLL

#include <iostream>
#include <fstream>
#include <string>
#include <filesystem>
#include <mutex>

#include "CompileTimeCheck.h"
#include "LogConstants.h"

class LOG_API Log
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

	/// @brief Current date with selected dateFormat
	/// @return 
	static std::string getFullCurrentDate();

private:
	static inline std::filesystem::path currentLogFilePath;
	static inline std::filesystem::path basePath;
	static inline std::mutex writeLock;
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

	/// @brief Create directory with current date
	static void newLogFolder();

	/// @brief Check arguments count
	/// @param format 
	/// @param count 
	/// @return 
	static bool validation(const std::string& format, size_t count);

	/// @brief Check current log file date equals current date
	/// @return 
	static bool checkDate();

	/// @brief Check file size is less than logFileSize
	/// @param filePath 
	/// @return 
	static bool checkFileSize(const std::filesystem::path& filePath);

	/// @brief Using ostringstream
	/// @return Information about current thread with next format: thread id = <id><tabulation>
	static std::string getCurrentThread();

	/// @brief Get date in dateFormat
	/// @param outDate 
	/// @param time 
	static void getDate(std::string& outDate, const tm* time);

	/// @brief Basic log function
	/// @tparam ...Args 
	/// @param type 
	/// @param format 
	/// @param ...args 
	template<typename... Args>
	static void log(level type, std::string&& format, Args&&... args);

	static tm getGMTTime();

private:
	Log() = delete;

	Log(const Log&) = delete;

	Log(Log&&) noexcept = delete;

	Log& operator = (const Log&) = delete;

	Log& operator = (Log&&) noexcept = delete;

	~Log() = delete;

public:
	/**
	* @brief Get Log library version
	*/
	static std::string getVersion();

	/// @brief Init logFile
	/// @param logDateFormat 
	/// @param endlAfterLog 
	static void init(dateFormat logDateFormat = dateFormat::DMY, bool endlAfterLog = true, const std::filesystem::path& pathToLogs = "");

	static bool isInitialized();

	static const std::filesystem::path getCurrentLogFilePath();

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
