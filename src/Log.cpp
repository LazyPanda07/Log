#include "Log.h"

#include <sstream>
#include <vector>
#include <thread>
#include <format>

#ifdef __LINUX__
#include <sys/types.h>
#include <unistd.h> 
#else
#include <Windows.h>
#endif

using namespace std;

static constexpr uint16_t dateSize = 10;
static constexpr uint16_t fullDateSize = 17;

static unique_ptr<Log> instance;

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

void Log::write(const string& data, level type)
{
	unique_lock<mutex> lock(writeMutex);

	currentLogFileSize += data.size();

	if (currentLogFileSize >= Log::logFileSize || !this->checkDate())
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
		(currentLogFilePath /= this->getFullCurrentDateFileName()) += Log::fileExtension
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
	return filesystem::file_size(filePath) < Log::logFileSize;
}

string Log::getCurrentDate() const
{
	auto now = chrono::floor<chrono::days>(chrono::system_clock::now());

	switch (logDateFormat)
	{
	case Log::dateFormat::DMY:
		return vformat("{0:%d.%m.%Y}", make_format_args(now));

	case Log::dateFormat::MDY:
		return vformat("{0:%m.%d.%Y}", make_format_args(now));

	case Log::dateFormat::YMD:
		return vformat("{0:%Y.%m.%d}", make_format_args(now));

	default:
		throw runtime_error(format("Wrong dateFormat in {}", __FUNCTION__));
	}

	return {};
}

string Log::getFullCurrentDateFileName() const
{
	auto now = chrono::floor<chrono::seconds>(chrono::system_clock::now());

	switch (logDateFormat)
	{
	case dateFormat::DMY:
		return vformat("{0:%d.%m.%Y-%H.%M.%S}", make_format_args(now));

	case dateFormat::MDY:
		return vformat("{0:%m.%d.%Y-%H.%M.%S}", make_format_args(now));

	case dateFormat::YMD:
		return vformat("{0:%Y.%m.%d-%H.%M.%S}", make_format_args(now));

	default:
		throw runtime_error(format("Wrong dateFormat in {}", __FUNCTION__));
	}

	return {};
}

string Log::getFullCurrentDateUTC() const
{
	auto now = chrono::floor<chrono::seconds>(chrono::system_clock::now());
	string formatString = "[";

	switch (logDateFormat)
	{
	case dateFormat::DMY:
		formatString += "{0:%d.%m.%Y-%H.%M.%S}";

		break;

	case dateFormat::MDY:
		formatString += "{0:%m.%d.%Y-%H.%M.%S}";

		break;

	case dateFormat::YMD:
		formatString += "{0:%Y.%m.%d-%H.%M.%S}";

		break;

	default:
		throw runtime_error(format("Wrong dateFormat in {}", __FUNCTION__));
	}

	formatString += " UTC]";

	return vformat(formatString, make_format_args(now));
}

string Log::getFullCurrentDateLocal() const
{
	auto now = chrono::get_tzdb().current_zone()->to_local(chrono::floor<chrono::seconds>(chrono::system_clock::now()));
	string_view zoneName = chrono::get_tzdb().current_zone()->name();
	string formatString = "[";

	switch (logDateFormat)
	{
	case dateFormat::DMY:
		formatString += "{0:%d.%m.%Y-%H.%M.%S}";

		break;

	case dateFormat::MDY:
		formatString += "{0:%m.%d.%Y-%H.%M.%S}";

		break;

	case dateFormat::YMD:
		formatString += "{0:%Y.%m.%d-%H.%M.%S}";

		break;

	default:
		throw runtime_error(format("Wrong dateFormat in {}", __func__));
	}

	formatString += " {1}]";

	return vformat(formatString, make_format_args(now, zoneName));
}

string Log::getProcessName() const
{
	return format("[process name: {}]", executablePath.string());
}

string Log::getProcessId() const
{
	return format("[process id: {}]", executableProcessId);
}

string Log::getThreadId() const
{
	return (ostringstream() << "[thread id: " << this_thread::get_id() << ']').str();
}

void Log::initModifiers(uint64_t flags)
{
	modifiers.clear();

	if (flags & AdditionalInformation::utcDate)
	{
		modifiers.emplace_back(bind(&Log::getFullCurrentDateUTC, this));
	}

	if (flags & AdditionalInformation::localDate)
	{
		modifiers.emplace_back(bind(&Log::getFullCurrentDateLocal, this));
	}

	if (flags & AdditionalInformation::processName)
	{
		modifiers.emplace_back(bind(&Log::getProcessName, this));
	}

	if (flags & AdditionalInformation::processId)
	{
		modifiers.emplace_back(bind(&Log::getProcessId, this));
	}

	if (flags & AdditionalInformation::threadId)
	{
		modifiers.emplace_back(bind(&Log::getThreadId, this));
	}
}

