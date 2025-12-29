add_library(RiskManager RiskManager.cpp)
target_link_libraries(RiskManager PRIVATE Events Portfolio)
target_include_directories(RiskManager PUBLIC ${CMAKE_CURRENT_SOURCE_DIR}/..)