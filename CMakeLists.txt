cmake_minimum_required(VERSION 4.0.0)
project(khinsiderdl)

set(CMAKE_CXX_STANDARD 23)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

find_package(CURL CONFIG REQUIRED)
find_package(unofficial-gumbo CONFIG REQUIRED)
find_package(GTest CONFIG REQUIRED)
find_package(cxxopts CONFIG REQUIRED)
find_package(fmt CONFIG REQUIRED)

include_directories(${CMAKE_CURRENT_SOURCE_DIR}/barkeep)

add_executable(khinsiderdl src/main.cpp src/url.cpp src/curl_handler.cpp)
add_executable(tests src/tests/test_main.cpp src/tests/test_url.cpp src/url.cpp src/curl_handler.cpp)

target_link_libraries(khinsiderdl PRIVATE CURL::libcurl)
target_link_libraries(khinsiderdl PRIVATE unofficial::gumbo::gumbo)
target_link_libraries(khinsiderdl PRIVATE cxxopts::cxxopts)
target_link_libraries(khinsiderdl PRIVATE fmt::fmt-header-only)
target_link_libraries(tests PRIVATE GTest::gtest GTest::gtest_main)

target_compile_definitions(khinsiderdl PRIVATE BARKEEP_ENABLE_FMT_FORMAT)

enable_testing()
add_test(NAME tests COMMAND tests)