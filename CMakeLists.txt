cmake_minimum_required(VERSION 3.16)
project(AeroMind)

option(AEROMIND_BUILD_TESTS "Build AeroMind automated tests" OFF)
if(AEROMIND_BUILD_TESTS)
    include(CTest)
    enable_testing()
endif()

# Add backend subdirectory
add_subdirectory(backend)
