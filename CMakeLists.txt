cmake_minimum_required(VERSION 3.15)
project(NovelTradingSystem)
set(CMAKE_CXX_STANDARD 23)
if(NOT CMAKE_BUILD_TYPE)
  set(CMAKE_BUILD_TYPE Release)
endif()

set(CMAKE_CXX_STANDARD_REQUIRED ON)

enable_testing()

add_subdirectory(third_party)
add_subdirectory(src)
add_subdirectory(tests)

add_executable(example src/example.cpp)
target_link_libraries(example PRIVATE databento::databento)
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
target_compile_definitions(example PRIVATE PROJECT_ROOT="${CMAKE_CURRENT_SOURCE_DIR}")