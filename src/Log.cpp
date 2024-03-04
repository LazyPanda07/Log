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

Log::dateFormat Log::dateFormatFromString(const string& source) const
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
	
	throw invalid_argument("Can't convert source to dateFormat");
}

bool Log::validation(const string& format, size_t count) const
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

string Log::getCurrentThreadId() const
{
	return (ostringstream() << "thread id = " << this_thread::get_id()).str();
}

void Log::write(const string& data)
{
	unique_lock<mutex> lock(writeMutex);

	currentLogFileSize += data.size();

	if (currentLogFileSize >= logFileSize || !checkDate())
	{
		nextLogFile();
	}

	logFile << data << endl;
}

void Log::nextLogFile()
{
	filesystem::directory_iterator it(currentLogFilePath.filename() == parentFolder ? currentLogFilePath : currentLogFilePath.parent_path());

	tm curTime = getGMTTime();
	string curDate;

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
					logFile.open(j.path(), ios::app);

					currentLogFilePath = j.path();

					currentLogFileSize = filesystem::file_size(currentLogFilePath);

					return;
				}
			}
		}
	}

	newLogFolder();

	logFile.close();

	string format = Log::getFullCurrentDate();

	logFile.open(currentLogFilePath.append(format).replace_extension(fileExtension));
}

void Log::newLogFolder()
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

bool Log::checkDate() const
{
	tm calendarTime = getGMTTime();

	string currentDate;
	string logFileDate = currentLogFilePath.filename().string();
	logFileDate.resize(cPlusPlusDateSize);

	getDate(currentDate, &calendarTime);

	return logFileDate == currentDate;
}

bool Log::checkFileSize(const filesystem::path& filePath) const
{
	return filesystem::file_size(filePath) < logFileSize;
}

void Log::getDate(string& outDate, const tm* time) const
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

Log::Log()
{
	Log::configure();
}

Log& Log::getInstance()
{
	static Log instance;

	return instance;
}

tm Log::getGMTTime() const
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

string Log::getFullCurrentDate()
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

string Log::getLogLibraryVersion()
{
	string version = "1.0.0";

	return version;
}

void Log::configure(dateFormat logDateFormat, const filesystem::path& pathToLogs)
{
	Log& instance = Log::getInstance();

	unique_lock<mutex> lock(instance.writeMutex);

	instance.logDateFormat = logDateFormat;
	instance.basePath = pathToLogs.empty() ? filesystem::current_path() : pathToLogs;
	instance.currentLogFilePath = instance.basePath;
	
	if (instance.currentLogFilePath == filesystem::current_path())
	{
		instance.currentLogFilePath /= "logs";
	}	

	if (filesystem::exists(instance.currentLogFilePath) && filesystem::is_directory(instance.currentLogFilePath))
	{
		instance.nextLogFile();
	}
	else if (filesystem::exists(instance.currentLogFilePath) && !filesystem::is_directory(instance.currentLogFilePath))
	{
		cerr << instance.currentLogFilePath << " must be directory" << endl;
	}
	else if (!filesystem::exists(instance.currentLogFilePath))
	{
		filesystem::create_directories(instance.currentLogFilePath);

		instance.nextLogFile();
	}
}

bool Log::isInitialized()
{
	return filesystem::exists(currentLogFilePath);
}

const filesystem::path& Log::getCurrentLogFilePath()
{
	return currentLogFilePath;
}

#ifndef __LINUX__
#pragma warning (pop)
#endif
