add_library(Strategies
    MovingAverageCrossover/MovingAverageCrossover.cpp
    StrategyConfig.h
)

target_link_libraries(Strategies PRIVATE Events)
target_include_directories(Strategies PUBLIC ${CMAKE_CURRENT_SOURCE_DIR}/..)
