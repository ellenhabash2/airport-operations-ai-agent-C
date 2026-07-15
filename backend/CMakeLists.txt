cmake_minimum_required(VERSION 3.16)
project(aeromind_backend CXX)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -std=c++20")

include(FetchContent)

# This service does not read Drogon YAML config files. Leaving Drogon's
# optional YAML support enabled can make local builds fail when only the
# yaml-cpp runtime library is installed without development headers.
set(BUILD_YAML_CONFIG OFF CACHE BOOL "Build Drogon YAML config support" FORCE)

# Find or fetch Drogon. When a previous local dependency checkout exists, prefer
# it before attempting a network fetch so offline development environments work.
find_package(Drogon CONFIG QUIET)
if(NOT Drogon_FOUND)
    if(EXISTS "${CMAKE_SOURCE_DIR}/build/_deps/drogon-src/CMakeLists.txt")
        FetchContent_Declare(drogon SOURCE_DIR "${CMAKE_SOURCE_DIR}/build/_deps/drogon-src")
    else()
        FetchContent_Declare(
            drogon
            GIT_REPOSITORY https://github.com/drogonframework/drogon.git
            GIT_TAG v1.9.13
        )
    endif()
    FetchContent_MakeAvailable(drogon)
endif()

# Find or fetch jsoncpp.
find_package(jsoncpp CONFIG QUIET)
if(NOT jsoncpp_FOUND)
    if(EXISTS "${CMAKE_SOURCE_DIR}/build/_deps/jsoncpp-src/CMakeLists.txt")
        FetchContent_Declare(jsoncpp SOURCE_DIR "${CMAKE_SOURCE_DIR}/build/_deps/jsoncpp-src")
    else()
        FetchContent_Declare(
            jsoncpp
            GIT_REPOSITORY https://github.com/open-source-parsers/jsoncpp.git
            GIT_TAG 1.9.5
        )
    endif()
    FetchContent_MakeAvailable(jsoncpp)
endif()

find_package(PostgreSQL REQUIRED)
find_package(PkgConfig REQUIRED)

# Find or fetch libpqxx.
find_package(pqxx CONFIG QUIET)
if(NOT pqxx_FOUND)
    pkg_check_modules(PQXX QUIET IMPORTED_TARGET libpqxx)
    if(NOT PQXX_FOUND)
        if(EXISTS "${CMAKE_SOURCE_DIR}/build/_deps/pqxx-src/CMakeLists.txt")
            FetchContent_Declare(pqxx SOURCE_DIR "${CMAKE_SOURCE_DIR}/build/_deps/pqxx-src")
        else()
            FetchContent_Declare(
                pqxx
                GIT_REPOSITORY https://github.com/jtv/libpqxx.git
                GIT_TAG 7.7.3
            )
        endif()
        FetchContent_MakeAvailable(pqxx)
    endif()
endif()

# Source files
set(AEROMIND_SOURCES
    main.cpp
    controllers/health_controller.cpp
    controllers/flight_controller.cpp
    controllers/gate_controller.cpp
    controllers/runway_controller.cpp
    controllers/incident_controller.cpp
    controllers/weather_controller.cpp
    controllers/auth_controller.cpp
    controllers/agent_controller.cpp
    database/database_manager.cpp
    repositories/flight_repository.cpp
    repositories/gate_repository.cpp
    repositories/runway_repository.cpp
    repositories/incident_repository.cpp
    repositories/weather_repository.cpp
    controllers/tools_test_controller.cpp
    tools/tools.cpp
    agent/GeminiClient.cpp
    controllers/gemini_test_controller.cpp
    utils/json_utils.cpp
)

# Create executable
add_executable(aeromind_backend ${AEROMIND_SOURCES})

# Include directories
target_include_directories(aeromind_backend PRIVATE
    ${CMAKE_CURRENT_SOURCE_DIR}
    ${PostgreSQL_INCLUDE_DIR}
)

# Link libraries
target_link_libraries(aeromind_backend PRIVATE
    Drogon::Drogon
    ${PostgreSQL_LIBRARIES}
)

if(TARGET Jsoncpp_lib)
    target_link_libraries(aeromind_backend PRIVATE Jsoncpp_lib)
elseif(TARGET JsonCpp::JsonCpp)
    target_link_libraries(aeromind_backend PRIVATE JsonCpp::JsonCpp)
elseif(TARGET jsoncpp_lib)
    target_link_libraries(aeromind_backend PRIVATE jsoncpp_lib)
else()
    target_link_libraries(aeromind_backend PRIVATE jsoncpp)
endif()

if(TARGET pqxx::pqxx)
    target_link_libraries(aeromind_backend PRIVATE pqxx::pqxx)
else()
    target_link_libraries(aeromind_backend PRIVATE PkgConfig::PQXX)
endif()

# Add optimization flags
if(CMAKE_BUILD_TYPE MATCHES Release)
    target_compile_options(aeromind_backend PRIVATE -O3)
else()
    target_compile_options(aeromind_backend PRIVATE -g)
endif()

# Install target for staging and packaging
install(TARGETS aeromind_backend
    RUNTIME DESTINATION bin
)


