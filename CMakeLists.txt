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

add_executable(producer src/producer.cpp)
target_link_libraries(producer PRIVATE DataProducer)

add_executable(LobScript src/consumer.cpp)
target_link_libraries(LobScript PRIVATE DataConsumer MboEvents NetworkSimulator)

add_executable(MainMboEventLoop src/MBOMain.cpp)
target_link_libraries(MainMboEventLoop PRIVATE DataConsumer MboEvents NetworkSimulator MBOStrategies LimitOrderBook OrderManagementSystem VirtualExchange RiskManager MarketMaker Performance)

add_executable(MainBarEventLoop src/main.cpp)

target_link_libraries(MainBarEventLoop PRIVATE 
    BarStrategies
    Statistics
    ExecutionHandlers
    RiskManager
    Portfolio
    DataHandlers 
    BarEvents 
    ThreadSafeQueue
    glaze::glaze
)
target_compile_definitions(MainBarEventLoop PRIVATE PROJECT_ROOT="${CMAKE_CURRENT_SOURCE_DIR}")
target_compile_definitions(producer PRIVATE PROJECT_ROOT="${CMAKE_CURRENT_SOURCE_DIR}")
target_compile_definitions(MainMboEventLoop PRIVATE PROJECT_ROOT="${CMAKE_CURRENT_SOURCE_DIR}")