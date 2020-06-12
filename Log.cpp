#include "Log.h"

#include <iostream>
#include <ctime>

#pragma warning (push)
#pragma warning (disable: 26812)

using namespace std;

void Log::nextLogFile(filesystem::path&& logs) noexcept
{
	filesystem::directory_iterator it(logs);

	for (const auto& i : it)
	{
		if (filesystem::file_size(i) < logFileSize)
		{
			logFile.open(i, ios::app);

			break;
		}
	}

	if (!logFile.is_open())
	{
		time_t epochTime;
		char format[32]{};
		tm calendarTime;

		time(&epochTime);

		gmtime_s(&calendarTime, &epochTime);

		strftime(format, sizeof(format), "%d-%m-%Y %H-%M-%S", &calendarTime);

		logFile.open(logs.append(format).replace_extension(".log"));
	}
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

void Log::log(logType type, const string& message)
{
	writeLock.lock();

	logFile << '[';

	switch (type)
	{
	case logType::info:
		logFile << "info";
		break;
	case logType::warning:
		logFile << "warning";
		break;
	case logType::error:
		logFile << "error";
		break;
	case logType::fatalError:
		logFile << "fatal error";
		break;
	default:
		writeLock.unlock();
		return;

		break;
	}

	logFile << "] ";

	logFile << message << endl;

	writeLock.unlock();
}

#pragma warning (pop)