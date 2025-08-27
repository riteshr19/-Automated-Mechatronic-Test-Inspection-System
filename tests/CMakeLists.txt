cmake_minimum_required(VERSION 3.16)

# Test configurations
enable_testing()

# Find GTest
find_package(GTest QUIET)

if(GTest_FOUND)
    message(STATUS "Google Test found - Unit tests enabled")
    
    # Unit tests
    file(GLOB_RECURSE TEST_SOURCES "unit/*.cpp")
    
    foreach(TEST_SOURCE ${TEST_SOURCES})
        get_filename_component(TEST_NAME ${TEST_SOURCE} NAME_WE)
        add_executable(${TEST_NAME} ${TEST_SOURCE})
        target_link_libraries(${TEST_NAME} 
            mechatronic_test_lib 
            GTest::GTest 
            GTest::Main
        )
        add_test(NAME ${TEST_NAME} COMMAND ${TEST_NAME})
    endforeach()
    
else()
    message(STATUS "Google Test not found - Creating simple test framework")
    
    # Simple test without GTest
    add_executable(simple_test unit/simple_test.cpp)
    target_link_libraries(simple_test mechatronic_test_lib)
    add_test(NAME simple_test COMMAND simple_test)
endif()

# Integration tests (always available)
file(GLOB_RECURSE INTEGRATION_TEST_SOURCES "integration/*.cpp")

foreach(TEST_SOURCE ${INTEGRATION_TEST_SOURCES})
    get_filename_component(TEST_NAME ${TEST_SOURCE} NAME_WE)
    add_executable(${TEST_NAME} ${TEST_SOURCE})
    target_link_libraries(${TEST_NAME} mechatronic_test_lib)
    add_test(NAME ${TEST_NAME} COMMAND ${TEST_NAME})
endforeach()