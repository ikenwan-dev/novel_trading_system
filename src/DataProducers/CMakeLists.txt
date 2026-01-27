add_library(DataProducer DataBentoProducer/DataBentoProducer.cpp)
target_link_libraries(DataProducer PUBLIC databento::databento SharedMemory Constants)
target_include_directories(DataProducer PUBLIC ${CMAKE_CURRENT_SOURCE_DIR}/..)
