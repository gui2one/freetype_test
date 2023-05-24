cmake_minimum_required(VERSION 3.2)

project(freetype_test)

link_directories(

    vendor/msdfgen/Release
)

include_directories(
    vendor/freetype/include
    vendor/glm
    vendor/
)
add_executable(${PROJECT_NAME}

    src/main.cpp
)


#freetype
set(PNG_PNG_INCLUDE_DIR ${CMAKE_CURRENT_SOURCE_DIR}/vendor/libpng)
add_subdirectory(vendor/freetype)
target_include_directories(${PROJECT_NAME} PUBLIC ${FREETYPE_INCLUDE_DIRS})
target_link_libraries(${PROJECT_NAME} freetype ${FREETYPE_LIBRARY})

#poly2tri
add_subdirectory(vendor/poly2tri)
target_include_directories(${PROJECT_NAME} PUBLIC ${POLY2TRY_INCLUDE_DIRS})
target_link_libraries(${PROJECT_NAME} poly2tri ${POLY2TRY_LIBRARY})



