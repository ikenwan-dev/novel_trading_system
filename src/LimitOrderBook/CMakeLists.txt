add_library(DataBentoLOB DataBentoLOB/DataBentoLOB.cpp)
target_link_libraries(DataBentoLOB PUBLIC databento::databento)
target_include_directories(DataBentoLOB PUBLIC ${CMAKE_CURRENT_SOURCE_DIR}/..)