void Log::initExecutableInformation()
{
	constexpr size_t bufferSize = 4096;
	char buffer[bufferSize];

#ifdef __LINUX__
	executableProcessId = static_cast<int64_t>(getpid());
	FILE* file = popen(format("realpath /proc/{}/exe", executableProcessId).data(), "r");

	fread(buffer, sizeof(char), bufferSize, file);

	executablePath = buffer;

	pclose(file);
#else
	executableProcessId = static_cast<int64_t>(GetCurrentProcessId());

	HANDLE handle = OpenProcess
	(
		PROCESS_QUERY_LIMITED_INFORMATION,
		FALSE,
		static_cast<DWORD>(executableProcessId)
	);

	if (handle)
	{
		DWORD size = bufferSize;

		if (QueryFullProcessImageNameA(handle, NULL, buffer, &size))
		{
			executablePath = buffer;
		}
		else
		{
			cerr << "Error GetModuleBaseNameA : " << GetLastError() << endl;
		}

		CloseHandle(handle);
	}
	else
	{
		cerr << "Error OpenProcess : " << GetLastError() << endl;
	}
#endif
}

void Log::init(dateFormat logDateFormat, const filesystem::path& pathToLogs, uintmax_t defaultLogFileSize, uint64_t flags)
{
	unique_lock<mutex> lock(writeMutex);

	this->logDateFormat = logDateFormat;
	basePath = pathToLogs.empty() ? filesystem::current_path() / "logs" : pathToLogs;
	currentLogFilePath = basePath;
	this->flags = flags;

	Log::logFileSize = defaultLogFileSize;

	this->initModifiers(flags);
	this->initExecutableInformation();

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

Log::Log(dateFormat logDateFormat, const filesystem::path& pathToLogs, uintmax_t defaultLogFileSize, uint64_t flags) :
	outputStream(nullptr),
	errorStream(nullptr)
{
	this->init(logDateFormat, pathToLogs, defaultLogFileSize, flags);
}

Log& Log::getInstance()
{
	if (!instance)
	{
		instance = unique_ptr<Log>(new Log());
	}

	return *instance;
}

string Log::getLogLibraryVersion()
{
	string version = "1.6.0";

	return version;
}

uint64_t Log::createFlags(const vector<string>& values)
{
#define ADD_FLAG(flagName) { #flagName, AdditionalInformation::flagName }

	const unordered_map<string, uint64_t> flagNameToFlag =
	{
		ADD_FLAG(utcDate),
		ADD_FLAG(localDate),
		ADD_FLAG(processName),
		ADD_FLAG(processId),
		ADD_FLAG(threadId)
	};
	uint64_t flags = 0;

	for (const string& value : values)
	{
		flags |= flagNameToFlag.at(value);
	}

	return flags;
#undef ADD_FLAG
}

void Log::configure(dateFormat logDateFormat, const filesystem::path& pathToLogs, uintmax_t defaultLogFileSize, uint64_t flags)
{
	if (instance)
	{
		return;
	}

	instance = unique_ptr<Log>(new Log(logDateFormat, pathToLogs, defaultLogFileSize, flags));
}

void Log::configure(const string& logDateFormat, const std::filesystem::path& pathToLogs, uintmax_t defaultLogFileSize, uint64_t flags)
{
	if (instance)
	{
		return;
	}

	instance = unique_ptr<Log>(new Log(Log::dateFormatFromString(logDateFormat), pathToLogs, defaultLogFileSize, flags));
}

void Log::duplicateLog(ostream& outputStream)
{
	Log::getInstance().outputStream = &outputStream;
}

void Log::duplicateErrorLog(ostream& errorStream)
{
	Log::getInstance().errorStream = &errorStream;
}

bool Log::isValid()
{
	return static_cast<bool>(instance);
}

const filesystem::path& Log::getCurrentLogFilePath()
{
	return Log::getInstance().currentLogFilePath;
}

const filesystem::path& Log::getExecutablePath()
{
	return Log::getInstance().executablePath;
}

int64_t Log::getExecutableProcessId()
{
	return Log::getInstance().executableProcessId;
}
