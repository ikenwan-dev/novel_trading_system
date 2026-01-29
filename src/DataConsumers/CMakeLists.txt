add_library(DataConsumer DataBentoConsumer/DataBentoConsumer.cpp)
target_link_libraries(DataConsumer PUBLIC databento::databento SharedMemory Constants)
target_include_directories(DataConsumer PUBLIC ${CMAKE_CURRENT_SOURCE_DIR}/..)