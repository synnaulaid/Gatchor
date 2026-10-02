cmake_minimum_required(VERSION 3.16)

project(Gatchor VERSION 0.1)

find_package(OpenSSL 1.1.1 REQUIRED COMPONENTS Crypto)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

option(GATCHOR_NATIVE_OPTIMIZATIONS
    "Enable CPU-specific optimizations for the local mining host" OFF)

add_compile_options(-O3 -Wall -Wextra)

add_library(gatchor
    src/gatchor.cpp
    src/gatchor_pow.cpp
    src/gatchor_chain.cpp
    src/gatchor_transaction.cpp
)
if(UNIX)
    target_sources(gatchor PRIVATE src/gatchor_network.cpp)
endif()
target_include_directories(gatchor PUBLIC include)
target_link_libraries(gatchor PUBLIC OpenSSL::Crypto)
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

add_executable(test_pow tests/test_pow.cpp)
target_link_libraries(test_pow gatchor)

add_executable(test_transaction tests/test_transaction.cpp)
target_link_libraries(test_transaction gatchor)

add_executable(test_chain tests/test_chain.cpp)
target_link_libraries(test_chain gatchor)

add_executable(test_nodes tests/test_nodes.cpp)
target_link_libraries(test_nodes gatchor)

if(UNIX)
    find_package(Threads REQUIRED)
    add_executable(test_network tests/test_network.cpp)
    target_link_libraries(test_network PRIVATE gatchor Threads::Threads)
endif()

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
add_test(NAME ProofOfWorkTest COMMAND test_pow)
add_test(NAME TransactionTest COMMAND test_transaction)
add_test(NAME ChainStoreTest COMMAND test_chain)
add_test(NAME TwoNodeSimulationTest COMMAND test_nodes)
if(UNIX)
    add_test(NAME LoopbackNetworkTest COMMAND test_network)
endif()
add_test(NAME SecurityTest COMMAND test_security)
add_test(NAME BenchmarkTest COMMAND test_benchmark)