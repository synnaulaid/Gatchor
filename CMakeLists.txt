cmake_minimum_required(VERSION 3.16)

project(Gatchor VERSION 0.1)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

option(GATCHOR_NATIVE_OPTIMIZATIONS
    "Enable CPU-specific optimizations for the local mining host" OFF)

add_compile_options(-O3 -Wall -Wextra)

add_library(gatchor
    src/gatchor.cpp
)
target_include_directories(gatchor PUBLIC include)
if(GATCHOR_NATIVE_OPTIMIZATIONS)
    target_compile_options(gatchor PRIVATE -march=native)
endif()

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
add_test(NAME GatchorTest COMMAND test_gatchor)
add_test(NAME SecurityTest COMMAND test_security)
add_test(NAME BenchmarkTest COMMAND test_benchmark)