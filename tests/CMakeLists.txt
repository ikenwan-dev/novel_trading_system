add_executable(run_tests main_test.cpp portfolio_test.cpp strategy_test.cpp)
target_link_libraries(run_tests PRIVATE gtest_main Portfolio Strategies)
target_include_directories(run_tests PUBLIC ${CMAKE_CURRENT_SOURCE_DIR}/../src)

add_test(NAME unit_tests COMMAND run_tests)