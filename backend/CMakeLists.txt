cmake_minimum_required(VERSION 3.16)
project(aeromind_backend CXX)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -std=c++20")

include(FetchContent)

set(BUILD_YAML_CONFIG OFF CACHE BOOL "Build Drogon YAML config support" FORCE)

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

# jwt-cpp (header-only) for authentication tokens.
set(JWT_BUILD_EXAMPLES OFF CACHE BOOL "" FORCE)
find_package(jwt-cpp CONFIG QUIET)
if(NOT jwt-cpp_FOUND)
    FetchContent_Declare(
        jwt-cpp
        GIT_REPOSITORY https://github.com/Thalhammer/jwt-cpp.git
        GIT_TAG 08bcf77a687fb06e34138e9e9fa12a4ecbe12332
    )
    FetchContent_MakeAvailable(jwt-cpp)
endif()

# Bcrypt.cpp for password hashing
FetchContent_Declare(
    bcrypt
    GIT_REPOSITORY https://github.com/hilch/Bcrypt.cpp.git
    GIT_TAG 0d18b6a99e8c57627910db4ef9a7706c009b12ad
)
FetchContent_MakeAvailable(bcrypt)

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
    repositories/user_repository.cpp
    tools/tools.cpp
    agent/GeminiClient.cpp
    agent/ToolRegistry.cpp
    security/PasswordHasher.cpp
    security/JwtService.cpp
    utils/json_utils.cpp
)

add_executable(aeromind_backend ${AEROMIND_SOURCES})

target_include_directories(aeromind_backend PRIVATE
    ${CMAKE_CURRENT_SOURCE_DIR}
    ${PostgreSQL_INCLUDE_DIR}
)

target_link_libraries(aeromind_backend PRIVATE
    Drogon::Drogon
    ${PostgreSQL_LIBRARIES}
    jwt-cpp::jwt-cpp
    bcrypt
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

if(CMAKE_BUILD_TYPE MATCHES Release)
    target_compile_options(aeromind_backend PRIVATE -O3)
else()
    target_compile_options(aeromind_backend PRIVATE -g)
endif()

install(TARGETS aeromind_backend
    RUNTIME DESTINATION bin
)
