#include <filesystem>
#include <iostream>

#include "gtest/gtest.h"

#include "Log.h"

TEST(Log, Logging)
{
	Log::init();

    std::cout << Log::getCurrentLogFilePath() << std::endl;
    std::cout << std::filesystem::exists(Log::getCurrentLogFilePath()) << std::endl;

    Log::info("Information message on line {}", __LINE__);

    Log::warning("Warning message on line {}", __LINE__);

    Log::error("Error message on line {}", __LINE__);

    Log::fatalError("Fatal error message on line {}", __LINE__);

    std::ifstream in(Log::getCurrentLogFilePath());
    std::string tem;

    std::getline(in, tem);

    ASSERT_TRUE(tem.find("INFO") != std::string::npos);
    ASSERT_TRUE(tem.find("Information message on line 15") != std::string::npos);

    std::getline(in, tem);

    ASSERT_TRUE(tem.find("WARNING") != std::string::npos);
    ASSERT_TRUE(tem.find("Warning message on line 17") != std::string::npos);

    std::getline(in, tem);

    ASSERT_TRUE(tem.find("ERROR") != std::string::npos);
    ASSERT_TRUE(tem.find("Error message on line 19") != std::string::npos);

    std::getline(in, tem);

    ASSERT_TRUE(tem.find("FATAL_ERROR") != std::string::npos);
    ASSERT_TRUE(tem.find("Fatal error message on line 21") != std::string::npos);
}

int main(int argc, char** argv)
{
	testing::InitGoogleTest(&argc, argv);

	return RUN_ALL_TESTS();
}
