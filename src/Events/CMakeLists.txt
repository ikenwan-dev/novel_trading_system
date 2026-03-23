add_library(BarEvents INTERFACE)
target_include_directories(BarEvents INTERFACE ${CMAKE_CURRENT_SOURCE_DIR}/../..)
target_link_libraries(BarEvents INTERFACE databento::databento)

add_library(MboEvents INTERFACE)
target_include_directories(MboEvents INTERFACE ${CMAKE_CURRENT_SOURCE_DIR}/../..)
target_link_libraries(MboEvents INTERFACE databento::databento)
