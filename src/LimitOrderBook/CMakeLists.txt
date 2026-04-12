add_library(LimitOrderBook 
    DataBentoLOB/DataBentoLOB.cpp
    DataBentoLOB/OptimizedDataBentoLOB.cpp
)
target_link_libraries(LimitOrderBook PUBLIC databento::databento)
target_include_directories(LimitOrderBook PUBLIC ${CMAKE_CURRENT_SOURCE_DIR}/..)