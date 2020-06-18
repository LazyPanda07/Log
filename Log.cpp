#include "Log.h"

#include <iostream>
#include <ctime>

#pragma warning (push)
#pragma warning (disable: 26812)

using namespace std;

void Log::nextLogFile()
{
	filesystem::directory_iterator it(currentLogFilePath.parent_path());

	time_t epoch;
	tm curTime;
	string curDate;
	curDate.resize(11);

	time(&epoch);

	gmtime_s(&curTime, &epoch);

	strftime(curDate.data(), curDate.size(), "%d-%m-%Y", &curTime);

	curDate.pop_back();

	for (const auto& i : it)
	{
		if (filesystem::file_size(i) < logFileSize)
		{
			string checkDate = i.path().filename().string();
			checkDate.resize(10);

			if (curDate == checkDate)
			{
				logFile.open(i, ios::app);

				currentLogFilePath = i;

				return;
			}
		}
	}

	newLogDirectory();

	logFile.close();

	time_t epochTime;
	string format;
	format.resize(20);
	tm calendarTime;

	time(&epochTime);

	gmtime_s(&calendarTime, &epochTime);

	strftime(format.data(), format.size(), "%d-%m-%Y %H-%M-%S", &calendarTime);

	format.pop_back();

	logFile.open(currentLogFilePath.append(format).replace_extension(".log"));
}

void Log::newLogDirectory()
{
	filesystem::path cur(filesystem::current_path());
	time_t epochTime;
	tm calendarTime;
	string curDate;
	curDate.resize(11);

	cur.append("logs");

	time(&epochTime);

	gmtime_s(&calendarTime, &epochTime);

	strftime(curDate.data(), curDate.size(), "%d-%m-%Y", &calendarTime);

	curDate.pop_back();

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
	currentDate.resize(11);
	logFileDate.resize(10);

	strftime(currentDate.data(), currentDate.size(), "%d-%m-%Y", &calendarTime);

	currentDate.pop_back();

	if (logFileDate == currentDate)
	{
		return true;
	}

	return false;
}

void Log::init()
{
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