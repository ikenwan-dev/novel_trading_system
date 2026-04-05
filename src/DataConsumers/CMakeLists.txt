add_library(DataConsumer 
    DataBentoIPCConsumer/DataBentoIPCConsumer.cpp
    DataBentoMappedConsumer/DataBentoMappedConsumer.cpp
)
target_link_libraries(DataConsumer PUBLIC databento::databento SharedMemory Constants LimitOrderBook)
target_include_directories(DataConsumer PUBLIC ${CMAKE_CURRENT_SOURCE_DIR}/..)