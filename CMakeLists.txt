cmake_minimum_required(VERSION 3.15)
project(NovelTradingSystem)
set(CMAKE_CXX_STANDARD 23)
# set(CMAKE_BUILD_TYPE RelWithDebInfo)
set(CMAKE_BUILD_TYPE Release)

set(CMAKE_CXX_STANDARD_REQUIRED ON)

add_subdirectory(third_party)
add_subdirectory(src)
add_subdirectory(tests)

 
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
target_compile_definitions(MainEventLoop PRIVATE PROJECT_ROOT="${CMAKE_CURRENT_SOURCE_DIR}")