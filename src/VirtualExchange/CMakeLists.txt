add_library(VirtualExchange VirtualExchange.cpp)
target_link_libraries(VirtualExchange PUBLIC Events NetworkSimulator LimitOrderBook databento::databento)
target_include_directories(VirtualExchange PUBLIC ${CMAKE_CURRENT_SOURCE_DIR}/..)