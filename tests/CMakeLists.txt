enable_testing()

# Prefer Conan-provided GTest (or system package) and fall back to FetchContent
find_package(GTest CONFIG QUIET)
if(GTest_FOUND)
  set(GTEST_MAIN_TARGET GTest::gtest_main)
else()
  include(FetchContent)
  FetchContent_Declare(
    googletest
    URL https://github.com/google/googletest/archive/refs/tags/release-1.12.1.zip
  )
  FetchContent_MakeAvailable(googletest)
  set(GTEST_MAIN_TARGET gtest_main)
endif()
enable_testing()

# GoogleTest
include(FetchContent)
FetchContent_Declare(
  googletest
  URL https://github.com/google/googletest/archive/refs/tags/release-1.12.1.zip
)
FetchContent_MakeAvailable(googletest)

add_executable(test_board test_board.cpp)
target_link_libraries(test_board ${GTEST_MAIN_TARGET} gogame)
add_test(NAME BoardTest COMMAND test_board)

add_executable(test_superko test_superko.cpp)
target_link_libraries(test_superko ${GTEST_MAIN_TARGET} gogame)
add_test(NAME SuperkoTest COMMAND test_superko)

add_executable(test_scoring test_scoring.cpp)
target_link_libraries(test_scoring ${GTEST_MAIN_TARGET} gogame)
add_test(NAME ScoringTest COMMAND test_scoring)

add_executable(test_sgf test_sgf.cpp)
target_link_libraries(test_sgf ${GTEST_MAIN_TARGET} gogame)
add_test(NAME SGFTest COMMAND test_sgf)

add_executable(test_sgf_extra test_sgf_extra.cpp)
target_link_libraries(test_sgf_extra ${GTEST_MAIN_TARGET} gogame)
add_test(NAME SGFExtraTest COMMAND test_sgf_extra)

add_executable(test_sgf_meta test_sgf_meta.cpp)
target_link_libraries(test_sgf_meta ${GTEST_MAIN_TARGET} gogame)
add_test(NAME SGFMetaTest COMMAND test_sgf_meta)

add_executable(test_sgf_escape test_sgf_escape.cpp)
target_link_libraries(test_sgf_escape ${GTEST_MAIN_TARGET} gogame)
add_test(NAME SGFEscapeTest COMMAND test_sgf_escape)

add_executable(test_game test_game.cpp)
target_link_libraries(test_game ${GTEST_MAIN_TARGET} gogame)
add_test(NAME GameTest COMMAND test_game)

add_executable(test_mcts test_mcts.cpp)
target_link_libraries(test_mcts ${GTEST_MAIN_TARGET} gogame ai)
add_test(NAME MCTSTest COMMAND test_mcts)

add_executable(test_mcts_threaded test_mcts_threaded.cpp)
target_link_libraries(test_mcts_threaded ${GTEST_MAIN_TARGET} gogame ai)
add_test(NAME MCTSThreadedTest COMMAND test_mcts_threaded)

add_executable(test_mcts_stress test_mcts_stress.cpp)
target_link_libraries(test_mcts_stress ${GTEST_MAIN_TARGET} gogame ai)
add_test(NAME MCTSStressTest COMMAND test_mcts_stress)
