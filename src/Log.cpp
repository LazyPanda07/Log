#include "Log.h"

#include <sstream>
#include <vector>
#include <thread>
#include <format>

#ifndef __LINUX__
#pragma warning (push)
#pragma warning (disable: 26812)
#endif

using namespace std;

static constexpr uint16_t dateSize = 10;
static constexpr uint16_t fullDateSize = 17;

Log::dateFormat Log::dateFormatFromString(const string& source)
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

string Log::getCurrentThreadId() const
{
	return (ostringstream() << "thread id: " << this_thread::get_id()).str();
}

void Log::write(const string& data, level type)
{
	unique_lock<mutex> lock(writeMutex);

	currentLogFileSize += data.size();

	if (currentLogFileSize >= log_constants::logFileSize || !this->checkDate())
	{
		this->nextLogFile();
	}

	logFile << data << endl;

	switch (type)
	{
	case Log::level::info:
	case Log::level::warning:
		if (outputStream)
		{
			(*outputStream) << data << endl;
		}

		break;

	case Log::level::error:
	case Log::level::fatalError:
		if (errorStream)
		{
			(*errorStream) << data << endl;
		}
		else if (outputStream)
		{
			(*outputStream) << data << endl;
		}
	}
}

void Log::nextLogFile()
{
	string currentDate = this->getCurrentDate();

	for (const auto& i : filesystem::directory_iterator(basePath))
	{
		string checkDate = i.path().filename().string();
		checkDate.resize(dateSize);

		if (currentDate == checkDate)
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

	logFile.open
	(
		(currentLogFilePath /= this->getFullCurrentDate()) += log_constants::fileExtension
	);

	currentLogFileSize = 0;
}

void Log::newLogFolder()
{
	filesystem::path current(basePath / this->getCurrentDate());

	filesystem::create_directories(current);

	currentLogFilePath = move(current);
}

bool Log::checkDate() const
{
	string logFileDate = currentLogFilePath.filename().string();
	
	logFileDate.resize(dateSize);

	return logFileDate == this->getCurrentDate();
}

bool Log::checkFileSize(const filesystem::path& filePath) const
{
	return filesystem::file_size(filePath) < log_constants::logFileSize;
}

string Log::getCurrentDate() const
{
	chrono::system_clock::time_point currentDate = chrono::system_clock::now();
	chrono::year_month_day ymd(chrono::floor<chrono::days>(currentDate));
	string dateFormat;
	chrono::day day = ymd.day();
	chrono::month month = ymd.month();
	chrono::year year = ymd.year();

	switch (logDateFormat)
	{
	case Log::dateFormat::DMY:
		dateFormat = "{0}.{1:%m}.{2}";
		break;

	case Log::dateFormat::MDY:
		dateFormat = "{1:%m}.{0}.{2}";
		break;

	case Log::dateFormat::YMD:
		dateFormat = "{2}.{1:%m}.{0}";
		break;

	default:
		break;
	}

	return vformat(dateFormat, make_format_args(day, month, year));
}

string Log::getFullCurrentDate() const
{
	chrono::system_clock::time_point currentDate = chrono::system_clock::now();
	chrono::year_month_day ymd(chrono::floor<chrono::days>(currentDate));
	chrono::hh_mm_ss<chrono::system_clock::duration> hms(currentDate.time_since_epoch());
	string fullDateFormat;
	int hours = hms.hours().count() % 24;
	int minutes = hms.minutes().count();
	chrono::seconds::rep seconds = hms.seconds().count();
	chrono::day day = ymd.day();
	chrono::month month = ymd.month();
	chrono::year year = ymd.year();
	
	fullDateFormat.reserve(fullDateSize);
	
	fullDateFormat += "-{3}.{4}.{5}";

	switch (logDateFormat)
	{
	case dateFormat::DMY:
		fullDateFormat.insert(0, "{0}.{1:%m}.{2}");
		break;

	case dateFormat::MDY:
		fullDateFormat.insert(0, "{1:%m}.{0}.{2}");
		break;

	case dateFormat::YMD:
		fullDateFormat.insert(0, "{2}.{1:%m}.{0}");
		break;

	default:
		break;
	}

	return vformat(fullDateFormat, make_format_args(day, month, year, hours, minutes, seconds));
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

Log::Log() :
	outputStream(nullptr),
	errorStream(nullptr)
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
	string version = "1.3.0";

	return version;
}

void Log::configure(dateFormat logDateFormat, const filesystem::path& pathToLogs, uintmax_t defaultLogFileSize)
{
	Log::getInstance().init(logDateFormat, pathToLogs, defaultLogFileSize);
}

void Log::configure(const string& logDateFormat, const std::filesystem::path& pathToLogs, uintmax_t defaultLogFileSize)
{
	Log::getInstance().init(Log::dateFormatFromString(logDateFormat), pathToLogs, defaultLogFileSize);
}

void Log::duplicateLog(ostream& outputStream)
{
	Log::getInstance().outputStream = &outputStream;
}

void Log::duplicateErrorLog(ostream& errorStream)
{
	Log::getInstance().errorStream = &errorStream;
}

const filesystem::path& Log::getCurrentLogFilePath()
{
	return Log::getInstance().currentLogFilePath;
}

#ifndef __LINUX__
#pragma warning (pop)
#endif
