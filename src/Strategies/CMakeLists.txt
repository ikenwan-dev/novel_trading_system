add_library(BarStrategies
    MovingAverageCrossover/MovingAverageCrossover.cpp
    StrategyConfig.h
)

target_link_libraries(BarStrategies PRIVATE Events)
target_include_directories(BarStrategies PUBLIC ${CMAKE_CURRENT_SOURCE_DIR}/..)
