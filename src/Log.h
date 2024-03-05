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

#include <chrono>
#include <fstream>
#include <string>
#include <filesystem>
#include <mutex>

#include "CompileTimeCheck.h"
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

private:
	std::filesystem::path currentLogFilePath;
	std::filesystem::path basePath;
	std::mutex writeMutex;
	std::ofstream logFile;
	dateFormat logDateFormat;
	size_t currentLogFileSize;

private:
	dateFormat dateFormatFromString(const std::string& source) const;

	bool validation(const std::string& format, size_t count) const;

	std::string getCurrentThreadId() const;

	void write(const std::string& data);

	void nextLogFile();

	void newLogFolder();

	bool checkDate() const;

	bool checkFileSize(const std::filesystem::path& filePath) const;

	std::string getCurrentDate() const;

	std::string getFullCurrentDate() const;

	void init(dateFormat logDateFormat = dateFormat::DMY, const std::filesystem::path& pathToLogs = "", uintmax_t defaultLogFileSize = log_constants::logFileSize);

private:
	Log();

	Log(const Log&) = delete;

	Log(Log&&) noexcept = delete;

	Log& operator = (const Log&) = delete;

	Log& operator = (Log&&) noexcept = delete;

	~Log() = default;

private:
	static Log& getInstance();

private:
	template<typename T, typename... Args>
	void stringFormat(std::string& format, T&& value, Args&&... args);

	template<typename... Args>
	void stringFormat(std::string& format, Args&&... args);

	template<typename... Args>
	void log(level type, std::string&& format, std::string_view category, Args&&... args);

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
	static void configure(dateFormat logDateFormat = dateFormat::DMY, const std::filesystem::path& pathToLogs = "", uintmax_t defaultLogFileSize = log_constants::logFileSize);

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
	static void info(std::string&& format, std::string_view category, Args&&... args);

	/**
	 * @brief Log some warning message
	 * @tparam ...Args
	 * @param format Warning message with {} brackets for insertions
	 * @param category Log category
	 * @param ...args Insertions
	 */
	template<typename... Args>
	static void warning(std::string&& format, std::string_view category, Args&&... args);

	/**
	 * @brief Log some error
	 * @tparam ...Args
	 * @param format Error message with {} brackets for insertions
	 * @param category Log category
	 * @param ...args Insertions
	 */
	template<typename... Args>
	static void error(std::string&& format, std::string_view category, Args&&... args);

	/**
	 * @brief Log and exit
	 * @tparam ...Args 
	 * @param format Fatal error message with {} brackets for insertions
	 * @param category Log category
	 * @param exitCode Exit code
	 * @param ...args 
	 */
	template<typename... Args>
	static void fatalError(std::string&& format, std::string_view category, int exitCode, Args&&... args);
};

template<typename... Args>
void Log::info(std::string&& format, std::string_view category, Args&&... args)
{
	Log::getInstance().log(level::info, std::move(format), category, std::forward<Args>(args)...);
}

template<typename... Args>
void Log::warning(std::string&& format, std::string_view category, Args&&... args)
{
	Log::getInstance().log(level::warning, std::move(format), category, std::forward<Args>(args)...);
}

template<typename... Args>
void Log::error(std::string&& format, std::string_view category, Args&&... args)
{
	Log::getInstance().log(level::error, std::move(format), category, std::forward<Args>(args)...);
}

template<typename... Args>
void Log::fatalError(std::string&& format, std::string_view category, int exitCode, Args&&... args)
{
	Log::getInstance().log(level::fatalError, std::move(format), category, std::forward<Args>(args)...);

	exit(exitCode);
}

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
void Log::log(level type, std::string&& format, std::string_view category, Args&&... args)
{
	if (!this->validation(format, sizeof...(args)))
	{
		throw std::runtime_error("Not enough arguments for format string");
	}

	this->stringFormat(format, std::forward<Args>(args)...);

	std::string additionalInformation;
	additionalInformation.reserve(log_constants::additionalInformationSize);

	additionalInformation
		.append("[")
		.append(getFullCurrentDate())
		.append(" UTC][")
		.append(getCurrentThreadId())
		.append("] ")
		.append(category).append(": ");

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

	additionalInformation += ": ";

	format.insert(format.begin(), additionalInformation.begin(), additionalInformation.end());

	this->write(format);
}
