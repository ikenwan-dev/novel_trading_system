add_library(DataHandlers 
    HistoricDataHandler/HistoricCSVDataHandler.cpp
)

target_link_libraries(DataHandlers PUBLIC BarEvents PRIVATE fast_cpp_csv_parser)
target_include_directories(DataHandlers PUBLIC ${CMAKE_CURRENT_SOURCE_DIR}/..)
