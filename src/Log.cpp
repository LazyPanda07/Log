#include "Log.h"

#include <sstream>
#include <vector>
#include <thread>
#include <format>

#ifdef __LINUX__
#include <sys/types.h>
#include <unistd.h>
#elif defined(__ANDROID__)
#include <ctime>
#else
#include <Windows.h>
#endif

static constexpr uint16_t dateSize = 10;
static constexpr uint16_t fullDateSize = 17;

static std::unique_ptr<Log> instance;

Log::DateFormat Log::dateFormatFromString(const std::string& source)
{
	if (source == "DMY")
	{
		return DateFormat::DMY;
	}
	else if (source == "MDY")
	{
		return DateFormat::MDY;
	}
	else if (source == "YMD")
	{
		return DateFormat::YMD;
	}

	throw std::invalid_argument("Can't convert source to DateFormat");
}

std::string_view Log::getLocalTimeZoneName()
{
#ifdef __ANDROID__
	return tzname[0];
#else
	return std::chrono::get_tzdb().current_zone()->name();
#endif
}

bool Log::verbosityFilter(Level level)
{
	switch (verbosityLevel)
	{
	case VerbosityLevel::verbose:
		return true;

	case VerbosityLevel::warning:
		return level >= Level::warning;

	case VerbosityLevel::error:
		return level >= Level::error;

	default:
		throw std::runtime_error(std::format("Wrong verbosityLevel in {}", __FUNCTION__));
	}

	return false;
}

void Log::write(const std::string& data, Level type)
{
	if (!this->verbosityFilter(type))
	{
		return;
	}

	std::unique_lock<std::mutex> lock(writeMutex);

	currentLogFileSize += data.size();

	if (currentLogFileSize >= Log::logFileSize || !this->checkDate())
	{
		this->nextLogFile();
	}

	logFile << data << std::endl;

	switch (type)
	{
	case Log::Level::info:
	case Log::Level::warning:
		if (outputStream)
		{
			(*outputStream) << data << std::endl;
		}

		break;

	case Log::Level::error:
	case Log::Level::fatalError:
		if (errorStream)
		{
			(*errorStream) << data << std::endl;
		}
		else if (outputStream)
		{
			(*outputStream) << data << std::endl;
		}
	}
}

