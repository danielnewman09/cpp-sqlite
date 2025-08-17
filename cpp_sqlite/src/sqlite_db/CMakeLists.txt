
target_sources(cpp_sqlite PRIVATE
    ${CMAKE_CURRENT_SOURCE_DIR}/example.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/DBDatabase.cpp
)

set(
    HEADER_FILES
        ${CMAKE_CURRENT_SOURCE_DIR}/example.hpp
        ${CMAKE_CURRENT_SOURCE_DIR}/DBBaseTransferObject.hpp
        ${CMAKE_CURRENT_SOURCE_DIR}/DBDataAccessObject.hpp
        ${CMAKE_CURRENT_SOURCE_DIR}/DBDatabase.hpp
        ${CMAKE_CURRENT_SOURCE_DIR}/DBTraits.hpp
        ${CMAKE_CURRENT_SOURCE_DIR}/DBRepeatedFieldTransferObject.hpp
# Add more headers here as needed
    PARENT_SCOPE
)

target_include_directories(${PROJECT_NAME} PUBLIC
$<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/..>
)