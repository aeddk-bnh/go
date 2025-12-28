enable_testing()

# GoogleTest
include(FetchContent)
FetchContent_Declare(
  googletest
  URL https://github.com/google/googletest/archive/refs/tags/release-1.12.1.zip
)
FetchContent_MakeAvailable(googletest)

add_executable(test_board test_board.cpp)
target_link_libraries(test_board gtest_main gogame)
add_test(NAME BoardTest COMMAND test_board)
