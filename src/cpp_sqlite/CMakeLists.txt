add_subdirectory(src)
add_subdirectory(test)
# add_subdirectory(Logging)

set(CPP_SQLITE_NAME cpp_sqlite)

add_library(${CPP_SQLITE_NAME})
set_target_properties(${CPP_SQLITE_NAME} PROPERTIES LINKER_LANGUAGE CXX)


# Add all relevant include directories
target_include_directories(${CPP_SQLITE_NAME} PUBLIC
$<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}>
$<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/cpp_sqlite/src>
)

# Add all sources (adjust as needed)
target_sources(${CPP_SQLITE_NAME} PRIVATE
    src/example.cpp
    src/example.hpp
    )

# Link the SQLite library to your executable
target_link_libraries(${CPP_SQLITE_NAME} PRIVATE SQLite::SQLite3)

