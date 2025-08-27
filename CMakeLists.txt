cmake_minimum_required(VERSION 3.16)
project(AutomatedMechatronicTestSystem VERSION 1.0.0 LANGUAGES CXX C)

# Set C++ standard
set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

# Set build type
if(NOT CMAKE_BUILD_TYPE)
    set(CMAKE_BUILD_TYPE Release)
endif()

# Compiler flags
set(CMAKE_CXX_FLAGS "-Wall -Wextra")
set(CMAKE_CXX_FLAGS_DEBUG "-g")
set(CMAKE_CXX_FLAGS_RELEASE "-O3")

# Include directories
include_directories(include)

# Find required packages
find_package(Threads REQUIRED)

# Optional packages for computer vision
find_package(OpenCV QUIET)
if(OpenCV_FOUND)
    add_definitions(-DHAS_OPENCV)
    message(STATUS "OpenCV found - Computer vision features enabled")
else()
    message(STATUS "OpenCV not found - Computer vision features disabled")
endif()

# Source files
file(GLOB_RECURSE SOURCES "src/cpp/*.cpp" "src/cpp/*.c")
file(GLOB_RECURSE HEADERS "include/*.h" "include/*.hpp")

# Create main library
add_library(mechatronic_test_lib STATIC ${SOURCES} ${HEADERS})

# Link libraries
target_link_libraries(mechatronic_test_lib Threads::Threads)
if(OpenCV_FOUND)
    target_link_libraries(mechatronic_test_lib ${OpenCV_LIBS})
endif()

# Create main executable
add_executable(mechatronic_test_system src/cpp/main.cpp)
target_link_libraries(mechatronic_test_system mechatronic_test_lib)

# Testing
enable_testing()
add_subdirectory(tests)

# Installation
install(TARGETS mechatronic_test_system DESTINATION bin)
install(TARGETS mechatronic_test_lib DESTINATION lib)
install(DIRECTORY include/ DESTINATION include)

# Documentation
find_package(Doxygen)
if(DOXYGEN_FOUND)
    set(DOXYGEN_IN ${CMAKE_CURRENT_SOURCE_DIR}/docs/Doxyfile.in)
    set(DOXYGEN_OUT ${CMAKE_CURRENT_BINARY_DIR}/Doxyfile)
    
    configure_file(${DOXYGEN_IN} ${DOXYGEN_OUT} @ONLY)
    
    add_custom_target(doc_doxygen ALL
        COMMAND ${DOXYGEN_EXECUTABLE} ${DOXYGEN_OUT}
        WORKING_DIRECTORY ${CMAKE_CURRENT_BINARY_DIR}
        COMMENT "Generating API documentation with Doxygen"
        VERBATIM)
endif()