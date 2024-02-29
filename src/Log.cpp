#include "Log.h"

#include <sstream>
#include <ctime>
#include <vector>
#include <thread>

#ifdef __LINUX__
#include <time.h>
#endif

#ifndef __LINUX__
#pragma warning (push)
#pragma warning (disable: 26812)
#endif

using namespace std;

static inline std::filesystem::path currentLogFilePath;
static inline std::filesystem::path basePath;
static inline std::mutex writeLock;
static inline std::ofstream logFile;
static inline dateFormat logDateFormat;
static inline bool endlAfterLog;

static void nextLogFile();

static void newLogFolder();

static bool checkFileSize(const std::filesystem::path& filePath);

static void getDate(std::string& outDate, const tm* time);

static tm getGMTTime();

namespace Log
{
	dateFormat dateFormatFromString(const string& source)
	{
		if (source == "DMY")
		{
			return dateFormat::DMY;
		}
		else if (source == "MDY")
		{
			return dateFormat::MDY;
		}
		else if (source == "YMD")
		{
			return dateFormat::YMD;
		}
		else
		{
			throw invalid_argument("Can't convert source to dateFormat");
		}
	}

	string getFullCurrentDate()
	{
		string format;
		format.resize(20);
		tm calendarTime = getGMTTime();

		switch (logDateFormat)
		{
		case dateFormat::DMY:
			strftime(format.data(), format.size(), "%d-%m-%Y %H-%M-%S", &calendarTime);
			break;

		case dateFormat::MDY:
			strftime(format.data(), format.size(), "%m-%d-%Y %H-%M-%S", &calendarTime);
			break;

		case dateFormat::YMD:
			strftime(format.data(), format.size(), "%Y-%m-%d %H-%M-%S", &calendarTime);
			break;

		default:
			break;
		}

		format.pop_back();

		return format;
	}

	string getLogLibraryVersion()
	{
		string version = "1.0.0";

		return version;
	}

	void init(dateFormat logDateFormat, bool endlAfterLog, const filesystem::path& pathToLogs)
	{
		endlAfterLog = endlAfterLog;
		logDateFormat = logDateFormat;
		basePath = pathToLogs.empty() ? filesystem::current_path() : pathToLogs;
		currentLogFilePath = basePath;

		currentLogFilePath /= "logs";

		if (filesystem::exists(currentLogFilePath) && filesystem::is_directory(currentLogFilePath))
		{
			nextLogFile();
		}
		else if (filesystem::exists(currentLogFilePath) && !filesystem::is_directory(currentLogFilePath))
		{
			cerr << currentLogFilePath << " must be directory" << endl;
		}
		else if (!filesystem::exists(currentLogFilePath))
		{
			filesystem::create_directories(currentLogFilePath);

			nextLogFile();
		}
	}

	bool isInitialized()
	{
		return filesystem::exists(currentLogFilePath);
	}

	const filesystem::path& getCurrentLogFilePath()
	{
		return currentLogFilePath;
	}

	bool __validation(const string& format, size_t count)
	{
		vector<size_t> values;
		size_t next = format.find("{}");

		values.reserve(count);

		while (next != string::npos)
		{
			values.push_back(next);
			next = format.find("{}", next + 1);
		}

		return values.size() == count;
	}

	string __getCurrentThread()
	{
		ostringstream format;

		format << "thread id = " << this_thread::get_id() << "	";

		return format.str();
	}

	bool __checkDate()
	{
		tm calendarTime = getGMTTime();

		string currentDate;
		string logFileDate = currentLogFilePath.filename().string();
		logFileDate.resize(cPlusPlusDateSize);

		getDate(currentDate, &calendarTime);

		return logFileDate == currentDate;
	}
}

void nextLogFile()
{
	filesystem::directory_iterator it(currentLogFilePath.filename() == parentFolder ? currentLogFilePath : currentLogFilePath.parent_path());

	tm curTime = getGMTTime();
	string curDate;

	getDate(curDate, &curTime);

	for (const auto& i : it)
	{
		string __checkDate = i.path().filename().string();
		__checkDate.resize(cPlusPlusDateSize);

		if (curDate == __checkDate)
		{
			filesystem::directory_iterator logFiles(i);

			for (const auto& j : logFiles)
			{
				if (checkFileSize(j))
				{
					logFile.open(j.path(), ios::app);

					currentLogFilePath = j.path();

					return;
				}
			}
		}
	}

	newLogFolder();

	logFile.close();

	string format = getFullCurrentDate();

	logFile.open(currentLogFilePath.append(format).replace_extension(fileExtension));
}

void newLogFolder()
{
	filesystem::path current(basePath);
	tm calendarTime = getGMTTime();
	string curDate;

	current /= "logs";

	getDate(curDate, &calendarTime);

	current /= curDate;

	filesystem::create_directories(current);

	currentLogFilePath = move(current);
}

bool checkFileSize(const filesystem::path& filePath)
{
	return filesystem::file_size(filePath) < logFileSize;
}

void getDate(string& outDate, const tm* time)
{
	outDate.resize(cDateSize);

	switch (logDateFormat)
	{
	case dateFormat::DMY:
		strftime(outDate.data(), cDateSize, "%d-%m-%Y", time);
		break;

	case dateFormat::MDY:
		strftime(outDate.data(), cDateSize, "%m-%d-%Y", time);
		break;

	case dateFormat::YMD:
		strftime(outDate.data(), cDateSize, "%Y-%m-%d", time);
		break;

	default:
		break;
	}

	outDate.resize(cPlusPlusDateSize);
}

tm getGMTTime()
{
	tm calendarTime;
	time_t epochTime;

	time(&epochTime);

#ifdef __LINUX__
	gmtime_r(&epochTime, &calendarTime);
#else
	gmtime_s(&calendarTime, &epochTime);
#endif

	return calendarTime;
}

#ifndef __LINUX__
#pragma warning (pop)
#endif
