#include "Log.h"

#include <iostream>
#include <sstream>
#include <ctime>

#pragma warning (push)
#pragma warning (disable: 26812)

using namespace std;

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
	filesystem::path cur(filesystem::current_path());
	time_t epochTime;
	tm calendarTime;
	string curDate;

	cur.append("logs");

	time(&epochTime);

	gmtime_s(&calendarTime, &epochTime);

	getDate(curDate, &calendarTime);

	cur.append(curDate);

	filesystem::create_directory(cur);

	currentLogFilePath = move(cur);
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
	time_t epochTime;
	tm calendarTime;

	time(&epochTime);

	gmtime_s(&calendarTime, &epochTime);

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

string Log::getFullCurrentDate()
{
	time_t epochTime;
	string format;
	format.resize(20);
	tm calendarTime;

	time(&epochTime);

	gmtime_s(&calendarTime, &epochTime);

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

void Log::init(dateFormat logDateFormat, bool endlAfterLog)
{
	Log::endlAfterLog = endlAfterLog;
	Log::logDateFormat = logDateFormat;
	currentLogFilePath = filesystem::current_path();

	currentLogFilePath.append("logs");

	if (filesystem::exists(currentLogFilePath) && filesystem::is_directory(currentLogFilePath))
	{
		nextLogFile();
	}
	else if (filesystem::exists(currentLogFilePath) && !filesystem::is_directory(currentLogFilePath))
	{
		cout << currentLogFilePath << " must be directory" << endl;
	}
	else if (!filesystem::exists(currentLogFilePath))
	{
		filesystem::create_directory(currentLogFilePath);

		nextLogFile();
	}
}

#pragma warning (pop)
