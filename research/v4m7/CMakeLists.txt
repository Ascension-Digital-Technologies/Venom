cmake_minimum_required(VERSION 3.20)
project(venom_v4m7_research LANGUAGES CXX)
set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)
enable_testing()

add_executable(venom_v4m7_tests
  tests.cpp ssa.cpp ../v4m5/frontend.cpp ../v4m6/cfg.cpp
  ../v4m3/vm.cpp ../../src/package/hash.cpp)
target_include_directories(venom_v4m7_tests PRIVATE ../../src)
if(MSVC)
  target_compile_options(venom_v4m7_tests PRIVATE /W4 /WX)
else()
  target_compile_options(venom_v4m7_tests PRIVATE -Wall -Wextra -Wpedantic -Werror)
endif()
add_test(NAME venom_v4m7_ssa_frontend_bridge COMMAND venom_v4m7_tests)
