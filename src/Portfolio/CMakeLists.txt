add_library(Portfolio Portfolio.cpp)

target_link_libraries(Portfolio PUBLIC Events DataHandlers)
target_include_directories(Portfolio PUBLIC ${CMAKE_CURRENT_SOURCE_DIR}/..)