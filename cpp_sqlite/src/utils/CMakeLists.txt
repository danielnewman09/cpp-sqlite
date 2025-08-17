
target_sources(cpp_sqlite PRIVATE
    ${CMAKE_CURRENT_SOURCE_DIR}/Logger.cpp
)

set(
    HEADER_FILES
        ${CMAKE_CURRENT_SOURCE_DIR}/Logger.hpp
# Add more headers here as needed
    PARENT_SCOPE
)

target_include_directories(${PROJECT_NAME} PUBLIC
$<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/>
)