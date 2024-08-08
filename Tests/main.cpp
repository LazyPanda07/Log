#include <chrono>

#include "gtest/gtest.h"

#include "Log.h"

using namespace std::string_literals;

TEST(Log, Configuration)
{
	Log::configure(Log::dateFormat::DMY);
}

TEST(Log, Logging)
{
	int firstLine = __LINE__;
	int secondLine = __LINE__;
	int thirdLine = __LINE__;

	Log::info("Information message on line {}", "LogInformation", firstLine);

	Log::warning("Warning message on line {}", "LogWarning", secondLine);

	Log::error("Error message on line {}", "LogError", thirdLine);

	std::ifstream in(Log::getCurrentLogFilePath());
	std::string temp = (std::ostringstream() << in.rdbuf()).str();

	ASSERT_NE(temp.find("INFO"), std::string::npos);
	ASSERT_NE(temp.find("Information message on line "s + std::to_string(firstLine)), std::string::npos);

	ASSERT_NE(temp.find("WARNING"), std::string::npos);
	ASSERT_NE(temp.find("Warning message on line "s + std::to_string(secondLine)), std::string::npos);

	ASSERT_NE(temp.find("ERROR"), std::string::npos);
	ASSERT_NE(temp.find("Error message on line "s + std::to_string(thirdLine)), std::string::npos);

	ASSERT_NO_THROW(Log::info("Log int", "LogInformation", 5));
	ASSERT_NO_THROW(Log::warning("Log int", "LogWarning", 5));
	ASSERT_NO_THROW(Log::error("Log int", "LogError", 5));

	std::cout << temp << std::endl;
}

TEST(Log, ChangingLogFile)
{
	static constexpr size_t cycles = 2'500'000;

	std::filesystemp::path currentLogFile = Log::getCurrentLogFilePath();

#ifdef NDEBUG
	auto start = std::chrono::high_resolution_clock::now();
#endif

	for (size_t i = 0; i < cycles; i++)
	{
		Log::info("Log some information with current index {} and line {}", "LogTest", i, __LINE__);
	}

#ifdef NDEBUG
	auto end = std::chrono::high_resolution_clock::now();
	auto resultSeconds = static_cast<double>((end - start).count()) / std::chrono::high_resolution_clock::period::den;

	std::cout << resultSeconds << " seconds" << std::endl;
	std::cout << resultSeconds / cycles << " seconds per message" << std::endl;
#endif

	ASSERT_NE(Log::getCurrentLogFilePath(), currentLogFile);
}

TEST(Log, DebugLogging)
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

	std::cout << temp << std::endl;
}

int main(int argc, char** argv)
{
	testing::InitGoogleTest(&argc, argv);

	return RUN_ALL_TESTS();
}
