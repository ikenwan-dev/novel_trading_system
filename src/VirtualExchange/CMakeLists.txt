add_library(VirtualExchange INTERFACE)
target_link_libraries(VirtualExchange INTERFACE MboEvents NetworkSimulator LimitOrderBook databento::databento Performance)
target_include_directories(VirtualExchange INTERFACE ${CMAKE_CURRENT_SOURCE_DIR}/..)