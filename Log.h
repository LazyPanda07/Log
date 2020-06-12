#pragma once

#include <fstream>
#include <filesystem>
#include <mutex>

inline constexpr uint32_t logFileSize = 1 * 1024 * 1024 * 1024;	//1 GB
inline std::mutex writeLock;

class Log
{
private:
	static inline std::ofstream logFile;

private:
	static void nextLogFile(std::filesystem::path&& logs) noexcept;

private:
	Log() = delete;

	Log(const Log&) = delete;

	~Log() = delete;

public:
	enum class logType
	{
		info,
		warning,
		error,
		fatalError
	};

public:
	//init logFile
	static void init();

	//basic log function
	static void log(logType type, const std::string& message);

};