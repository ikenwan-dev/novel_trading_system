add_library(fast_cpp_csv_parser INTERFACE)
target_include_directories(fast_cpp_csv_parser INTERFACE ${CMAKE_CURRENT_SOURCE_DIR}/fast_cpp_csv_parser)


include(FetchContent)
FetchContent_Declare(
  glaze
  GIT_REPOSITORY https://github.com/stephenberry/glaze.git
  GIT_TAG v6.4.0 # Use the latest version tag
)
FetchContent_MakeAvailable(glaze)

