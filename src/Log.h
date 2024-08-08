#pragma once

#define _DISABLE_CONSTEXPR_MUTEX_CONSTRUCTOR

#ifdef __LINUX__
#define LOG_API __attribute__((visibility("default")))
#else
#define LOG_API __declspec(dllexport)

#pragma warning(disable: 4251)
#endif

#include <iostream>
#include <chrono>
#include <fstream>
#include <string>
#include <filesystem>
#include <mutex>
#include <format>
#include <vector>
#include <functional>

#include "LogConstants.h"

#ifdef NDEBUG
#define LOG_DEBUG_INFO(format, category, ...)
#define LOG_DEBUG_WARNING(format, category, ...)
#define LOG_DEBUG_ERROR(format, category, ...)
#define LOG_DEBUG_FATAL_ERROR(format, category, errorCode, ...)
#else
#define LOG_DEBUG_INFO(format, category, ...) Log::info(format, category, __VA_ARGS__)
#define LOG_DEBUG_WARNING(format, category, ...) Log::warning(format, category, __VA_ARGS__)
#define LOG_DEBUG_ERROR(format, category, ...) Log::error(format, category, __VA_ARGS__)
#define LOG_DEBUG_FATAL_ERROR(format, category, errorCode, ...) Log::fatalError(format, category, exitCode, __VA_ARGS__)
#endif

class LOG_API Log
{
public:
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

	enum AdditionalInformation : uint64_t
	{
		utcDate = 1,
		localDate = utcDate << 1,
		processName = localDate << 1,
		processId = processName << 1,
		threadId = processId << 1
	};

private:
	std::ofstream logFile;
	std::mutex writeMutex;
	std::filesystem::path currentLogFilePath;
	std::filesystem::path basePath;
	std::vector<std::function<std::string()>> modifiers;
	uint64_t flags;
	size_t currentLogFileSize;
	std::ostream* outputStream;
	std::ostream* errorStream;
	dateFormat logDateFormat;

private:
	static dateFormat dateFormatFromString(const std::string& source);

	void write(const std::string& data, level type);

	void nextLogFile();

	void newLogFolder();

	bool checkDate() const;

	bool checkFileSize(const std::filesystem::path& filePath) const;

	std::string getCurrentDate() const;

	std::string getFullCurrentDateUTC() const;

	std::string getFullCurrentDateLocal() const;

	std::string getProcessName() const;

	std::string getProcessId() const;

	std::string getThreadId() const;

	void initModifiers(uint64_t flags);

	void init
	(
		dateFormat logDateFormat = dateFormat::DMY,
		const std::filesystem::path& pathToLogs = "",
		uintmax_t defaultLogFileSize = log_constants::logFileSize,
		uint64_t flags = AdditionalInformation::utcDate | AdditionalInformation::processName | AdditionalInformation::processId
	);

private:
	Log();

	Log(dateFormat logDateFormat, const std::filesystem::path& pathToLogs, uintmax_t defaultLogFileSize, uint64_t flags);

	Log(const Log&) = delete;

	Log(Log&&) noexcept = delete;

	Log& operator = (const Log&) = delete;

	Log& operator = (Log&&) noexcept = delete;

	~Log() = default;

	friend struct std::default_delete<Log>;

private:
	static Log& getInstance();

private:
	template<typename... Args>
	void log(level type, std::string_view format, std::string_view category, Args&&... args);

public:
	/**
	 * @brief Get Log library version
	 * @return 
	 */
	static std::string getLogLibraryVersion();

	/**
	* @brief Additional configuration
	* @param logDateFormat One of DMY, MDY, YMD
	* @param pathToLogs Path to logs folder
	* @param defaultLogFileSize Size of each log file in bytes
	*/
	static void configure
	(
		dateFormat logDateFormat = dateFormat::DMY,
		const std::filesystem::path& pathToLogs = "",
		uintmax_t defaultLogFileSize = log_constants::logFileSize,
		uint64_t flags = AdditionalInformation::utcDate | AdditionalInformation::processName | AdditionalInformation::processId
	);

	/**
	* @brief Additional configuration
	* @param logDateFormat One of DMY, MDY, YMD
	* @param pathToLogs Path to logs folder
	* @param defaultLogFileSize Size of each log file in bytes
	*/
	static void configure
	(
		const std::string& logDateFormat = "DMY",
		const std::filesystem::path& pathToLogs = "",
		uintmax_t defaultLogFileSize = log_constants::logFileSize,
		uint64_t flags = AdditionalInformation::utcDate | AdditionalInformation::processName | AdditionalInformation::processId
	);

	/**
	 * @brief Also output log information into stream
	 * @param outputStream 
	 */
	static void duplicateLog(std::ostream& outputStream);

	/**
	 * @brief Also output log error information into stream
	 * @param errorStream
	 */
	static void duplicateErrorLog(std::ostream& errorStream);

	/**
	 * @brief Get current log file path
	 * @tparam ...Args 
	 * @param format 
	 * @param ...args 
	 */
	static const std::filesystem::path& getCurrentLogFilePath();

	/**
	 * @brief Log some information
	 * @tparam ...Args 
	 * @param format Information with {} brackets for insertions
	 * @param category Log category
	 * @param ...args Insertions
	 */
	template<typename... Args>
	static void info(std::string_view format, std::string_view category, Args&&... args);

	/**
	 * @brief Log some warning message
	 * @tparam ...Args
	 * @param format Warning message with {} brackets for insertions
	 * @param category Log category
	 * @param ...args Insertions
	 */
	template<typename... Args>
	static void warning(std::string_view format, std::string_view category, Args&&... args);

	/**
	 * @brief Log some error
	 * @tparam ...Args
	 * @param format Error message with {} brackets for insertions
	 * @param category Log category
	 * @param ...args Insertions
	 */
	template<typename... Args>
	static void error(std::string_view format, std::string_view category, Args&&... args);

	/**
	 * @brief Log and exit
	 * @tparam ...Args 
	 * @param format Fatal error message with {} brackets for insertions
	 * @param category Log category
	 * @param exitCode Exit code
	 * @param ...args 
	 */
	template<typename... Args>
	static void fatalError(std::string_view format, std::string_view category, int exitCode, Args&&... args);
};

template<typename... Args>
void Log::info(std::string_view format, std::string_view category, Args&&... args)
{
	Log::getInstance().log(level::info, format, category, std::forward<Args>(args)...);
}

template<typename... Args>
void Log::warning(std::string_view format, std::string_view category, Args&&... args)
{
	Log::getInstance().log(level::warning, format, category, std::forward<Args>(args)...);
}

template<typename... Args>
void Log::error(std::string_view format, std::string_view category, Args&&... args)
{
	Log::getInstance().log(level::error, format, category, std::forward<Args>(args)...);
}

template<typename... Args>
void Log::fatalError(std::string_view format, std::string_view category, int exitCode, Args&&... args)
{
	Log::getInstance().log(level::fatalError, format, category, std::forward<Args>(args)...);

	exit(exitCode);
}

template<typename... Args>
void Log::log(level type, std::string_view format, std::string_view category, Args&&... args)
{
	std::string result = std::vformat(format, std::make_format_args(args...));
	std::string additionalInformation;

	additionalInformation.reserve(log_constants::additionalInformationSize);

	for (const std::function<std::string()>& modifier : modifiers)
	{
		additionalInformation += modifier();
	}

	additionalInformation.append(category).append(": ");

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
		throw std::runtime_error("Wrong level type");
	}

	additionalInformation += ": ";

	result.insert(result.begin(), additionalInformation.begin(), additionalInformation.end());

	this->write(result, type);
}
