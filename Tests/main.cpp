#include "gtest/gtest.h"

#include "Log.h"

TEST(Log, Logging)
{
	Log::init();

    Log::info("Information message on line {}", __LINE__);

    Log::warning("Warning message on line {}", __LINE__);

    Log::error("Error message on line {}", __LINE__);

    Log::fatalError("Fatal error message on line {}", __LINE__);

    std::ifstream in(Log::getCurrentLogFilePath());
    std::string tem;

    std::getline(in, tem);

    ASSERT_TRUE(tem.find("INFO") != std::string::npos);
    ASSERT_TRUE(tem.find("Information message on line 9") != std::string::npos);

    std::getline(in, tem);

    ASSERT_TRUE(tem.find("WARNING") != std::string::npos);
    ASSERT_TRUE(tem.find("Warning message on line 11") != std::string::npos);

    std::getline(in, tem);

    ASSERT_TRUE(tem.find("ERROR") != std::string::npos);
    ASSERT_TRUE(tem.find("Error message on line 13") != std::string::npos);

    std::getline(in, tem);

    ASSERT_TRUE(tem.find("FATAL_ERROR") != std::string::npos);
    ASSERT_TRUE(tem.find("Fatal error message on line 15") != std::string::npos);
}

TEST(Log, ChangingLogFile)
{
    Log::init();

    std::string currentLogFile = Log::getCurrentLogFilePath().str();

    for (size_t i = 0; i < 1'000'000'000; i++)
    {
        Log::info("Log some information with current index {} and line {}", i, __LINE__);
    }

    ASSERT_NE(Log::getCurrentLogFilePath().str(), currentLogFile);
}

int main(int argc, char** argv)
{
	testing::InitGoogleTest(&argc, argv);

	return RUN_ALL_TESTS();
}
