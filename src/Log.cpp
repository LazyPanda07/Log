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
	return (ostringstream() << "thread id: " << this_thread::get_id()).str();
}

void Log::write(const string& data)
{
	unique_lock<mutex> lock(writeMutex);

	currentLogFileSize += data.size();

	if (currentLogFileSize >= log_constants::logFileSize || !checkDate())
	{
		this->nextLogFile();
	}

	logFile << data << endl;
}

void Log::nextLogFile()
{
	tm curTime = this->getGMTTime();
	string curDate;

	this->getDate(curDate, &curTime);

	for (const auto& i : filesystem::directory_iterator(basePath))
	{
		string checkDate = i.path().filename().string();
		checkDate.resize(log_constants::cPlusPlusDateSize);

		if (curDate == checkDate)
		{
			for (const auto& j : filesystem::directory_iterator(i))
			{
				if (this->checkFileSize(j))
				{
					logFile.open(j.path(), ios::app);

					currentLogFilePath = j.path();

					currentLogFileSize = filesystem::file_size(currentLogFilePath);

					return;
				}
			}
		}
	}

	this->newLogFolder();

	logFile.close();

	string format = this->getFullCurrentDate();

	currentLogFilePath /= format;
	currentLogFilePath.replace_extension(log_constants::fileExtension);

	logFile.open(currentLogFilePath);

	currentLogFileSize = 0;
}

void Log::newLogFolder()
{
	filesystem::path current(basePath);
	tm calendarTime = this->getGMTTime();
	string curDate;

	this->getDate(curDate, &calendarTime);

	current /= curDate;

	filesystem::create_directories(current);

	currentLogFilePath = move(current);
}

bool Log::checkDate() const
{
	tm calendarTime = this->getGMTTime();

	string currentDate;
	string logFileDate = currentLogFilePath.filename().string();
	logFileDate.resize(log_constants::cPlusPlusDateSize);

	this->getDate(currentDate, &calendarTime);

	return logFileDate == currentDate;
}

bool Log::checkFileSize(const filesystem::path& filePath) const
{
	return filesystem::file_size(filePath) < log_constants::logFileSize;
}

void Log::getDate(string& outDate, const tm* time) const
{
	outDate.resize(log_constants::cDateSize);

	switch (logDateFormat)
	{
	case Log::dateFormat::DMY:
		strftime(outDate.data(), log_constants::cDateSize, "%d-%m-%Y", time);
		break;

	case Log::dateFormat::MDY:
		strftime(outDate.data(), log_constants::cDateSize, "%m-%d-%Y", time);
		break;

	case Log::dateFormat::YMD:
		strftime(outDate.data(), log_constants::cDateSize, "%Y-%m-%d", time);
		break;

	default:
		break;
	}

	outDate.resize(log_constants::cPlusPlusDateSize);
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

string Log::getFullCurrentDate() const
{
	string format;
	tm calendarTime = this->getGMTTime();

	format.resize(20);

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

void Log::init(dateFormat logDateFormat, const filesystem::path& pathToLogs, uintmax_t defaultLogFileSize)
{
	unique_lock<mutex> lock(writeMutex);

	this->logDateFormat = logDateFormat;
	basePath = pathToLogs.empty() ? filesystem::current_path() / "logs" : pathToLogs;
	currentLogFilePath = basePath;
	
	log_constants::logFileSize = defaultLogFileSize;

	if (filesystem::exists(currentLogFilePath) && filesystem::is_directory(currentLogFilePath))
	{
		this->nextLogFile();
	}
	else if (filesystem::exists(currentLogFilePath) && !filesystem::is_directory(currentLogFilePath))
	{
		throw runtime_error(currentLogFilePath.string() + " must be directory");
	}
	else if (!filesystem::exists(currentLogFilePath))
	{
		filesystem::create_directories(currentLogFilePath);

		this->nextLogFile();
	}
}

Log::Log()
{
	this->init();
}

Log& Log::getInstance()
{
	static Log instance;

	return instance;
}

string Log::getLogLibraryVersion()
{
	string version = "1.0.0";

	return version;
}

void Log::configure(dateFormat logDateFormat, const filesystem::path& pathToLogs, uintmax_t defaultLogFileSize)
{
	Log::getInstance().init(logDateFormat, pathToLogs);
}

const filesystem::path& Log::getCurrentLogFilePath()
{
	return Log::getInstance().currentLogFilePath;
}

#ifndef __LINUX__
#pragma warning (pop)
#endif
