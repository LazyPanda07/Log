#include "Log.h"

#include <iostream>
#include <ctime>

#pragma warning (push)
#pragma warning (disable: 26812)

using namespace std;

void Log::nextLogFile(filesystem::path&& logs) noexcept
{
	filesystem::directory_iterator it(logs);

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

				break;
			}
		}
	}

	if (!logFile.is_open())
	{
		time_t epochTime;
		char format[20]{};
		tm calendarTime;

		time(&epochTime);

		gmtime_s(&calendarTime, &epochTime);

		strftime(format, sizeof(format), "%d-%m-%Y %H-%M-%S", &calendarTime);

		logFile.open(logs.append(format).replace_extension(".log"));
		currentLogFilePath = move(logs);
	}
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

void Log::init()
{
	filesystem::path curPath(filesystem::current_path());

	curPath.append("logs");

	if (filesystem::exists(curPath) && filesystem::is_directory(curPath))
	{
		nextLogFile(move(curPath));
	}
	else if (filesystem::exists(curPath) && !filesystem::is_directory(curPath))
	{
		cout << curPath << " must be directory" << endl;
	}
	else if (!filesystem::exists(curPath))
	{
		filesystem::create_directory(curPath);

		nextLogFile(move(curPath));
	}
}

#pragma warning (pop)