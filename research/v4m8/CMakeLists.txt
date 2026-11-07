cmake_minimum_required(VERSION 3.20)
project(venom_v4m8_research LANGUAGES CXX)
set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)
enable_testing()

add_executable(venom_v4m8_js
  cli.cpp typed.cpp ../v4m3/vm.cpp ../../src/package/hash.cpp)
add_executable(venom_v4m8_tests
  tests.cpp typed.cpp
  ../v4m5/frontend.cpp ../v4m7/ssa.cpp ../v4m6/cfg.cpp
  ../v4m3/vm.cpp ../../src/package/hash.cpp)
foreach(target venom_v4m8_js venom_v4m8_tests)
  target_include_directories(${target} PRIVATE ../../src)
  if(MSVC)
    target_compile_options(${target} PRIVATE /W4 /WX)
  else()
    target_compile_options(${target} PRIVATE -Wall -Wextra -Wpedantic -Werror)
  endif()
endforeach()
add_test(NAME venom_v4m8_source_direct_typed_ssa COMMAND venom_v4m8_tests)
