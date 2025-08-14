cmake_minimum_required(VERSION 3.15)

set(PROJECT_NAME cpp_sqlite)

project(${PROJECT_NAME} VERSION 0.1.0 LANGUAGES C CXX)

# Set compiler options
set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

add_compile_options(-Wall -Wextra -Wpedantic)

add_library(${PROJECT_NAME})
set_target_properties(${PROJECT_NAME} PROPERTIES LINKER_LANGUAGE CXX)

target_link_libraries(${PROJECT_NAME} 
        PRIVATE
                SQLite::SQLite3)

# Add all relevant include directories
target_include_directories(${PROJECT_NAME} PUBLIC
$<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}>
$<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/src>
$<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/include>
)

# Find the packages (dependencies) provided by Conan
find_package(SQLite3 REQUIRED)

# Enable testing at the top level
enable_testing()

# IMPORTANT: Test framework now available before adding source directories
add_subdirectory(cpp_sqlite)

set(CPACK_PROJECT_NAME ${PROJECT_NAME})
set(CPACK_PROJECT_VERSION ${PROJECT_VERSION})
include(CPack)


# Installation rules
install(TARGETS ${PROJECT_NAME} DESTINATION "./installs"
        RUNTIME DESTINATION bin
        ARCHIVE DESTINATION lib
        LIBRARY DESTINATION lib
        )

# Install headers using the variable
install(FILES ${HEADER_FILES}
        DESTINATION include/${PROJECT_NAME})        