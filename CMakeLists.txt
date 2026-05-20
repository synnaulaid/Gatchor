cmake_minimum_required(VERSION 3.16)

project(Gatchor VERSION 0.1)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

add_compile_options(
    -O3
    -march=native
    -Wall
    -Wextra
)

include_directories(include)

add_library(gatchor
    src/gatchor.cpp
)

add_executable(main
    main.cpp
)
target_link_libraries(main gatchor)

add_executable(test_gatchor
    tests/test.cpp
)
target_link_libraries(test_gatchor gatchor)

add_executable(interactive 
    interactive.cpp
)
target_link_libraries(interactive gatchor)

add_executable(test_security tests/test_security.cpp)
target_link_libraries(test_security gatchor)

add_executable(test_benchmark tests/test_benchmark.cpp)
target_link_libraries(test_benchmark gatchor)

enable_testing()
add_test(NAME SecurityTest COMMAND test_security)
add_test(NAME BenchmarkTest COMMAND test_benchmark)