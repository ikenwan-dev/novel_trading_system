cmake_minimum_required(VERSION 3.15)
project(NovelTradingSystem)
set(CMAKE_CXX_STANDARD 23)
# set(CMAKE_BUILD_TYPE RelWithDebInfo)
set(CMAKE_BUILD_TYPE Release)

set(CMAKE_CXX_STANDARD_REQUIRED ON)

add_subdirectory(third_party)
add_subdirectory(src)

add_executable(MainEventLoop src/main.cpp)

target_link_libraries(MainEventLoop PRIVATE 
    Strategies
    Statistics
    ExecutionHandlers
    RiskManager
    Portfolio
    DataHandlers 
    Events 
    ThreadSafeQueue
    glaze::glaze
)

add_executable(run_tests tests/main_test.cpp tests/portfolio_test.cpp)
target_link_libraries(run_tests PRIVATE gtest_main) 

target_compile_definitions(MainEventLoop PRIVATE PROJECT_ROOT="${CMAKE_CURRENT_SOURCE_DIR}")
target_compile_definitions(run_tests PRIVATE PROJECT_ROOT="${CMAKE_CURRENT_SOURCE_DIR}")
enable_testing()
add_test(NAME unit_tests COMMAND run_tests)