void Log::nextLogFile()
{
	std::string currentDate = this->getCurrentDate();

	for (const auto& i : std::filesystem::directory_iterator(basePath))
	{
		std::string checkDate = i.path().filename().string();
		checkDate.resize(dateSize);

		if (currentDate == checkDate)
		{
			for (const auto& j : std::filesystem::directory_iterator(i))
			{
				if (this->checkFileSize(j))
				{
					logFile.open(j.path(), std::ios::app);

					currentLogFilePath = j.path();

					currentLogFileSize = std::filesystem::file_size(currentLogFilePath);

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
	std::filesystem::path current(basePath / this->getCurrentDate());

	std::filesystem::create_directories(current);

	currentLogFilePath = std::move(current);
}

bool Log::checkDate() const
{
	std::string logFileDate = currentLogFilePath.filename().string();

	logFileDate.resize(dateSize);

	return logFileDate == this->getCurrentDate();
}

bool Log::checkFileSize(const std::filesystem::path& filePath) const
{
	return std::filesystem::file_size(filePath) < Log::logFileSize;
}

std::string Log::getCurrentDate() const
{
	auto now = std::chrono::floor<std::chrono::days>(std::chrono::system_clock::now());

	switch (logDateFormat)
	{
	case Log::DateFormat::DMY:
		return std::vformat("{0:%d.%m.%Y}", std::make_format_args(now));

	case Log::DateFormat::MDY:
		return std::vformat("{0:%m.%d.%Y}", std::make_format_args(now));

	case Log::DateFormat::YMD:
		return std::vformat("{0:%Y.%m.%d}", std::make_format_args(now));

	default:
		throw std::runtime_error(std::format("Wrong DateFormat in {}", __FUNCTION__));
	}

	return {};
}

std::string Log::getFullCurrentDateFileName() const
{
	auto now = std::chrono::floor<std::chrono::seconds>(std::chrono::system_clock::now());

	switch (logDateFormat)
	{
	case DateFormat::DMY:
		return std::vformat("{0:%d.%m.%Y-%H.%M.%S}", std::make_format_args(now));

	case DateFormat::MDY:
		return std::vformat("{0:%m.%d.%Y-%H.%M.%S}", std::make_format_args(now));

	case DateFormat::YMD:
		return std::vformat("{0:%Y.%m.%d-%H.%M.%S}", std::make_format_args(now));

	default:
		throw std::runtime_error(std::format("Wrong DateFormat in {}", __FUNCTION__));
	}

	return {};
}

std::string Log::getFullCurrentDateUTC() const
{
	auto now = std::chrono::floor<std::chrono::seconds>(std::chrono::system_clock::now());
	std::string formatString = "[";

	switch (logDateFormat)
	{
	case DateFormat::DMY:
		formatString += "{0:%d.%m.%Y-%H.%M.%S}";

		break;

	case DateFormat::MDY:
		formatString += "{0:%m.%d.%Y-%H.%M.%S}";

		break;

	case DateFormat::YMD:
		formatString += "{0:%Y.%m.%d-%H.%M.%S}";

		break;

	default:
		throw std::runtime_error(std::format("Wrong DateFormat in {}", __FUNCTION__));
	}

	formatString += " UTC]";

	return std::vformat(formatString, make_format_args(now));
}

std::string Log::getFullCurrentDateLocal() const
{
#ifdef __ANDROID__
	auto now = std::chrono::system_clock::now();
	time_t currentTime = std::chrono::system_clock::to_time_t(now);
	std::string formatString;

	tm localTime;
	localtime_r(&currentTime, &localTime);

	switch (logDateFormat)
	{
	case DateFormat::DMY:
		formatString += "%d.%m.%Y-%H.%M.%S";

		break;

	case DateFormat::MDY:
		formatString += "%m.%d.%Y-%H.%M.%S";

		break;

	case DateFormat::YMD:
		formatString += "%Y.%m.%d-%H.%M.%S";

		break;

	default:
		throw std::runtime_error(std::format("Wrong DateFormat in {}", __func__));
	}

	std::string currentDateLocal(256, '\0');

	currentDateLocal.resize(strftime(currentDateLocal.data(), currentDateLocal.size(), formatString.data(), &localTime));

	return std::format("[{} {}]", currentDateLocal, Log::getLocalTimeZoneName());
#else
	auto now = std::chrono::get_tzdb().current_zone()->to_local(std::chrono::floor<std::chrono::seconds>(std::chrono::system_clock::now()));
	std::string_view zoneName = Log::getLocalTimeZoneName();
	std::string formatString = "[";

	switch (logDateFormat)
	{
	case DateFormat::DMY:
		formatString += "{0:%d.%m.%Y-%H.%M.%S}";

		break;

	case DateFormat::MDY:
		formatString += "{0:%m.%d.%Y-%H.%M.%S}";

		break;

	case DateFormat::YMD:
		formatString += "{0:%Y.%m.%d-%H.%M.%S}";

		break;

	default:
		throw std::runtime_error(std::format("Wrong DateFormat in {}", __func__));
	}

	formatString += " {1}]";

	return std::vformat(formatString, std::make_format_args(now, zoneName));
#endif
}

std::string Log::getProcessName() const
{
	return std::format("[process name: {}]", executablePath.string());
}

std::string Log::getProcessId() const
{
	return std::format("[process id: {}]", executableProcessId);
}

std::string Log::getThreadId() const
{
	return (std::ostringstream() << "[thread id: " << std::this_thread::get_id() << ']').str();
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
	FILE* file = popen(std::format("realpath /proc/{}/exe", executableProcessId).data(), "r");

	fread(buffer, sizeof(char), bufferSize, file);

	executablePath = buffer;

	std::string temp = executablePath.string();

	temp.erase(remove(temp.begin(), temp.end(), '\n'), temp.cend());

	executablePath = temp;

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
			std::cerr << "Error GetModuleBaseNameA : " << GetLastError() << std::endl;
		}

		CloseHandle(handle);
	}
	else
	{
		std::cerr << "Error OpenProcess : " << GetLastError() << std::endl;
	}
#endif
}

void Log::init(DateFormat logDateFormat, const std::filesystem::path& pathToLogs, uintmax_t defaultLogFileSize, uint64_t flags, VerbosityLevel verbosityLevel)
{
	std::unique_lock<std::mutex> lock(writeMutex);

	this->logDateFormat = logDateFormat;
	basePath = pathToLogs.empty() ? std::filesystem::current_path() / "logs" : pathToLogs;
	currentLogFilePath = basePath;
	this->flags = flags;
	this->verbosityLevel = verbosityLevel;

	Log::logFileSize = defaultLogFileSize;

	this->initModifiers(flags);
	this->initExecutableInformation();

#ifdef __ANDROID__
	tzset();
#endif

	if (std::filesystem::exists(currentLogFilePath) && std::filesystem::is_directory(currentLogFilePath))
	{
		this->nextLogFile();
	}
	else if (std::filesystem::exists(currentLogFilePath) && !std::filesystem::is_directory(currentLogFilePath))
	{
		throw std::runtime_error(currentLogFilePath.string() + " must be directory");
	}
	else if (!std::filesystem::exists(currentLogFilePath))
	{
		std::filesystem::create_directories(currentLogFilePath);

		this->nextLogFile();
	}
}

Log::Log() :
	outputStream(nullptr),
	errorStream(nullptr)
{
	this->init();
}

Log::Log(DateFormat logDateFormat, const std::filesystem::path& pathToLogs, uintmax_t defaultLogFileSize, uint64_t flags, VerbosityLevel verbosityLevel) :
	outputStream(nullptr),
	errorStream(nullptr)
{
	this->init(logDateFormat, pathToLogs, defaultLogFileSize, flags);
}

Log& Log::operator +=(const std::string& message)
{
	this->log(Level::info, "{}", "LogTemp", message);

	return *this;
}

Log& Log::getInstance()
{
	if (!instance)
	{
		instance = std::unique_ptr<Log>(new Log());
	}

	return *instance;
}

std::string Log::getLogLibraryVersion()
{
	std::string version = "1.9.2";

	return version;
}

uint64_t Log::createFlags(const std::vector<std::string>& values)
{
#define ADD_FLAG(flagName) { #flagName, AdditionalInformation::flagName }

	const std::unordered_map<std::string, uint64_t> flagNameToFlag =
	{
		ADD_FLAG(utcDate),
		ADD_FLAG(localDate),
		ADD_FLAG(processName),
		ADD_FLAG(processId),
		ADD_FLAG(threadId)
	};
	uint64_t flags = 0;

	for (const std::string& value : values)
	{
		flags |= flagNameToFlag.at(value);
	}

	return flags;
#undef ADD_FLAG
}

void Log::configure(DateFormat logDateFormat, const std::filesystem::path& pathToLogs, uintmax_t defaultLogFileSize, uint64_t flags, VerbosityLevel verbosityLevel)
{
	if (instance)
	{
		return;
	}

	instance = std::unique_ptr<Log>(new Log(logDateFormat, pathToLogs, defaultLogFileSize, flags, verbosityLevel));
}

void Log::configure(const std::string& logDateFormat, const std::filesystem::path& pathToLogs, uintmax_t defaultLogFileSize, uint64_t flags, VerbosityLevel verbosityLevel)
{
	if (instance)
	{
		return;
	}

	instance = std::unique_ptr<Log>(new Log(Log::dateFormatFromString(logDateFormat), pathToLogs, defaultLogFileSize, flags, verbosityLevel));
}

void Log::duplicateLog(std::ostream& outputStream)
{
	Log::getInstance().outputStream = &outputStream;
}

void Log::duplicateErrorLog(std::ostream& errorStream)
{
	Log::getInstance().errorStream = &errorStream;
}

bool Log::isValid()
{
	return static_cast<bool>(instance);
}

void Log::setVerbosityLevel(VerbosityLevel level)
{
	Log::getInstance().verbosityLevel = level;
}

const std::filesystem::path& Log::getCurrentLogFilePath()
{
	return Log::getInstance().currentLogFilePath;
}

const std::filesystem::path& Log::getExecutablePath()
{
	return Log::getInstance().executablePath;
}

int64_t Log::getExecutableProcessId()
{
	return Log::getInstance().executableProcessId;
}
