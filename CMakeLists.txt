set(CMAKE_CXX_STANDARD 11)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

include(FetchContent)

project(resource_pool LANGUAGES CXX)

enable_testing()
FetchContent_Declare(googletest
    GIT_REPOSITORY https://github.com/google/googletest
)
FetchContent_MakeAvailable(googletest)
add_executable(resource_pool test.cpp)
target_link_libraries(resource_pool gtest gtest_main)
add_test(resource_pool resource_pool)