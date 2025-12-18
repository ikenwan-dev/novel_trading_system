cmake_minimum_required(VERSION 3.15)
project(NovelTradingSystem)
set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

add_subdirectory(third_party)
add_subdirectory(src)


add_executable(MainEventLoop src/main.cpp)

target_link_libraries(MainEventLoop PRIVATE 
    DataHandlers 
    Strategies 
    Events 
    ThreadSafeQueue
)