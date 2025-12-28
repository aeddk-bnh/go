# CMake generated Testfile for 
# Source directory: C:/Users/ASUS/Documents/go/tests
# Build directory: C:/Users/ASUS/Documents/go/build-bench/tests
# 
# This file includes the relevant testing commands required for 
# testing this directory and lists subdirectories to be tested as well.
add_test(BoardTest "C:/Users/ASUS/Documents/go/build-bench/tests/test_board.exe")
set_tests_properties(BoardTest PROPERTIES  _BACKTRACE_TRIPLES "C:/Users/ASUS/Documents/go/tests/CMakeLists.txt;13;add_test;C:/Users/ASUS/Documents/go/tests/CMakeLists.txt;0;")
add_test(SuperkoTest "C:/Users/ASUS/Documents/go/build-bench/tests/test_superko.exe")
set_tests_properties(SuperkoTest PROPERTIES  _BACKTRACE_TRIPLES "C:/Users/ASUS/Documents/go/tests/CMakeLists.txt;17;add_test;C:/Users/ASUS/Documents/go/tests/CMakeLists.txt;0;")
add_test(ScoringTest "C:/Users/ASUS/Documents/go/build-bench/tests/test_scoring.exe")
set_tests_properties(ScoringTest PROPERTIES  _BACKTRACE_TRIPLES "C:/Users/ASUS/Documents/go/tests/CMakeLists.txt;21;add_test;C:/Users/ASUS/Documents/go/tests/CMakeLists.txt;0;")
add_test(SGFTest "C:/Users/ASUS/Documents/go/build-bench/tests/test_sgf.exe")
set_tests_properties(SGFTest PROPERTIES  _BACKTRACE_TRIPLES "C:/Users/ASUS/Documents/go/tests/CMakeLists.txt;25;add_test;C:/Users/ASUS/Documents/go/tests/CMakeLists.txt;0;")
add_test(SGFExtraTest "C:/Users/ASUS/Documents/go/build-bench/tests/test_sgf_extra.exe")
set_tests_properties(SGFExtraTest PROPERTIES  _BACKTRACE_TRIPLES "C:/Users/ASUS/Documents/go/tests/CMakeLists.txt;29;add_test;C:/Users/ASUS/Documents/go/tests/CMakeLists.txt;0;")
add_test(SGFMetaTest "C:/Users/ASUS/Documents/go/build-bench/tests/test_sgf_meta.exe")
set_tests_properties(SGFMetaTest PROPERTIES  _BACKTRACE_TRIPLES "C:/Users/ASUS/Documents/go/tests/CMakeLists.txt;33;add_test;C:/Users/ASUS/Documents/go/tests/CMakeLists.txt;0;")
add_test(SGFEscapeTest "C:/Users/ASUS/Documents/go/build-bench/tests/test_sgf_escape.exe")
set_tests_properties(SGFEscapeTest PROPERTIES  _BACKTRACE_TRIPLES "C:/Users/ASUS/Documents/go/tests/CMakeLists.txt;37;add_test;C:/Users/ASUS/Documents/go/tests/CMakeLists.txt;0;")
add_test(MCTSTest "C:/Users/ASUS/Documents/go/build-bench/tests/test_mcts.exe")
set_tests_properties(MCTSTest PROPERTIES  _BACKTRACE_TRIPLES "C:/Users/ASUS/Documents/go/tests/CMakeLists.txt;41;add_test;C:/Users/ASUS/Documents/go/tests/CMakeLists.txt;0;")
add_test(MCTSThreadedTest "C:/Users/ASUS/Documents/go/build-bench/tests/test_mcts_threaded.exe")
set_tests_properties(MCTSThreadedTest PROPERTIES  _BACKTRACE_TRIPLES "C:/Users/ASUS/Documents/go/tests/CMakeLists.txt;45;add_test;C:/Users/ASUS/Documents/go/tests/CMakeLists.txt;0;")
add_test(MCTSStressTest "C:/Users/ASUS/Documents/go/build-bench/tests/test_mcts_stress.exe")
set_tests_properties(MCTSStressTest PROPERTIES  _BACKTRACE_TRIPLES "C:/Users/ASUS/Documents/go/tests/CMakeLists.txt;49;add_test;C:/Users/ASUS/Documents/go/tests/CMakeLists.txt;0;")
subdirs("../_deps/googletest-build")
