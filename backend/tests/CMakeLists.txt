include(FetchContent)

find_package(GTest CONFIG QUIET)
if(NOT GTest_FOUND)
    FetchContent_Declare(
        googletest
        GIT_REPOSITORY https://github.com/google/googletest.git
        GIT_TAG v1.15.2
    )
    set(INSTALL_GTEST OFF CACHE BOOL "" FORCE)
    FetchContent_MakeAvailable(googletest)
else()
    # The preinstalled Conda GTest package exports its whole include prefix.
    # That prefix can contain an unrelated pqxx version and shadow the system
    # pqxx headers used by the application. GTest itself is available through
    # the normal compiler include path, so keep the imported target narrow.
    set_property(TARGET GTest::gtest PROPERTY INTERFACE_INCLUDE_DIRECTORIES "")
    set_property(TARGET GTest::gtest_main PROPERTY INTERFACE_INCLUDE_DIRECTORIES "")
    get_filename_component(_gtest_prefix "${GTest_DIR}/../../.." ABSOLUTE)
    file(COPY "${_gtest_prefix}/include/gtest" DESTINATION "${CMAKE_CURRENT_BINARY_DIR}/gtest_include")
endif()

add_executable(aeromind_tests
    security/PasswordHasherTest.cpp
    security/JwtServiceTest.cpp
    tools/ToolRegistryTest.cpp
    fakes/FakeTools.cpp
    agent/AgentLoopTest.cpp
    agent/LLMConfigTest.cpp
    agent/LLMClientTest.cpp
    services/ServiceTest.cpp
)

target_include_directories(aeromind_tests PRIVATE ${CMAKE_CURRENT_SOURCE_DIR}/..)
if(GTest_FOUND)
    target_include_directories(aeromind_tests PRIVATE "${CMAKE_CURRENT_BINARY_DIR}/gtest_include")
endif()
target_sources(aeromind_tests PRIVATE
    ../security/PasswordHasher.cpp
    ../security/JwtService.cpp
    ../agent/ToolRegistry.cpp
    ../agent/AgentLoop.cpp
    ../agent/LLMConfig.cpp
    ../agent/LLMClient.cpp
    ../services/flight_service.cpp
    ../services/gate_service.cpp
    ../services/terminal_service.cpp
    ../services/runway_service.cpp
    ../services/incident_service.cpp
    ../services/weather_service.cpp
    ../services/conversation_service.cpp
    ../services/agent_service.cpp
    ../repositories/flight_repository.cpp
    ../repositories/gate_repository.cpp
    ../repositories/terminal_repository.cpp
    ../repositories/runway_repository.cpp
    ../repositories/incident_repository.cpp
    ../repositories/weather_repository.cpp
    ../repositories/conversation_repository.cpp
    ../database/database_manager.cpp
)
target_link_libraries(aeromind_tests PRIVATE
    GTest::gtest_main Drogon::Drogon
    jwt-cpp::jwt-cpp bcrypt crypt
    pqxx ${PostgreSQL_LIBRARIES}
)
if(TARGET Jsoncpp_lib)
    target_link_libraries(aeromind_tests PRIVATE Jsoncpp_lib)
elseif(TARGET JsonCpp::JsonCpp)
    target_link_libraries(aeromind_tests PRIVATE JsonCpp::JsonCpp)
elseif(TARGET jsoncpp_lib)
    target_link_libraries(aeromind_tests PRIVATE jsoncpp_lib)
else()
    target_link_libraries(aeromind_tests PRIVATE jsoncpp)
endif()
include(GoogleTest)
gtest_discover_tests(aeromind_tests)
