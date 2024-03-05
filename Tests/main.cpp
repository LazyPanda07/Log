#include <chrono>

#include "gtest/gtest.h"

#include "Log.h"

TEST(Log, Logging)
{
    Log::info("Information message on line {}", "LogInformation", __LINE__);

    Log::warning("Warning message on line {}", "LogWarning", __LINE__);

    Log::error("Error message on line {}", "LogError", __LINE__);

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
}

TEST(Log, ChangingLogFile)
{
    std::filesystem::path currentLogFile = Log::getCurrentLogFilePath();
    
#ifdef NDEBUG
    auto start = std::chrono::high_resolution_clock::now();
#endif

    for (size_t i = 0; i < 5'000'000; i++)
    {
        Log::info("Log some information with current index {} and line {}", "LogTest", i, __LINE__);
    }

#ifdef NDEBUG
    auto end = std::chrono::high_resolution_clock::now();
    auto resultSeconds = static_cast<double>((end - start).count()) / std::chrono::high_resolution_clock::period::den;

    std::cout << resultSeconds << " seconds" << std::endl;
    std::cout << resultSeconds / 5'000'000 << " seconds per message" << std::endl;
#endif

    ASSERT_NE(Log::getCurrentLogFilePath(), currentLogFile);
}

int main(int argc, char** argv)
{
	testing::InitGoogleTest(&argc, argv);

	return RUN_ALL_TESTS();
}
