add_library(NetworkSimulator INTERFACE)
target_include_directories(NetworkSimulator INTERFACE ${CMAKE_CURRENT_SOURCE_DIR}/..)
target_link_libraries(NetworkSimulator INTERFACE Events)