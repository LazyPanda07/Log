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

	void getDate(std::string& outDate, const tm* time) const;

	tm getGMTTime() const;

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
	/// @brief Current date with selected dateFormat
	/// @return 
	static std::string getFullCurrentDate();

	/**
	* @brief Get Log library version
	*/
	static std::string getLogLibraryVersion();

	/**
	* @brief Additional configuration
	* @param logDateFormat One of DMY, MDY, YMD
	* @param pathToLogs Path to logs folder
	*/
	static void configure(dateFormat logDateFormat = dateFormat::DMY, const std::filesystem::path& pathToLogs = "");

	static bool isInitialized();

	static const std::filesystem::path& getCurrentLogFilePath();

	template<typename... Args>
	static void info(std::string&& format, std::string_view category, Args&&... args);

	template<typename... Args>
	static void warning(std::string&& format, std::string_view category, Args&&... args);

	template<typename... Args>
	static void error(std::string&& format, std::string_view category, Args&&... args);

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
	if (!validation(format, sizeof...(args)))
	{
		std::cerr << "Not enough arguments for format string" << std::endl;

		return;
	}

	stringFormat(format, std::forward<Args>(args)...);

	std::string additionalInformation;
	additionalInformation.reserve(additionalInformationSize);

	additionalInformation += '[' + getFullCurrentDate() + " UTC][" 
		+ getCurrentThreadId() + "] " +
		+ category + ": ";

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

	write(format);
}
