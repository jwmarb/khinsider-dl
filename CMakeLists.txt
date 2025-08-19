cmake_minimum_required(VERSION 4.0.0)
project(khinsiderdl)

set(CMAKE_CXX_STANDARD 23)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

find_package(CURL CONFIG REQUIRED)
find_package(unofficial-gumbo CONFIG REQUIRED)
find_package(GTest CONFIG REQUIRED)
add_executable(khinsiderdl src/main.cpp src/url.cpp)
add_executable(tests src/tests/test_main.cpp src/tests/test_url.cpp src/url.cpp)

target_link_libraries(khinsiderdl PRIVATE CURL::libcurl)
target_link_libraries(khinsiderdl PRIVATE unofficial::gumbo::gumbo)
target_link_libraries(tests PRIVATE GTest::gtest GTest::gtest_main)

enable_testing()
add_test(NAME tests COMMAND tests)