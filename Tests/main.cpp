#include "gtest/gtest.h"

#include "Log.h"

TEST(Log, Logging)
{
	Log::init();

    std::cout << __LINE__ << std::endl;

    Log::info("Information message on line {}", __LINE__);

    std::cout << __LINE__ << std::endl;

    Log::warning("Warning message on line {}", __LINE__);

    std::cout << __LINE__ << std::endl;

    Log::error("Error message on line {}", __LINE__);

    std::cout << __LINE__ << std::endl;

    Log::fatalError("Fatal error message on line {}", __LINE__);

    std::cout << __LINE__ << std::endl;

    std::ifstream in(Log::getCurrentLogFilePath());
    std::ostringstream os;

    std::cout << __LINE__ << std::endl;

    os << in.rdbuf();

    std::cout << __LINE__ << std::endl;

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

int main(int argc, char** argv)
{
	testing::InitGoogleTest(&argc, argv);

	return RUN_ALL_TESTS();
}
