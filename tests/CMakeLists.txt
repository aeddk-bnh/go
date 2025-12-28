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

add_executable(test_superko test_superko.cpp)
target_link_libraries(test_superko gtest_main gogame)
add_test(NAME SuperkoTest COMMAND test_superko)

add_executable(test_scoring test_scoring.cpp)
target_link_libraries(test_scoring gtest_main gogame)
add_test(NAME ScoringTest COMMAND test_scoring)

add_executable(test_sgf test_sgf.cpp)
target_link_libraries(test_sgf gtest_main gogame)
add_test(NAME SGFTest COMMAND test_sgf)

add_executable(test_sgf_extra test_sgf_extra.cpp)
target_link_libraries(test_sgf_extra gtest_main gogame)
add_test(NAME SGFExtraTest COMMAND test_sgf_extra)

add_executable(test_sgf_meta test_sgf_meta.cpp)
target_link_libraries(test_sgf_meta gtest_main gogame)
add_test(NAME SGFMetaTest COMMAND test_sgf_meta)
