add_library(Events INTERFACE)
target_include_directories(Events INTERFACE ${CMAKE_CURRENT_SOURCE_DIR}/..)
target_link_libraries(Events INTERFACE databento::databento)
