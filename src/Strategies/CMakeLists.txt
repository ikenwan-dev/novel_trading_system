add_library(BarStrategies
    BarStrategies/MovingAverageCrossover/MovingAverageCrossover.cpp
    BarStrategies/StrategyConfig.h
)

target_link_libraries(BarStrategies PRIVATE BarEvents)
target_include_directories(BarStrategies PUBLIC ${CMAKE_CURRENT_SOURCE_DIR}/..)

add_library(MBOStrategies INTERFACE
    MBOStrategies/MBOStrategyBase.h)

target_link_libraries(MBOStrategies INTERFACE LimitOrderBook databento::databento OrderManagementSystem Performance)

add_library(MarketMaker MBOStrategies/MarketMaker/MarketMaker.cpp)
target_link_libraries(MarketMaker PUBLIC MBOStrategies)
target_include_directories(MarketMaker PUBLIC ${CMAKE_CURRENT_SOURCE_DIR}/..)
