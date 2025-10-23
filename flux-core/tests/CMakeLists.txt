# Flux-Core Tests
cmake_minimum_required(VERSION 3.22)

# Find Google Test
find_package(GTest REQUIRED)
find_package(Threads REQUIRED)

# Create test executable
add_executable(flux-core-tests
    test_archive_utils.cpp
)

# Link libraries
target_link_libraries(flux-core-tests
    PRIVATE
    flux-core
    GTest::gtest
    GTest::gtest_main
    Threads::Threads
)

# Set C++ standard
set_target_properties(flux-core-tests PROPERTIES
    CXX_STANDARD 23
    CXX_STANDARD_REQUIRED ON
    CXX_EXTENSIONS OFF
)

# Include directories
target_include_directories(flux-core-tests PRIVATE
    ${CMAKE_CURRENT_SOURCE_DIR}/../include
)

# Add test to CTest
include(GoogleTest)
gtest_discover_tests(flux-core-tests)

# Custom target to run tests
add_custom_target(run-tests
    COMMAND flux-core-tests
    DEPENDS flux-core-tests
    WORKING_DIRECTORY ${CMAKE_CURRENT_BINARY_DIR}
    COMMENT "Running flux-core unit tests"
)
