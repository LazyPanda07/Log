#include <gtest/gtest.h>

#include "Log.h"

using namespace std::string_literals;

TEST(CompileTimeLog, Configuration)
{
	Log::configure(Log::DateFormat::DMY);
	Log::nextLogFile(true);
}

TEST(CompileTimeLog, Logging)
{
	int firstLine = __LINE__;
	int secondLine = __LINE__;
	int thirdLine = __LINE__;

	Log::info<"Information message on line {}", "LogInformation">(firstLine);

	Log::warning<"Warning message on line {}", "LogWarning">(secondLine);

	Log::error<"Error message on line {}", "LogError">(thirdLine);

	std::ifstream in(Log::getCurrentLogFilePath());
	std::string temp = (std::ostringstream() << in.rdbuf()).str();

	ASSERT_NE(temp.find("INFO"), std::string::npos);
	ASSERT_NE(temp.find("Information message on line "s + std::to_string(firstLine)), std::string::npos);

	ASSERT_NE(temp.find("WARNING"), std::string::npos);
	ASSERT_NE(temp.find("Warning message on line "s + std::to_string(secondLine)), std::string::npos);

	ASSERT_NE(temp.find("ERROR"), std::string::npos);
	ASSERT_NE(temp.find("Error message on line "s + std::to_string(thirdLine)), std::string::npos);
}

TEST(CompileTimeLog, ChangingLogFile)
{
	static constexpr size_t cycles = 2'500'000;

	std::filesystem::path currentLogFile = Log::getCurrentLogFilePath();

#ifdef NDEBUG
	auto start = std::chrono::high_resolution_clock::now();
#endif

	for (size_t i = 0; i < cycles; i++)
	{
		Log::info<"Log some information with current index {} and line {}", "LogTest">(i, __LINE__);
	}

#ifdef NDEBUG
	auto end = std::chrono::high_resolution_clock::now();
	auto resultSeconds = static_cast<double>((end - start).count()) / std::chrono::high_resolution_clock::period::den;

	std::cout << resultSeconds << " seconds" << std::endl;
	std::cout << resultSeconds / cycles << " seconds per message" << std::endl;
#endif

	ASSERT_NE(Log::getCurrentLogFilePath(), currentLogFile);
}

TEST(CompileTimeLog, DebugLogging)
{
	int firstLine = __LINE__;
	int secondLine = __LINE__;
	int thirdLine = __LINE__;

	LOG_DEBUG_INFO("Information message on line {}", "LogInformation", firstLine);

	LOG_DEBUG_WARNING("Warning message on line {}", "LogWarning", secondLine);

	LOG_DEBUG_ERROR("Error message on line {}", "LogError", thirdLine);

	std::ifstream in(Log::getCurrentLogFilePath());
	std::string temp = (std::ostringstream() << in.rdbuf()).str();

#ifdef NDEBUG
	ASSERT_EQ(temp.find("Information message on line "s + std::to_string(firstLine)), std::string::npos);
	ASSERT_EQ(temp.find("Warning message on line "s + std::to_string(secondLine)), std::string::npos);
	ASSERT_EQ(temp.find("Error message on line "s + std::to_string(thirdLine)), std::string::npos);
#else
	ASSERT_NE(temp.find("Information message on line "s + std::to_string(firstLine)), std::string::npos);
	ASSERT_NE(temp.find("Warning message on line "s + std::to_string(secondLine)), std::string::npos);
	ASSERT_NE(temp.find("Error message on line "s + std::to_string(thirdLine)), std::string::npos);
#endif
}

TEST(CompileTimeLog, VerbosityLogging)
{
	Log::setVerbosityLevel(Log::VerbosityLevel::warning);

	Log::info<"This info message should not be logged", "LogInformation">();
	Log::warning<"LogWarning: This info message should be logged", "LogWarning">();

	Log::setVerbosityLevel(Log::VerbosityLevel::error);

	Log::info<"This info message should not be logged", "LogInformation">();
	Log::info<"This info message should not be logged", "LogWarning">();
	Log::error<"LogError: This info message should be logged", "LogError">();

	Log::setVerbosityLevel(Log::VerbosityLevel::verbose);

	std::ifstream in(Log::getCurrentLogFilePath());
	std::string temp = (std::ostringstream() << in.rdbuf()).str();

	ASSERT_NE(temp.find("LogWarning: This info message should be logged"), std::string::npos);
	ASSERT_NE(temp.find("LogError: This info message should be logged"), std::string::npos);
}
