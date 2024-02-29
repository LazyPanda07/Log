#include "Log.h"

#include <sstream>
#include <ctime>

#ifdef __LINUX__
#include <time.h>
#endif

#ifndef __LINUX__
#pragma warning (push)
#pragma warning (disable: 26812)
#endif

using namespace std;

Log::dateFormat Log::dateFormatFromString(const string& source)
{
	if (source == "DMY")
	{
		return Log::dateFormat::DMY;
	}
	else if (source == "MDY")
	{
		return Log::dateFormat::MDY;
	}
	else if (source == "YMD")
	{
		return Log::dateFormat::YMD;
	}
	else
	{
		throw invalid_argument("Can't convert source to dateFormat");
	}
}

string Log::getFullCurrentDate()
{
	string format;
	format.resize(20);
	tm calendarTime = Log::getGMTTime();

	switch (logDateFormat)
	{
	case Log::dateFormat::DMY:
		strftime(format.data(), format.size(), "%d-%m-%Y %H-%M-%S", &calendarTime);
		break;

	case Log::dateFormat::MDY:
		strftime(format.data(), format.size(), "%m-%d-%Y %H-%M-%S", &calendarTime);
		break;

	case Log::dateFormat::YMD:
		strftime(format.data(), format.size(), "%Y-%m-%d %H-%M-%S", &calendarTime);
		break;

	default:
		break;
	}

	format.pop_back();

	return format;
}

void Log::nextLogFile()
{
	filesystem::directory_iterator it(currentLogFilePath.filename() == parentFolder ? currentLogFilePath : currentLogFilePath.parent_path());

	time_t epoch;
	tm curTime;
	string curDate;

	time(&epoch);

	gmtime_s(&curTime, &epoch);

	getDate(curDate, &curTime);

	for (const auto& i : it)
	{
		string checkDate = i.path().filename().string();
		checkDate.resize(cPlusPlusDateSize);

		if (curDate == checkDate)
		{
			filesystem::directory_iterator logFiles(i);

			for (const auto& j : logFiles)
			{
				if (checkFileSize(j))
				{
					logFile.open(j, ios::app);

					currentLogFilePath = j;

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

void Log::newLogFolder()
{
	filesystem::path current(basePath);
	tm calendarTime = Log::getGMTTime();
	string curDate;

	current /= "logs";

	getDate(curDate, &calendarTime);

	current /= curDate;

	filesystem::create_directories(current);

	currentLogFilePath = move(current);
}

bool Log::validation(const string& format, size_t count)
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

bool Log::checkDate()
{
	tm calendarTime = Log::getGMTTime();

	string currentDate;
	string logFileDate = currentLogFilePath.filename().string();
	logFileDate.resize(cPlusPlusDateSize);

	getDate(currentDate, &calendarTime);

	return logFileDate == currentDate;
}

bool Log::checkFileSize(const filesystem::path& filePath)
{
	return filesystem::file_size(filePath) < logFileSize;
}

string Log::getCurrentThread()
{
	ostringstream format;

	format << "thread id = " << this_thread::get_id() << "	";

	return format.str();
}

void Log::getDate(string& outDate, const tm* time)
{
	outDate.resize(cDateSize);

	switch (logDateFormat)
	{
	case Log::dateFormat::DMY:
		strftime(outDate.data(), cDateSize, "%d-%m-%Y", time);
		break;

	case Log::dateFormat::MDY:
		strftime(outDate.data(), cDateSize, "%m-%d-%Y", time);
		break;

	case Log::dateFormat::YMD:
		strftime(outDate.data(), cDateSize, "%Y-%m-%d", time);
		break;

	default:
		break;
	}

	outDate.resize(cPlusPlusDateSize);
}

tm Log::getGMTTime();
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

string Log::getVersion()
{
	string version = "1.0.0";

	return version;
}

void Log::init(dateFormat logDateFormat, bool endlAfterLog, const filesystem::path& pathToLogs)
{
	Log::endlAfterLog = endlAfterLog;
	Log::logDateFormat = logDateFormat;
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

bool Log::isInitialized()
{
	return filesystem::exists(currentLogFilePath);
}

const filesystem::path Log::getCurrentLogFilePath()
{
	return Log::currentLogFilePath;
}

#ifndef __LINUX__
#pragma warning (pop)
#endif
